#pragma once
#include <iostream>
#include <string>	
#include <Windows.h>
#include <mutex>
#define INTEGRITY_LENGTH (20)
#define PE_CAHCE_BUCKET_COUNT	1024
#define  HASHTYPE_WLPECACHE		6L //V300R006C03
#define FNV_32_PRIME 0x01000193
#define FNV1_32_INIT 0x811c9dc5

#define OBJECT_PTR(address, type, field) ((type *)( \
													  (unsigned char *)(address) - \
													  (unsigned long)(&((type *)0)->field)))

#define __list_add_tail(head, item) \
do{	\
	LIST_HEAD * prev = (head)->prev;	\
	(item)->next = (head);				\
	(item)->prev = prev;				\
	prev->next = (item);				\
	(head)->prev = (item);				\
}while(0);

#define __list_init(list)	\
do{		\
	(list)->prev = list;	\
	(list)->next = list;	\
}while(0);

#define __list_add_head(head, item)	\
	do{		\
		LIST_HEAD * next = (head)->next;	\
		(item)->prev = (head);				\
		(item)->next = next;				\
		next->prev = (item);				\
		(head)->next = (item);				\
	}while(0);

#define __list_is_empty(list) ((list)->prev==list) 

#define __list_remove_item(item)	\
	do{	\
		LIST_HEAD * prev;	\
		LIST_HEAD * next;	\
		prev = (item)->prev;	\
		next = (item)->next;	\
		prev->next = next;	\
		next->prev = prev;	\
	}while(0);


typedef struct _LIST_HEAD LIST_HEAD;
typedef BOOL (* HAHSTB_RELEASE)(LIST_HEAD * head,BOOL allref,UCHAR type);
//比较是否一致的callback
typedef BOOL (* HAHSTB_COMPARE)(void* KeyPtr1,void* KeyPtr2);
typedef unsigned char* (* HAHSTB_VALUE)(LIST_HEAD * head,UCHAR type);  
//用户自己定义计算hash的callback（HAHSTB_Calc）
typedef unsigned int  (* HAHSTB_Calc)(void *table,void* valPtr);  

typedef struct _PE_CACHE_ITEM_KEY
{
	std::wstring FilePathBuffer;			//文件路径
	int	   FilePathLen;				//文件路径长度
	//UNICODE_STRING FilePath;
	ULONGLONG  FileSize;			//文件大小
	ULONGLONG  LastWriteTime;    //文件系统中上一此写文件的时间
}PE_CACHE_ITEM_KEY;

struct _LIST_HEAD
{
	LIST_HEAD * prev;
	LIST_HEAD * next;	
};

typedef struct __HASHTB
{
	unsigned short		bucket_count;	//hash表的桶数
	unsigned int        valueLen;		//value的长度
	LIST_HEAD * 		hash_list;		//hash表 数组，每个元素是一个链表
	long				item_count;		//hash表中的元素个数
	UCHAR				type;			//hash表的类型
	std::mutex			ListMutex;		//hash表的锁

	HAHSTB_RELEASE 	    release_item; 
	HAHSTB_VALUE  		get_value;
	HAHSTB_COMPARE		compare_item;
	HAHSTB_Calc			hash_calc;

}HASHTB;


//将已经判断过白名单的信息，组织成链表缓存，供查询以减少

typedef struct _PE_CACHE_ITEM
{
	LIST_HEAD head;					//hashtab头
	//compare content
	PE_CACHE_ITEM_KEY Key;			//hashtab key值

	std::wstring			wstrFileHash;	//文件哈希 
	//access time.
	LARGE_INTEGER			InsertTime;  //条目插入或者更新的时间
}
PE_CACHE_ITEM,*PPE_CACHE_ITEM;

typedef struct _PE_CACHE_DATA
{
	HASHTB		 lookupTB;
	int			 CacheCount;
	unsigned int matchCount;
	BOOLEAN		 bInit;
}
PE_CACHE_DATA,*PPE_CACHE_DATA;

BOOL __ComparePeCache(void *Key1,void*Key2);

unsigned char* __PeCacheValue(
	LIST_HEAD* entry,
	UCHAR type
);
void __destroyPeCache(PE_CACHE_ITEM* ctx);
BOOL __PeCacheRelease(
	LIST_HEAD* entry,
	BOOL allref,
	UCHAR type
);
unsigned int __hashCalc(void* table, void* KeyPtr);

ULONG hashkey(void* pdata, int len);

PE_CACHE_ITEM* __createPECacheItem(std::wstring FilePath, IN ULONGLONG FileSize, IN ULONGLONG LastWriteTime);

BOOLEAN
hashtb_insert_item(
	HASHTB* table,
	LIST_HEAD* item,
	unsigned int* valPtr,
	UCHAR type
);

LIST_HEAD*
hashtb_query_item(
	HASHTB* table,
	unsigned int* valPtr,
	UCHAR type
);

BOOL Misc_GetBootTime(OUT PLARGE_INTEGER BootTime);
void
hashtb_remove_all(
	HASHTB* table,
	UCHAR type
);