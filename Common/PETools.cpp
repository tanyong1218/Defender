#include "PETools.h"
#include <tchar.h>

BOOL CPETool::IsPEFile(const std::wstring& wstrFilePath)
{
	FILE* fp = NULL;
	long location;
	unsigned char ch[4];
	unsigned char type[3];
	BOOL retval = FALSE;

	_tfopen_s(&fp, wstrFilePath.c_str(), _T("rb"));
	if (fp == NULL) goto DONE;

	location = 0x00;

	// -- 0x5A4D
	if (fseek(fp, location, 0) != 0) goto DONE;
	if (1 != fread(ch, 2, 1, fp)) goto DONE;
	if ((ch[0] != 0x4D) || (ch[1] != 0x5A)) goto DONE;

	location = 0x3c;
	if (fseek(fp, location, 0) != 0) goto DONE;
	fscanf_s(fp, "%c", &type[0]);

	if (feof(fp)) goto DONE;
	fscanf_s(fp, "%c", &type[1]);

	location = type[1] * 256 + type[0];

	if (fseek(fp, location, 0)) goto DONE;
	fscanf_s(fp, "%c", &ch[0]);

	if (feof(fp)) goto DONE;
	fscanf_s(fp, "%c", &ch[1]);

	if (((ch[0] != 0x50) || (ch[1] != 0x45)) &&
		((ch[0] != 0x4c) || (ch[1] != 0x45)) &&
		((ch[0] != 0x4D) || (ch[1] != 0x45)) &&
		((ch[0] != 0x4E) || (ch[1] != 0x45))) {
		goto DONE;
	}
	retval = TRUE;
DONE:
	if (fp) {
		fclose(fp);
	}
	return retval;
}

BOOL CPETool::PaserPeFile(const std::wstring& wstrFilePath)
{
	HANDLE	hRead	= INVALID_HANDLE_VALUE;
	DWORD	dwRet	= ERROR_SUCCESS;
	PBYTE	pBuffer = nullptr;
	BOOL	bResult = FALSE;
	MSI_PE_STRUCT   peData;


	IS_EXIST(wstrFilePath.c_str());
	// �������ļ����
	hRead = CreateFile(wstrFilePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// �ƶ����ļ���ͷ
	dwRet = SetFilePointer(hRead, 0, NULL, FILE_BEGIN);
	if (dwRet == INVALID_SET_FILE_POINTER)
	{
		goto END;
	}

	try
	{
		// ��ȡ�ļ�����
		DWORD dwFileSize = GetFileSize(hRead, NULL);
		pBuffer = new BYTE[dwFileSize];
		if (pBuffer == NULL)
		{
			goto END;
		}

		DWORD dwRead = 0;
		if (!ReadFile(hRead, pBuffer, dwFileSize, &dwRead, NULL))
		{
			goto END;
		}

		// ����PE�ļ�
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBuffer;
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		{
			goto END;
		}

		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)(pBuffer + pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
		{
			goto END;
		}

		peData.dos_header = *pDosHeader;
		// ���PE�ļ���Ϣ
		_tprintf(_T("PE�ļ���Ϣ��\n"));
		_tprintf(_T("�ļ���С��%d\n"), dwFileSize);
		_tprintf(_T("DOSͷ����С��%d\n"), pDosHeader->e_lfanew);
		_tprintf(_T("NTͷ����С��%d\n"), sizeof(IMAGE_NT_HEADERS));
		_tprintf(_T("�ڱ�������%d\n"), pNtHeader->FileHeader.NumberOfSections);
		_tprintf(_T("�ڱ��С��%d\n"), sizeof(IMAGE_SECTION_HEADER));
		_tprintf(_T("�ڱ�ƫ�ƣ�%d\n"), pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));
		_tprintf(_T("�ڱ���Ϣ��\n"));
		PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);
		for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++)
		{
			_tprintf(_T("������%s\n"), pSectionHeader->Name);
			_tprintf(_T("�ڴ�С��%d\n"), pSectionHeader->SizeOfRawData);
			_tprintf(_T("��ƫ�ƣ�%d\n"), pSectionHeader->PointerToRawData);
			_tprintf(_T("�ڵ�ַ��%d\n"), pSectionHeader->VirtualAddress);
			pSectionHeader++;
		}
	}
	catch (...)
	{
		goto END;
	}

	bResult = TRUE;

END:
	if (hRead)
	{
		CloseHandle(hRead);
	}
	if (pBuffer)
	{
		delete[] pBuffer;
		pBuffer = NULL;
	}

	return bResult;
}
