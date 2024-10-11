#include "PECache.h"

PE_CACHE_DATA l_PECacheData = {0};

PECacheHelper::PECacheHelper()
{
	PE_CACHE_Init();
}

PECacheHelper::~PECacheHelper()
{

}

BOOL PECacheHelper::PE_CACHE_Init()
{
	BOOLEAN retVal = FALSE;
	//InitializeListHead (&l_PECacheData.CacheHead);
	int bucket_count = PE_CAHCE_BUCKET_COUNT;
	PE_CACHE_Init_Internal(&l_PECacheData.lookupTB,
		__PeCacheValue, 
		__PeCacheRelease, 
		__ComparePeCache,
		__hashCalc,
		sizeof(PE_CACHE_ITEM_KEY*),
		HASHTYPE_WLPECACHE);

	PE_CACHE_SetBucketCount(&l_PECacheData.lookupTB, 1000);

	return TRUE;
}

BOOL PECacheHelper::PE_CACHE_Init_Internal(
			HASHTB * table, 
			HAHSTB_VALUE __getValuePtr,
			HAHSTB_RELEASE release_item, 
			HAHSTB_COMPARE compare_item, 
			HAHSTB_Calc  hash_calc,
			unsigned int valueLen,
			UCHAR type
			)
{
	if(0 != (valueLen % sizeof(int)))
	{
		return FALSE;
	}
	table->bucket_count = 0;
	table->hash_list = NULL;
	table->release_item = release_item;
	table->get_value = __getValuePtr;
	table->valueLen = valueLen;
	table->type = type;
	table->compare_item = compare_item;
	table->hash_calc = hash_calc;
	return TRUE; 	
}

BOOL PECacheHelper::PE_CACHE_SetBucketCount(HASHTB * table,unsigned short BucketCount)
{
	int real_count = BucketCount + 1;
	LIST_HEAD * retval = NULL;
	if((BucketCount <= 0))
	{
 		return FALSE;	
	}

	if((table->bucket_count == BucketCount))
	{
		return FALSE;	
	}

	retval = (LIST_HEAD * )malloc(sizeof(LIST_HEAD) * (real_count)); 	
	table->hash_list = retval;
	if((!table->hash_list))
	{
		table->bucket_count = 0;
		return FALSE;	
	} 

	table->bucket_count =  (unsigned short)BucketCount;
	for(int i = 0; i < real_count; i++)
	{
		__list_init(&table->hash_list[i]);	
	}

	table->bucket_count = BucketCount;

	return TRUE;
}


/********************************************************
函数：PE_CACHE_Query_Cache

作用：查缓存

参数：
	IN PUNICODE_STRING FilePath,文件路径
	IN PLARGE_INTEGER FileSize,文件大小
	IN PLARGE_INTEGER LastWriteTime,文件的最后修改时间
	OUT PBOOLEAN IsMatch,是否匹配上
	OUT int *CheckResult 返回缓存的结果
	OUT LARGE_INTEGER InsertTimeDif 返回上次插入时间和此次更新时间的差
返回值

	NTSTATUS ,成功，有效

********************************************************/
BOOL PECacheHelper::PE_CACHE_Query_Cache(IN std::wstring FilePath, IN ULONGLONG FileSize, IN ULONGLONG LastWriteTime, OUT PBOOLEAN IsMatch, OUT unsigned char* pWhiteListFileHash, OUT DWORD* InsertTimeDif)
{
	BOOL bRet = TRUE;
	PPE_CACHE_ITEM PECACHEItem = nullptr; 
	// 检查参数
	if (FilePath.empty() || !FileSize || !LastWriteTime)
	{
		return STATUS_INVALID_PARAMETER;
	}
	PECACHEItem = __createPECacheItem(FilePath,FileSize,LastWriteTime);

	if (hashtb_query_item(&l_PECacheData.lookupTB, (unsigned int*)&PECACHEItem->Key, HASHTYPE_WLPECACHE) == NULL)
	{
		return FALSE;
	}
	return TRUE;
}


/********************************************************
函数：PE_CACHE_insert

作用：插入

参数：
	IN PUNICODE_STRING FilePath,文件路径
	IN PLARGE_INTEGER FileSize,文件大小
	IN PLARGE_INTEGER LastWriteTime,文件的最后修改时间

返回值

	NTSTATUS ,成功

********************************************************/

BOOL PECacheHelper::PE_CACHE_insert(IN std::wstring FilePath, IN ULONGLONG FileSize, IN ULONGLONG LastWriteTime,IN std::wstring wstrWhiteListFileHash)
{
	BOOL bRet = TRUE;
	PPE_CACHE_ITEM PECACHEItem = nullptr; 
	// 检查参数
	if (FilePath.empty() || !FileSize || !LastWriteTime )
	{
		return STATUS_INVALID_PARAMETER;
	}

	PECACHEItem = __createPECacheItem(FilePath,FileSize,LastWriteTime);
	PECACHEItem->wstrFileHash = wstrWhiteListFileHash;
	if(!hashtb_insert_item(&l_PECacheData.lookupTB, &PECACHEItem->head, (unsigned int * )&PECACHEItem->Key,HASHTYPE_WLPECACHE))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL PECacheHelper::PE_CACHE_Clear_AllCache()
{
	if (!l_PECacheData.bInit)
	{
		return FALSE;
	}

	hashtb_remove_all (&l_PECacheData.lookupTB,HASHTYPE_WLPECACHE);

	return TRUE;
}


