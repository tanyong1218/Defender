#include <tchar.h>
#include <Windows.h>
#include <string>
#include"HashHelper.h"


class PECacheHelper
{
public:
	PECacheHelper();
	~PECacheHelper();
	BOOL PE_CACHE_Query_Cache (IN std::wstring FilePath,IN ULONGLONG FileSize,IN ULONGLONG LastWriteTime,
		OUT PBOOLEAN IsMatch,OUT unsigned char *pWhiteListFileHash,OUT DWORD *InsertTimeDif);

	BOOL PE_CACHE_insert (IN std::wstring FilePath,IN ULONGLONG FileSize,IN ULONGLONG LastWriteTime,IN std::wstring wstrWhiteListFileHash);

	BOOL PE_CACHE_Clear_AllCache ();

	BOOL PE_CACHE_Init ();

	VOID PE_CACHE_Cleanup ();

	BOOL Check_PECacheNeedToExpire(PE_CACHE_ITEM *PECACHEItem, DWORD Time, DWORD MaxFileSize);

	VOID Clear_ExpireCache();
private:
	BOOL PE_CACHE_Init_Internal(HASHTB* table, HAHSTB_VALUE __getValuePtr, HAHSTB_RELEASE release_item, 
		HAHSTB_COMPARE compare_item, HAHSTB_Calc hash_calc, unsigned int valueLen, UCHAR type);
	BOOL PE_CACHE_SetBucketCount(HASHTB * table,unsigned short BucketCount);
};

