/*
SHA-1 Source Code

This optimized SHA-1 implementation conforms to FIPS-180-1.

sha1.h  */
#ifndef _SHA1_H
#define _SHA1_H

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif
#pragma pack(1)
typedef struct
{
	uint32 total[2];
	uint32 state[5];
	uint8 buffer[64];
} sha1_context;

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

	void sha1_starts(sha1_context* ctx);
	void sha1_update(sha1_context* ctx, uint8* input, uint32 length);
	void sha1_finish(sha1_context* ctx, uint8 digest[20]);

#ifdef __cplusplus
}
#endif

#endif /* sha1.h */
