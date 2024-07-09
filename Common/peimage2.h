#pragma once
#include <windows.h>
#include <DbgHelp.h>
#include "scheme.h"
#include <malloc.h>
#include "../Common/sha_local.h"

extern "C" {
#include "../Common/sha1.h"
#include "../Common/sha256.h"
}

// -- typedef BOOL (WINAPI *DIGEST_FUNCTION) (DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength);

// -- mscdfapi.cpp
typedef struct __PE_DATA
{
	IMAGE_DOS_HEADER DosHeader;
	LONG_PTR imageLength; //
	union {
		IMAGE_NT_HEADERS32 u32;
		IMAGE_NT_HEADERS64 u64;
	}ntHdr;
	LONG_PTR ntHdrOffset;
	DWORD NumberOfSections;
	BOOLEAN is64Bit;
	BOOLEAN isPE;
}PE_DATA;

typedef PVOID DIGEST_HANDLE;
typedef BOOL(WINAPI* DIGEST_FUNCTION) (DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength);
#ifdef __cplusplus
#define DELETE_OBJECT(obj0)     if (obj0)           \
	{                   \
		delete obj0;    \
		obj0 = NULL;    \
	}
#else
#define DELETE_OBJECT(obj0)     if (obj0)           \
	{                   \
		free(obj0);     \
		obj0 = NULL;    \
	}
#endif

BOOL WINAPI digest_sha1(DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength);
BOOL WINAPI digest_sha1_lib(DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength);
BOOL __KeReadDataSpec(HANDLE handle, LONG_PTR offset, unsigned char* data, unsigned int length);

extern "C" PVOID
__tmp_ImageDirectoryEntryToData(
	IN PVOID Base,
	IN BOOLEAN MappedAsImage,
	IN USHORT DirectoryEntry,
	OUT PULONG Size
);

__inline DWORD AlignIt(DWORD Value, DWORD Alignment) { return (Value + (Alignment - 1)) & ~(Alignment - 1); }
BOOL ImageDigestCalcExt(const WCHAR* path, unsigned char* digest, BOOL force);
BOOL ImageDigestCalcExtBySHA1Lib(const WCHAR* path, unsigned char* digest, BOOL bForce);

BOOL ImageDigestCalcExt_SHA2(const char* path, unsigned char* digest);

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define MAP_READONLY  TRUE
#define MAP_READWRITE FALSE