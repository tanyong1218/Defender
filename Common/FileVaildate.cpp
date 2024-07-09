#include "FileVaildate.h"
#include "LogHelper.h"
#include "peimage2.h"
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

void startTime(LARGE_INTEGER& start)
{
	// ��ʼ��ʱ
	QueryPerformanceCounter(&start);
}

double endTime(LARGE_INTEGER& start)
{
	LARGE_INTEGER end;
	double elapsedTime;

	LARGE_INTEGER frequency;
	// ��ȡ�߾��ȼ�ʱ����Ƶ��
	QueryPerformanceFrequency(&frequency);
	// ֹͣ��ʱ
	QueryPerformanceCounter(&end);

	// ���㾭����ʱ�䣨��΢��Ϊ��λ��
	elapsedTime = (double)(end.QuadPart - start.QuadPart) * 1000000.0 / frequency.QuadPart;

	return elapsedTime;
}

CPEFileValidate::CPEFileValidate(void)
{
}

CPEFileValidate::~CPEFileValidate(void)
{
}

BOOL CPEFileValidate::GetPEFileDegist(const TCHAR* szFileFullPath, unsigned char bHashCode[INTEGRITY_LENGTH])
{
	if (NULL == szFileFullPath || NULL == bHashCode)
	{
		WriteError(("GetPEFileDegistW parameters error."));
		return FALSE;
	}
	BOOL bRet = FALSE;

	bRet = ImageDigestCalcExt(szFileFullPath, bHashCode, TRUE);

	if (!bRet)
	{
		WriteError(("GetPEFileDegistW failed."));
		return FALSE;
	}

	return bRet;
}

BOOL CPEFileValidate::GetPEFileDegistByLib(const TCHAR* szFileFullPath, unsigned char bHashCode[INTEGRITY_LENGTH])
{
	if (NULL == szFileFullPath || NULL == bHashCode)
	{
		WriteError(("GetPEFileDegistW parameters error."));
		return FALSE;
	}
	BOOL bRet = FALSE;

	bRet = ImageDigestCalcExtBySHA1Lib(szFileFullPath, bHashCode, TRUE);

	if (!bRet)
	{
		WriteError(("GetPEFileDegistW failed."));
		return FALSE;
	}

	return bRet;
}