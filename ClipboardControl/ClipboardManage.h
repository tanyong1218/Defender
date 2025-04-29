#pragma once
#include <windows.h>
#include <cstdint>
#include <singletonClass.h>
#include "IComponent.h"
#define SPICE_ATTR_PACKED
#define SPICE_ATTR_ALIGNED(n) __declspec (align (n))
#define VD_CLIPBOARD_FORMAT_MAX_TYPES 16

typedef struct SPICE_ATTR_PACKED VDAgentClipboard {
#if 0 /* VD_AGENT_CAP_CLIPBOARD_SELECTION */
    uint8_t selection;
    uint8_t __reserved[sizeof(uint32_t) - 1 * sizeof(uint8_t)];
#endif
    uint32_t type;
    uint8_t data[0];
} VDAgentClipboard;

enum {
    VD_AGENT_CLIPBOARD_NONE = 0,
    VD_AGENT_CLIPBOARD_UTF8_TEXT,
    VD_AGENT_CLIPBOARD_IMAGE_PNG,  /* All clients with image support should support this one */
    VD_AGENT_CLIPBOARD_IMAGE_BMP,  /* optional */
    VD_AGENT_CLIPBOARD_IMAGE_TIFF, /* optional */
    VD_AGENT_CLIPBOARD_IMAGE_JPG,  /* optional */
    /* identifies a list of absolute paths in phodav server
     * that is associated with the "org.spice-space.webdav.0" webdav channel;
     * the items are encoded in UTF-8 and separated by '\0';
     * the first item must be either "copy" or "cut" (without the quotes)
     * to indicate what action should be performed with the files that follow */
    VD_AGENT_CLIPBOARD_FILE_LIST,
};


//FIXME: extract format/type stuff to win_vdagent_common for use by windows\platform.cpp as well
typedef struct VDClipboardFormat {
    uint32_t format;
    uint32_t types[VD_CLIPBOARD_FORMAT_MAX_TYPES];
} VDClipboardFormat;

static const VDClipboardFormat clipboard_formats[] = {
    {CF_UNICODETEXT, {VD_AGENT_CLIPBOARD_UTF8_TEXT, 0}},
    //FIXME: support more image types
    {CF_DIB, {VD_AGENT_CLIPBOARD_IMAGE_PNG, VD_AGENT_CLIPBOARD_IMAGE_BMP, 0}},
    {CF_HDROP, {VD_AGENT_CLIPBOARD_FILE_LIST, 0}},
};

// 检查常用格式
const UINT Formats[] = {
	CF_UNICODETEXT,  // Unicode文本
	CF_TEXT,         // ANSI文本
	CF_DIB,         // 位图
	CF_BITMAP,      // 位图句柄
	CF_HDROP,       // 文件
	CF_ENHMETAFILE, // 增强型图元文件
	CF_METAFILEPICT // 图元文件
};

class ClipboardManage : public Singleton<ClipboardManage>
{
public:
    friend ClipboardManage;

    ClipboardManage();

    ~ClipboardManage();

    void Init();
    void UnInit();
    UINT GetClipboardFormat();
    uint8_t* GetClipboardImage(HANDLE clip_data, long& new_size);
	uint8_t * FromBitMap(const BITMAPINFO & bmp_info, const void * bits, long & size);
	BOOL SaveClipboardImageAsPNG(const char * filename);
private:
    HANDLE m_hClipboard;
	UINT m_iClipBoardType;
};

