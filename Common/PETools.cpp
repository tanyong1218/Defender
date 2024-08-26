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

MSI_PE_STRUCT CPETool::PaserPeFile(const std::wstring& wstrFilePath)
{
	HANDLE	hRead = INVALID_HANDLE_VALUE;
	DWORD	dwRet = ERROR_SUCCESS;
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
		if (pNtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
		{
			//32λ
			peData.is_pe64 = false;
			peData.nt_headers32 = *(PIMAGE_NT_HEADERS32)(pBuffer + pDosHeader->e_lfanew);
		}
		else if (pNtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
		{
			//64λ
			peData.is_pe64 = true;
			peData.nt_headers64 = *(PIMAGE_NT_HEADERS64)(pBuffer + pDosHeader->e_lfanew);
		}

		PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);
		for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++)
		{
			IMAGE_SECTION_HEADER SectionHeader = *pSectionHeader;
			peData.section.push_back(SectionHeader);
			pSectionHeader++;
		}

		// �������ű�
		PIMAGE_DEBUG_DIRECTORY pDebugDirectory;
		PIMAGE_DATA_DIRECTORY pDebugDataDirectory;
		PIMAGE_EXPORT_DIRECTORY pExportDirectory;
		PIMAGE_DATA_DIRECTORY pExportDataDirectory;
		if (peData.is_pe64)
		{
			pDebugDataDirectory = &peData.nt_headers64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
			pExportDataDirectory = &peData.nt_headers64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			pDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PBYTE)pDosHeader +
				ConvertRvaToFoa(pDebugDataDirectory->VirtualAddress, pDosHeader));
			pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)pDosHeader +
				ConvertRvaToFoa(pExportDataDirectory->VirtualAddress, pDosHeader));
		}
		else
		{
			pDebugDataDirectory = &peData.nt_headers32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
			pExportDataDirectory = &peData.nt_headers32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			pDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PBYTE)pDosHeader +
				ConvertRvaToFoa(pDebugDataDirectory->VirtualAddress, pDosHeader));
			pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)pDosHeader +
				ConvertRvaToFoa(pExportDataDirectory->VirtualAddress, pDosHeader));
		}

		if (pExportDirectory->NumberOfFunctions > 0)
		{
			PWORD AddressOfNameOrdinals = (PWORD)((PBYTE)pDosHeader + ConvertRvaToFoa(pExportDirectory->AddressOfNameOrdinals, pDosHeader));
			if (AddressOfNameOrdinals != nullptr)
			{
				for (int i = 0; i < pExportDirectory->NumberOfNames; i++)
				{
					WORD Ordinal = *(AddressOfNameOrdinals++);
					DWORD NameRva = *(DWORD*)((PBYTE)pDosHeader + ConvertRvaToFoa(pExportDirectory->AddressOfNames, pDosHeader) + i * sizeof(DWORD));
					std::string Name = (char*)((PBYTE)pDosHeader + ConvertRvaToFoa(NameRva, pDosHeader));
					DWORD FunctionRva = *(DWORD*)((PBYTE)pDosHeader + ConvertRvaToFoa(pExportDirectory->AddressOfFunctions, pDosHeader));
					auto x = ((PBYTE)pDosHeader + ConvertRvaToFoa(FunctionRva, pDosHeader) + Ordinal * sizeof(DWORD));
					DWORD Address = *(DWORD*)((PBYTE)pDosHeader + ConvertRvaToFoa(FunctionRva, pDosHeader) + Ordinal * sizeof(DWORD));
					peData.va_to_symbol[Ordinal] = Name;

					if (Name.compare("UDV_CloseAPI"))
					{
						// ȷ��Ŀ���ַ�ǿ�ִ�е�
						DWORD oldProtect;
						VirtualProtect((void*)Address, 4, PAGE_EXECUTE_READ, &oldProtect);
						// ���ú���
						typedef UINT32(__cdecl* PWL_UDV_CloseAPI)();
						PWL_UDV_CloseAPI func = (PWL_UDV_CloseAPI)Address;
						func();
						// �ָ�ԭʼ��������
						VirtualProtect((void*)Address, 4, oldProtect, &oldProtect);
					}



				}
			}
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

	return peData;
}



size_t CPETool::ConvertRvaToFoa(size_t RVA, LPVOID pFileBuffer)
{
	int i;//���ڱ����ڱ�
	//	size_t FOA = 0;

		//�����ͷָ��
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;

	//����ͷ����ֵ

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pFileBuffer + pDosHeader->e_lfanew);
	//��һ���ڱ�ͷ

	pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);
	if (RVA < pSectionHeader->VirtualAddress)//�ж�RVA�Ƿ���PEͷ��
	{
		if (RVA < pSectionHeader->PointerToRawData)
			return RVA;//��ʱFOA == RVA
		else
			return 0;
	}

	for (i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++)//ѭ�������ڱ�ͷ
	{
		if (RVA >= pSectionHeader->VirtualAddress)//�Ƿ��������ڱ��RVA
		{
			if (RVA <= pSectionHeader->VirtualAddress + pSectionHeader->SizeOfRawData)//�ж��Ƿ����������
			{
				return (RVA - pSectionHeader->VirtualAddress) + pSectionHeader->PointerToRawData;//ȷ�������󣬼���FOA
			}
		}
		else//RVA�����ܴ�ʱ��pSectionHeader->VirtuallAddressС�������Ƿ���ֵΪ0�������
		{
			return 0;
		}
		pSectionHeader++;
	}

	return 0;
}


