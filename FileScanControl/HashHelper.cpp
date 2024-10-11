#include "HashHelper.h"


BOOL __ComparePeCache(void *Key1,void*Key2)
{
	PE_CACHE_ITEM_KEY *pKey1 = (PE_CACHE_ITEM_KEY *)Key1;
	PE_CACHE_ITEM_KEY *pKey2 = (PE_CACHE_ITEM_KEY *)Key2;

	if (NULL == pKey1 || NULL == pKey2 || pKey1->FilePathBuffer.empty() || pKey2->FilePathBuffer.empty())
	{
		return FALSE;
	}


	if(pKey1->FileSize != pKey2->FileSize || pKey1->LastWriteTime != pKey2->LastWriteTime || pKey1->FilePathLen != pKey2->FilePathLen)
	{
		return FALSE;
	}


	if (pKey1->FilePathBuffer.compare(pKey2->FilePathBuffer) != 0)
	{
		return FALSE;
	}
	return TRUE;
}

unsigned char  * __PeCacheValue(
			   LIST_HEAD * entry,
			   UCHAR type
			   )
{
	PE_CACHE_ITEM *	ctx = OBJECT_PTR(entry, PE_CACHE_ITEM, head);

	UNREFERENCED_PARAMETER(type);
	return ( unsigned char * )&ctx->Key;
}

void __destroyPeCache(PE_CACHE_ITEM * ctx)
{
	if (ctx == NULL)
	{
		goto DONE;
	}

	if(!ctx->Key.FilePathBuffer.empty())
	{
		ctx->Key.FilePathBuffer.clear();
		//KFfree(ctx->Key.FilePathBuffer);
		//g_ExFreePool[SERVICE_WL_PE_CACHE]++;

	}
	//KFfree(ctx);
	//g_ExFreePool[SERVICE_WL_PE_CACHE]++;

DONE:
	return;
}

BOOL __PeCacheRelease(
				 LIST_HEAD * entry,
				 BOOL allref,
				 UCHAR type
				 )
{
	BOOLEAN		bDelete = FALSE;
	PE_CACHE_ITEM*	ctx		= OBJECT_PTR(entry, PE_CACHE_ITEM, head);
	LONG		lRef	= 0;

	if (allref)
	{
		__destroyPeCache(ctx);
		bDelete = TRUE;
	}
	else
	{
		//xobject_release(ctx);
	}


	return bDelete;
}


ULONG hashkey(void* pdata, int len) 
{
	ULONG hash = FNV1_32_INIT;
	unsigned char* data = (unsigned char*)pdata;
	int i;
	for (i = 0; i < len; i++) 
	{
		hash ^= data[i];
		hash *= FNV_32_PRIME;
	}

	return hash;
}

unsigned int __hashCalc(void* table,void* KeyPtr)
{
	unsigned int retval = 0;
	HASHTB * pTable = (HASHTB *)(table);
	PE_CACHE_ITEM_KEY* pKey = (PE_CACHE_ITEM_KEY*)KeyPtr;

	unsigned int *p = (unsigned int *)&pKey->FileSize;
	int count = sizeof(pKey->FileSize);
	ULONG HashKey =	hashkey(p,count)+ hashkey((unsigned int *)&pKey->LastWriteTime,sizeof(pKey->LastWriteTime));
	ULONG HashKey2 = hashkey((void*)pKey->FilePathBuffer.c_str(), pKey->FilePathLen * 2);

	
	retval = ( (unsigned int)(HashKey2+HashKey) % (unsigned int)pTable->bucket_count);
	return retval;
}

BOOL Misc_GetBootTime (OUT PLARGE_INTEGER BootTime)
{
	LARGE_INTEGER Time; 
	ULONG units;

	if (!BootTime)
	{
		return STATUS_INVALID_PARAMETER;
	}

	// 获取自系统启动以来的毫秒数
	ULONGLONG tickCount = GetTickCount64();
	BootTime->QuadPart = tickCount * 10000;

	return TRUE; 
}

PE_CACHE_ITEM* __createPECacheItem(std::wstring FilePath,IN ULONGLONG FileSize,IN ULONGLONG LastWriteTime)
{
	NTSTATUS	Status;
	//后续在其他地方Free
	PE_CACHE_ITEM*	PECACHEItem = (PE_CACHE_ITEM*)malloc(sizeof (PE_CACHE_ITEM));
	if (!PECACHEItem)
	{
		return NULL; //STATUS_INSUFFICIENT_RESOURCES;
	}
	memset (PECACHEItem,0,sizeof (PE_CACHE_ITEM));

	PECACHEItem->Key.FileSize = FileSize;
	PECACHEItem->Key.LastWriteTime = LastWriteTime;
	PECACHEItem->Key.FilePathBuffer = FilePath;
	PECACHEItem->Key.FilePathLen = FilePath.length();
	Misc_GetBootTime (&PECACHEItem->InsertTime);
	return PECACHEItem;
}

static 
unsigned int 
hashtb_hash(
	HASHTB * table, 
	unsigned int * valPtr
	)
{
	if(!table->hash_calc)
	{
		return 0;
	}
	else
	{
		return table->hash_calc(table,valPtr);
	}
}

static 
BOOLEAN 
__hashtb_equal(
	HASHTB * table, 
	LIST_HEAD * item, 
	unsigned int * ptrVal,
	UCHAR type
	)
{
	if(!table->compare_item)
	{
		return 0;
	}
	else
	{
		void* ItemKey = table->get_value(item,type);
		return table->compare_item(ItemKey,ptrVal);
	}
	
}

static  
BOOLEAN
__hashtb_insert_item(
	HASHTB * table, 
	LIST_HEAD * item, 
	unsigned int * valPtr,
	UCHAR type
	)
{
	unsigned int index;
 	LIST_HEAD * entry;
	if((0==table->bucket_count))
	{
		return FALSE;
	}
	index = hashtb_hash(table, valPtr);		//计算Hash下标
	
	{
		std::unique_lock<std::mutex> lock(table->ListMutex);
		entry = table->hash_list[index].next;	//获取Hash冲突的链表头
		while (entry != &table->hash_list[index])
		{
			if (__hashtb_equal(table, entry, valPtr, type))
			{
				return FALSE;
			}
			entry = entry->next;
		}
	}
	__list_add_tail(&table->hash_list[index], item);
	
	return TRUE;		
}


BOOLEAN 
hashtb_insert_item(
	HASHTB * table,
	LIST_HEAD * item, 
	unsigned int * valPtr,
	UCHAR type
	)
{
	BOOL retval = FALSE;
	retval = __hashtb_insert_item(table, item, valPtr,type);
	if(retval)
	{
		table->item_count++;
	}
	return retval;		
}


LIST_HEAD* 
hashtb_query_item(
	HASHTB* table,
	unsigned int* valPtr,
	UCHAR type
)
{
	unsigned int index;
	LIST_HEAD * entry;
	if ((0 == table->bucket_count))
	{
		return NULL;
	}
	index = hashtb_hash(table, valPtr);		//计算Hash下标


	//锁定链表
	{
		std::unique_lock<std::mutex> lock(table->ListMutex);
		entry = table->hash_list[index].next;	//获取Hash冲突的链表头

		if (entry == NULL || entry->next == NULL)
		{
			return NULL;
		}

		while (entry != &table->hash_list[index])
		{
			if (__hashtb_equal(table, entry, valPtr, type))
			{
				return entry;
			}
			entry = entry->next;
			if (entry == NULL)
			{
				return NULL;
			}
		}
	}
	return NULL;		
}

void 
hashtb_remove_all(
	HASHTB* table,
	UCHAR type
)
{
	if((0 == table->bucket_count))
	{
		return;	
	}	
	for(int i = 0; i<table->bucket_count; i++)
	{
		std::unique_lock<std::mutex> lock(table->ListMutex);
		LIST_HEAD * header = &table->hash_list[i];
		while(!__list_is_empty(header))
		{
			LIST_HEAD * entry = header->next;
			__list_remove_item(entry);
			table->item_count--;
			__list_init(entry);
			table->release_item(entry,TRUE,type);	
		}
	}
	
	return;
}