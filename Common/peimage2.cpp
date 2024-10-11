#include "peimage2.h"
void __initPEData(PE_DATA* _PEData)
{
	_PEData->imageLength = 0;
	_PEData->ntHdrOffset = 0;
	_PEData->NumberOfSections = 0;
	_PEData->is64Bit = FALSE;
	_PEData->isPE = FALSE;
	return;
}

BOOL CalculateImagePtrs(HANDLE handle, PE_DATA* _PEData)
{
	BOOL retval = FALSE;
	DWORD outLen = 0;

	if (!ReadFile(handle, (char*)&_PEData->DosHeader, sizeof(_PEData->DosHeader), &outLen, NULL)) {
		goto DONE;
	}
	if (outLen != sizeof(_PEData->DosHeader)) goto DONE;

	if ((_PEData->DosHeader.e_magic != IMAGE_DOS_SIGNATURE) &&
		(_PEData->DosHeader.e_magic != IMAGE_NT_SIGNATURE)) {
		goto DONE;
	}
	if (_PEData->DosHeader.e_magic == IMAGE_DOS_SIGNATURE) {
		LONG_PTR _fileHeaderOffset;

		if (_PEData->DosHeader.e_lfanew == 0) {
			goto DONE;
		}
		_fileHeaderOffset = _PEData->DosHeader.e_lfanew;
		_PEData->ntHdrOffset = _fileHeaderOffset;
		// -- If IMAGE_NT_HEADERS would extend past the end of file...
		if (_fileHeaderOffset + (LONG_PTR)sizeof(IMAGE_NT_HEADERS32) > _PEData->imageLength) {
			goto DONE;
		}
		// -- or if it would begin in, or before the IMAGE_DOS_HEADER...
		if (_fileHeaderOffset < (LONG_PTR)sizeof(IMAGE_DOS_HEADER)) {
			goto DONE;
		}
	}
	else {
		// -- No DOS header indicates an image built w/o a dos stub
		_PEData->ntHdrOffset = 0;
		// -- goto DONE; // -- add by fyx
		if (_PEData->imageLength < (LONG_PTR)sizeof(IMAGE_NT_HEADERS32)) goto DONE;
	}
	if (!__KeReadDataSpec(handle, _PEData->ntHdrOffset, (unsigned char*)&_PEData->ntHdr.u32, sizeof(IMAGE_NT_HEADERS32))) {
		goto DONE;
	}
	if (_PEData->ntHdr.u32.Signature != IMAGE_NT_SIGNATURE) {
		unsigned short ntSign = (unsigned short)(_PEData->ntHdr.u32.Signature & 0x0000FFFF);
		// -- other format NE LE .etc.
		if (IMAGE_DOS_SIGNATURE == ntSign ||
			IMAGE_OS2_SIGNATURE == ntSign ||
			IMAGE_OS2_SIGNATURE_LE == ntSign) {
			_PEData->NumberOfSections = 0;
			_PEData->isPE = FALSE;
			retval = TRUE;
		}
		goto DONE;
	}

	// No optional header indicates an object...
	if (!_PEData->ntHdr.u32.FileHeader.SizeOfOptionalHeader) {
		goto DONE;
	}
	// Check for versions < 2.50

	if (_PEData->ntHdr.u32.OptionalHeader.MajorLinkerVersion < 3 &&
		_PEData->ntHdr.u32.OptionalHeader.MinorLinkerVersion < 5) {
		goto DONE;
		// -- __leave;
	}
	_PEData->isPE = TRUE;
	_PEData->NumberOfSections = _PEData->ntHdr.u32.FileHeader.NumberOfSections;

	retval = TRUE;
DONE:
	return retval;
}

typedef struct _EXCLUDE_RANGE {
	DWORD Offset;
	DWORD Size;
	struct _EXCLUDE_RANGE* Next;
} EXCLUDE_RANGE;

class EXCLUDE_LIST
{
public:
	EXCLUDE_LIST() {
		m_Image = NULL;
		m_ExRange = new EXCLUDE_RANGE;

		if (m_ExRange)
			memset(m_ExRange, 0x00, sizeof(EXCLUDE_RANGE));
	}

	~EXCLUDE_LIST() {
		EXCLUDE_RANGE* pTmp;
		pTmp = m_ExRange->Next;
		while (pTmp)
		{
			DELETE_OBJECT(m_ExRange);
			m_ExRange = pTmp;
			pTmp = m_ExRange->Next;
		}
		DELETE_OBJECT(m_ExRange);
	}

	void Init(LOADED_IMAGE* Image, DIGEST_FUNCTION pFunc, DIGEST_HANDLE dh) {
		m_Image = Image;
		m_ExRange->Offset = NULL;
		m_ExRange->Size = 0;
		m_pFunc = pFunc;
		m_dh = dh;
		return;
	}

	void Add(DWORD_PTR Offset, DWORD Size);

	BOOL Emit(HANDLE handle, DWORD Size);
	BOOL __calculate(HANDLE handle, LONG_PTR Offset, DWORD size, unsigned char* _buffer, DWORD _bufLen);
private:
	LOADED_IMAGE* m_Image;
	EXCLUDE_RANGE* m_ExRange;
	DIGEST_FUNCTION m_pFunc;
	DIGEST_HANDLE m_dh;
};

void
EXCLUDE_LIST::Add(
	DWORD_PTR Offset,
	DWORD Size
)
{
	EXCLUDE_RANGE* pTmp, * pExRange;

	pExRange = m_ExRange;

	while (pExRange->Next && (pExRange->Next->Offset < Offset)) {
		pExRange = pExRange->Next;
	}

	pTmp = new EXCLUDE_RANGE;

	if (pTmp)
	{
		pTmp->Next = pExRange->Next;
		pTmp->Offset = Offset;
		pTmp->Size = Size;
		pExRange->Next = pTmp;
	}

	return;
}

BOOL EXCLUDE_LIST::__calculate(HANDLE handle, LONG_PTR Offset, DWORD size, unsigned char* _buffer, DWORD _bufLen)
{
	BOOL retval = FALSE;
	LARGE_INTEGER to, nPos;
	int outLen = 0;
	int left = size;

	to.QuadPart = Offset;
	if (!SetFilePointerEx(handle, to, &nPos, FILE_BEGIN)) {
		goto DONE;
	}

	while (left > 0) {
		DWORD length = min(left, (int)_bufLen);
		DWORD outLen = 0;
		if (!ReadFile(handle, (char*)_buffer, length, &outLen, NULL)) {
			goto DONE;
		}
		if (outLen != length) goto DONE;

		(*m_pFunc)((DIGEST_HANDLE*)m_dh, _buffer, length);
		left = left - length;
	}

	retval = TRUE;
DONE:
	return retval;
}

BOOL
EXCLUDE_LIST::Emit(
	HANDLE handle,
	DWORD Size
)
{
	BOOL retval = FALSE;

	EXCLUDE_RANGE* pExRange;
	DWORD EmitSize, ExcludeSize;

#define EMIT_BUFFER_SIZE (4096*10)
	unsigned char* _buffer = NULL;
	LONG_PTR Offset = 0;
	_buffer = (unsigned char*)malloc(EMIT_BUFFER_SIZE);

	if (!_buffer) goto DONE;

	pExRange = m_ExRange->Next;

	while (pExRange && (Size > 0)) {
		if (pExRange->Offset >= (ULONG)Offset) {
			// Emit what's before the exclude list.
			EmitSize = min((DWORD)(pExRange->Offset - Offset), Size);
			if (EmitSize) {
				if (!__calculate(handle, Offset, EmitSize, _buffer, EMIT_BUFFER_SIZE)) {
					goto DONE;
				}
				Size -= EmitSize;
				Offset += EmitSize;
			}
		}

		if (Size) {
			if (pExRange->Offset + pExRange->Size >= (DWORD)Offset) {
				// Skip over what's in the exclude list.
				ExcludeSize = min(Size, (DWORD)(pExRange->Offset + pExRange->Size - Offset));
				Size -= ExcludeSize;
				Offset += ExcludeSize;
			}
		}

		pExRange = pExRange->Next;
	}

	// Emit what's left.
	if (Size) {
		if (!__calculate(handle, Offset, Size, _buffer, EMIT_BUFFER_SIZE)) {
			goto DONE;
		}
	}
	retval = TRUE;
DONE:
	if (_buffer) {
		free(_buffer);
	}
	return retval;
}

BOOL __AuImageGetDigestStream(
	IN HANDLE handle,
	EXCLUDE_LIST& ExList,
	IN DWORD            DigestLevel,
	IN DIGEST_FUNCTION  DigestFunction,
	IN DIGEST_HANDLE    DigestHandle,
	BOOL bForce
)
{
	PE_DATA* _PEData = NULL;
	LOADED_IMAGE    LoadedImage;
	int            rc = ERROR_SUCCESS;
	LARGE_INTEGER fileSize;
	BOOL retval = FALSE;
	LONG_PTR 		offset;

	if (!GetFileSizeEx(handle, &fileSize))
	{
		rc = GetLastError();
		goto DONE;
	}

	if (fileSize.QuadPart == 0)
	{
		rc = 0xFFFFFE01;
		goto DONE;
	}

	_PEData = (PE_DATA*)malloc(sizeof(PE_DATA));
	if (!_PEData)
	{
		rc = ERROR_NOT_ENOUGH_MEMORY;
		goto DONE;
	}

	__initPEData(_PEData);
	_PEData->imageLength = (LONG)fileSize.QuadPart;
	if (!CalculateImagePtrs(handle, _PEData)) {
		if (bForce) {
			_PEData->isPE = FALSE;
		}
		else
		{
			rc = GetLastError();
			goto DONE;
		}
	}
	rc = ERROR_INVALID_PARAMETER;
	ExList.Init(&LoadedImage, DigestFunction, DigestHandle);

	__try {
		if (_PEData->NumberOfSections > 0) {
			if ((_PEData->ntHdr.u32.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) &&
				(_PEData->ntHdr.u32.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC))
			{
				if (!bForce)
				{
					rc = 0xFFFFFE02;
					goto DONE;
				}
			}
			else {
				if (_PEData->ntHdr.u32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
					PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)(&_PEData->ntHdr.u32);

					offset = (unsigned char*)&NtHeader32->OptionalHeader.CheckSum - (unsigned char*)NtHeader32;
					offset += _PEData->ntHdrOffset;
					// Exclude the checksum.
					ExList.Add((DWORD_PTR)offset, sizeof(NtHeader32->OptionalHeader.CheckSum));

					offset = (unsigned char*)&NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] - (unsigned char*)NtHeader32;
					offset += _PEData->ntHdrOffset;
					// Exclude the Security directory.
					ExList.Add(offset,
						sizeof(NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

					// Exclude the certs.
					ExList.Add((DWORD_PTR)NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress,
						NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
				}
				else {
					PIMAGE_NT_HEADERS64 NtHeader64 = NULL;
					if (!__KeReadDataSpec(handle, _PEData->ntHdrOffset, (unsigned char*)&_PEData->ntHdr.u64, sizeof(IMAGE_NT_HEADERS64))) goto DONE;

					NtHeader64 = (PIMAGE_NT_HEADERS64)(&_PEData->ntHdr.u64);

					offset = (unsigned char*)&NtHeader64->OptionalHeader.CheckSum - (unsigned char*)NtHeader64;
					offset += _PEData->ntHdrOffset;

					// Exclude the checksum.
					ExList.Add(offset, sizeof(NtHeader64->OptionalHeader.CheckSum));

					offset = (unsigned char*)&NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] - (unsigned char*)NtHeader64;
					offset += _PEData->ntHdrOffset;
					// Exclude the Security directory.
					ExList.Add(((DWORD_PTR)offset),
						sizeof(NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

					// Exclude the certs.
					ExList.Add((DWORD_PTR)NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress,
						NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
				}
			}
		}
		ExList.Emit(handle, _PEData->imageLength);
		rc = ERROR_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		rc = 0xFFFFFE03;
	}

	retval = TRUE;
DONE:
	SetLastError(rc);

	if (_PEData) {
		free(_PEData);
	}
	return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}

BOOL __KeReadDataSpec(HANDLE handle, LONG_PTR offset, unsigned char* data, unsigned int length)
{
	BOOL retval;
	DWORD outLen = 0;
	LARGE_INTEGER to, nPos;
	to.QuadPart = offset;
	if (!SetFilePointerEx(handle, to, &nPos, FILE_BEGIN)) {
		goto DONE;
	}

	if (!ReadFile(handle, data, length, &outLen, NULL)) {
		goto DONE;
	}
	if (outLen != length) goto DONE;
	retval = TRUE;
DONE:
	return retval;
}

BOOL
imagehack_AuImageGetDigestStream(
	HANDLE handle,
	IN DWORD            DigestLevel,
	IN DIGEST_FUNCTION  DigestFunction,
	IN DIGEST_HANDLE    DigestHandle,
	BOOL bForce
)

/*++

Routine Description:
	Given an image, return the bytes necessary to construct a certificate.
	Only PE images are supported at this time.

Arguments:
	FileHandle  -   Handle to the file in question.  The file should be opened
					with at least GENERIC_READ access.
	DigestLevel -   Indicates what data will be included in the returned buffer.
					Valid values are:
					CERT_PE_IMAGE_DIGEST_ALL_BUT_CERTS - Include data outside the PE image itself
															(may include non-mapped debug symbolic)
	DigestFunction - User supplied routine that will process the data.
	DigestHandle -  User supplied handle to identify the digest.  Passed as the first
					argument to the DigestFunction.

Return Value:
	TRUE         - Success.
	FALSE        - There was some error.  Call GetLastError for more information.  Possible
				   values are ERROR_INVALID_PARAMETER or ERROR_OPERATION_ABORTED.
--*/

{
	EXCLUDE_LIST ExList;
	BOOL rc;
	rc = __AuImageGetDigestStream(
		handle,
		ExList,
		DigestLevel,
		DigestFunction,
		DigestHandle,
		bForce
	);
	return rc;
}

BOOL ImageDigestCalcExt(const WCHAR* path, unsigned char* digest, BOOL bForce)
{
	BOOL retval = FALSE;
	HANDLE fHandle = NULL;

	int DigestLevel = 0;
	sha1_context ctx;
	fHandle = CreateFileW(path, GENERIC_READ,
		FILE_SHARE_READ, NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)0);
	if (INVALID_HANDLE_VALUE == fHandle) goto DONE;
	sha1_starts(&ctx);

	retval = imagehack_AuImageGetDigestStream(
		fHandle,
		DigestLevel,
		digest_sha1,
		(DIGEST_HANDLE)&ctx,
		bForce
	);
	sha1_finish(&ctx, digest);

DONE:
	if (INVALID_HANDLE_VALUE != fHandle) {
		::CloseHandle(fHandle);
	}
	return retval;

	return 0;
}

int ImageDigestCalcExt_SHA2(const char* path, unsigned char* digest)
{
	return  0;//sha256_file(path, digest, FALSE);
}
BOOL WINAPI digest_sha1(DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength)

{
	sha1_context* ctx = (sha1_context*)refdata;
	sha1_update(ctx, (unsigned char*)pData, dwLength);
	return TRUE;
}

BOOL ImageDigestCalcExtBySHA1Lib(const WCHAR* path, unsigned char* digest, BOOL bForce)
{
	BOOL retval = FALSE;
	HANDLE fHandle = NULL;

	int DigestLevel = 0;
	SHA_CTX ctx;
	unsigned int digestLength;
	fHandle = CreateFileW(path, GENERIC_READ,
		FILE_SHARE_READ, NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)0);

	if (INVALID_HANDLE_VALUE == fHandle) goto DONE;
	SHA1_Init(&ctx);
	retval = imagehack_AuImageGetDigestStream(
		fHandle,
		DigestLevel,
		digest_sha1_lib,
		(DIGEST_HANDLE)&ctx,
		bForce
	);
	SHA1_Final(digest, &ctx);

DONE:
	if (INVALID_HANDLE_VALUE != fHandle) {
		::CloseHandle(fHandle);
	}
	return retval;
}

BOOL WINAPI digest_sha1_lib(DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength)
{
	/*EVP_DigestUpdate((EVP_MD_CTX *)refdata, pData, dwLength);*/

	SHA1_Update((SHA_CTX*)refdata, pData, dwLength);
	return TRUE;
}