#include "ClipboardManage.h"
#include <vector>
#include <png.h>
ClipboardManage::ClipboardManage()
{

}

ClipboardManage::~ClipboardManage()
{

}

void ClipboardManage::Init()
{
	UINT iType = GetClipboardFormat();
	uint8_t* Clipboard_Data = NULL;
	size_t Len = 0;
	long NewSize = 0;
	uint8_t* NewData = NULL;
    HDROP hDrop = NULL;
    UINT nFiles = 0;
	if (!IsClipboardFormatAvailable(iType) || !OpenClipboard(NULL)) 
	{
        return;
    }

	if (iType == CF_DIB) 
    {
		iType = CF_BITMAP;
	}

    //获取当前剪贴板数据
	m_hClipboard = GetClipboardData(iType);
	if (m_hClipboard == NULL)
	{
        CloseClipboard();
		return;
	}

	switch (iType)
	{
	case CF_UNICODETEXT:
		if (!(Clipboard_Data = (uint8_t*)GlobalLock(m_hClipboard))) 
		{
			break;
		}
		Len = wcslen((LPCWSTR)Clipboard_Data);
		NewSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)Clipboard_Data, (int)Len, NULL, 0, NULL, NULL);
		GlobalUnlock(Clipboard_Data);
		break;
	case CF_TEXT:
		break;
    case CF_DIB:
	case CF_BITMAP:
		NewData = GetClipboardImage(m_hClipboard, NewSize);
		break;
	case CF_HDROP:
		if (!(Clipboard_Data = (uint8_t*)GlobalLock(m_hClipboard)))
		{
			break;
		}

		hDrop = (HDROP)Clipboard_Data;

		// 获取文件数量
		nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

        for (UINT i = 0; i < nFiles; ++i)
        {
            // 获取文件名
            UINT len = DragQueryFile(hDrop, i, NULL, 0);
            if (len > 0)
            {
                TCHAR FileName[MAX_PATH];
                DragQueryFile(hDrop, i, FileName, len + 1);

				 DWORD attributes = GetFileAttributes(FileName);
                 if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
                 {
                     //当前是文件夹

                 }
                // 输出文件名
                wprintf(L"%s\n", FileName);
            }
        }

		break;
    case CF_ENHMETAFILE:
		break;
	case CF_METAFILEPICT:
		break;
	default:
		break;
	}

	CloseClipboard();
    if (NewData)
    {
        free(NewData);
    }

    return;
}

void ClipboardManage::UnInit()
{
}



uint8_t* ClipboardManage::GetClipboardImage(HANDLE clip_data, long& new_size)
{
	HPALETTE pal = 0;
    if (IsClipboardFormatAvailable(CF_PALETTE)) 
	{
        pal = (HPALETTE)GetClipboardData(CF_PALETTE);
    }

	BITMAP bitmap;
	GetObject(clip_data, sizeof(bitmap), &bitmap);

	struct {
		BITMAPINFOHEADER head;
		RGBQUAD colors[256];
	} info;

	BITMAPINFOHEADER& head(info.head);
	memset(&head, 0, sizeof(head));
	head.biSize = sizeof(head);
	head.biWidth = bitmap.bmWidth;
	head.biHeight = bitmap.bmHeight;
	head.biPlanes = bitmap.bmPlanes;
	head.biBitCount = bitmap.bmBitsPixel >= 16 ? 24 : bitmap.bmBitsPixel;
	head.biCompression = BI_RGB;

	HDC dc = GetDC(NULL);
	HPALETTE old_pal = NULL;
	if (pal) 
	{
		old_pal = (HPALETTE)SelectObject(dc, pal);
		RealizePalette(dc);
	}
	
	size_t stride = (((head.biWidth * head.biBitCount + 31u) & ~31u) / 8u);
	std::vector<uint8_t> bits(stride* head.biHeight);
	int res = GetDIBits(dc, (HBITMAP)clip_data, 0, head.biHeight,&bits[0], (LPBITMAPINFO)&info, DIB_RGB_COLORS);
	if (pal) 
	{
		SelectObject(dc, old_pal);
	}
	ReleaseDC(NULL, dc);
	if (!res) 
	{
		return NULL;
	}

    return FromBitMap(*(LPBITMAPINFO)&info, &bits[0], new_size);
}

struct WriteBufferIo {
    uint8_t *buf;
    uint32_t pos, size;
    WriteBufferIo():
        buf(NULL), pos(0), size(0)
    {}
    ~WriteBufferIo() { free(buf); }
    uint8_t *release() {
        uint8_t *res = buf;
        buf = NULL;
        pos = size = 0;
        return res;
    }
};

static void write_to_bufio(png_structp png, png_bytep in, png_size_t size)
{
    WriteBufferIo& io(*(WriteBufferIo*)png_get_io_ptr(png));
    if (io.pos + size > io.size) {
        uint32_t new_size = io.size ? io.size * 2 : 4096;
        while (io.pos + size >= new_size) {
            new_size *= 2;
        }
        uint8_t *p = (uint8_t*) realloc(io.buf, new_size);
        if (!p)
            png_error(png, "out of memory");
        io.buf = p;
        io.size = new_size;
    }
    memcpy(io.buf+io.pos, in, size);
    io.pos += size;
}

static void flush_bufio(png_structp png)
{
}

uint8_t* ClipboardManage::FromBitMap(const BITMAPINFO& bmp_info, const void* bits, long& size)
{
	std::vector<png_color> palette;
    WriteBufferIo io;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        return 0;

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, &info);
        return 0;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        return 0;
    }

    png_set_write_fn(png, &io, write_to_bufio, flush_bufio);

    const BITMAPINFOHEADER& head(bmp_info.bmiHeader);
    int color_type;
    int out_bits = head.biBitCount;
    switch (out_bits) {
    case 1:
    case 4:
    case 8:
        color_type = PNG_COLOR_TYPE_PALETTE;
        break;
    case 24:
    case 32:
        png_set_bgr(png);
        color_type = PNG_COLOR_TYPE_RGB;
        break;
    default:
        png_error(png, "BMP bit count not supported");
        break;
    }
    // TODO detect gray
    png_set_IHDR(png, info, head.biWidth, head.biHeight,
                 out_bits > 8 ? 8 : out_bits, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    // palette
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        palette.resize(head.biClrUsed);
        const RGBQUAD *rgb = bmp_info.bmiColors;
        for (unsigned int color = 0; color < head.biClrUsed; ++color) {
            palette[color].red = rgb->rgbRed;
            palette[color].green = rgb->rgbGreen;
            palette[color].blue = rgb->rgbBlue;
            ++rgb;
        }
        png_set_PLTE(png, info, &palette[0], palette.size());
    }

    png_write_info(png, info);

    const unsigned int width = head.biWidth;
    const unsigned int height = head.biHeight;
    const size_t stride = ((width * out_bits + 31u) & ~31u) / 8u;
    const size_t image_size = stride * height;

    // now do the actual conversion!
    const uint8_t *src = (const uint8_t*)bits + image_size;
    for (unsigned int row = 0; row < height; ++row) {
        src -= stride;
        png_write_row(png, src);
    }
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &info);
    size = io.pos;
    return io.release();


}
// 获取剪贴板格式的辅助函数
UINT ClipboardManage::GetClipboardFormat()
{
	if (!OpenClipboard(NULL)) 
    {
        return 0;
    }

    int result = GetPriorityClipboardFormat((UINT*)Formats, ARRAYSIZE(Formats));

	CloseClipboard();

	//-1: 没有找到任何格式  0: 没找到匹配的格式 
	return (result > 0) ? result : 0;

}


BOOL ClipboardManage::SaveClipboardImageAsPNG(const char* filename) 
{
    BOOL result = FALSE;
    FILE* fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte** row_pointers = NULL;
    png_byte* temp_row = NULL;
    int stride = 0;
    
    if (!OpenClipboard(NULL)) {
        return FALSE;
    }

    HANDLE hClipboard = GetClipboardData(CF_DIB);
    if (!hClipboard) {
        CloseClipboard();
        return FALSE;
    }

    LPVOID pData = GlobalLock(hClipboard);
    if (!pData) {
        CloseClipboard();
        return FALSE;
    }

    BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;
    int width = bih->biWidth;
    int height = abs(bih->biHeight);
    int bpp = bih->biBitCount;

    // 获取调色板和像素数据
    RGBQUAD* palette = NULL;
    BYTE* pixels = (BYTE*)pData + sizeof(BITMAPINFOHEADER);
    if (bpp <= 8) {
        int colors = bih->biClrUsed ? bih->biClrUsed : 1 << bpp;
        palette = (RGBQUAD*)pixels;
        pixels += colors * sizeof(RGBQUAD);
    }

    fopen_s(&fp, filename, "wb");
    if (!fp) goto cleanup;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) goto cleanup;
    
    png_set_compression_level(png_ptr, 9);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) goto cleanup;

    if (setjmp(png_jmpbuf(png_ptr))) goto cleanup;

    png_init_io(png_ptr, fp);

    // 根据位深度选择合适的颜色类型
    int color_type;
    if (bpp == 32) {
        color_type = PNG_COLOR_TYPE_RGBA;
    } else if (bpp == 24) {
        color_type = PNG_COLOR_TYPE_RGB;
    } else if (bpp <= 8) {
        color_type = PNG_COLOR_TYPE_PALETTE;
    } else {
        color_type = PNG_COLOR_TYPE_RGB;
    }

    // 设置PNG头信息
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8,  // 位深度
                 color_type,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    // 设置调色板（如果需要）
    if (bpp <= 8 && palette) {
        png_color png_palette[256];
        int colors = bih->biClrUsed ? bih->biClrUsed : 1 << bpp;
        for (int i = 0; i < colors; i++) {
            png_palette[i].red = palette[i].rgbRed;
            png_palette[i].green = palette[i].rgbGreen;
            png_palette[i].blue = palette[i].rgbBlue;
        }
        png_set_PLTE(png_ptr, info_ptr, png_palette, colors);
    }

    // 写入PNG头信息
    png_write_info(png_ptr, info_ptr);

    // 分配行指针数组
    row_pointers = (png_byte**)malloc(sizeof(png_byte*) * height);
    if (!row_pointers) goto cleanup;

    // 计算stride
    stride = ((width * bpp + 31) & ~31) >> 3;

    // 处理图像数据
    if (bpp == 32 || bpp == 24) {
        int dst_stride = (bpp == 32) ? width * 4 : width * 3;
        temp_row = (png_byte*)malloc(height * dst_stride);  // 为所有行分配内存
        if (!temp_row) goto cleanup;

        for (int y = 0; y < height; y++) {
            BYTE* src = pixels + (height - 1 - y) * stride;
            BYTE* dst = temp_row + y * dst_stride;
            
            if (bpp == 32) {
                // BGRA to RGBA
                for (int x = 0; x < width; x++) {
                    dst[x * 4 + 0] = src[x * 4 + 2]; // R
                    dst[x * 4 + 1] = src[x * 4 + 1]; // G
                    dst[x * 4 + 2] = src[x * 4 + 0]; // B
                    dst[x * 4 + 3] = src[x * 4 + 3]; // A
                }
            } else { // bpp == 24
                // BGR to RGB
                for (int x = 0; x < width; x++) {
                    dst[x * 3 + 0] = src[x * 3 + 2]; // R
                    dst[x * 3 + 1] = src[x * 3 + 1]; // G
                    dst[x * 3 + 2] = src[x * 3 + 0]; // B
                }
            }
            row_pointers[y] = dst;
        }
    } else {
        // 对于索引色图像，直接使用原始数据
        for (int y = 0; y < height; y++) {
            row_pointers[y] = pixels + (height - 1 - y) * stride;
        }
    }

    // 写入所有图像数据
    png_write_image(png_ptr, row_pointers);

    // 结束写入
    png_write_end(png_ptr, NULL);

    result = TRUE;

cleanup:
    if (temp_row) {
        free(temp_row);
    }
    if (row_pointers) {
        free(row_pointers);
    }
    if (png_ptr) {
        if (info_ptr) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
        } else {
            png_destroy_write_struct(&png_ptr, NULL);
        }
    }
    if (fp) {
        fclose(fp);
    }
    GlobalUnlock(hClipboard);
    CloseClipboard();
    return result;
}