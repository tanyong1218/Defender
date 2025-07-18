﻿#pragma once
#include <string>
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <cstdint>
#include <map>
namespace fs = boost::filesystem;  //Boost库


#define IS_EXIST(file) (INVALID_FILE_ATTRIBUTES != GetFileAttributes(file))

typedef struct __MSI_PE_STRUCT
{
	uint64_t address{};
	uint64_t size{};
	bool	 is_pe64{};
	bool	 is_memory_layout{};
	IMAGE_DOS_HEADER dos_header{};
	union 
	{
		IMAGE_NT_HEADERS64 nt_headers64{};
		IMAGE_NT_HEADERS32 nt_headers32;
	};
	std::vector<IMAGE_SECTION_HEADER> section{};
	std::map<uint64_t, std::string> va_to_symbol{};
}MSI_PE_STRUCT,*PMSI_PE_STRUCT;

class CPETool
{
public:
	static BOOL IsPEFile(const std::wstring& wstrFilePath);
	static MSI_PE_STRUCT PaserPeFile(const std::wstring& wstrFilePath);
	static size_t ConvertRvaToFoa(size_t RVA, LPVOID pFileBuffer);
};
