//#include "precompile.h"
// sha1.c
/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "scheme.h"
#ifdef WIN32
#ifdef __OS_KERNEL__
#include <ntifs.h>
#endif

#endif
#include <string.h>

#include "sha1.h"

#define GET_UINT32(n,b,i)                       \
{                                               \
    (n) = ( (uint32) (b)[(i)    ] << 24 )       \
        | ( (uint32) (b)[(i) + 1] << 16 )       \
        | ( (uint32) (b)[(i) + 2] <<  8 )       \
        | ( (uint32) (b)[(i) + 3]       );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
    (b)[(i)    ] = (uint8) ( (n) >> 24 );       \
    (b)[(i) + 1] = (uint8) ( (n) >> 16 );       \
    (b)[(i) + 2] = (uint8) ( (n) >>  8 );       \
    (b)[(i) + 3] = (uint8) ( (n)       );       \
}

void sha1_starts(sha1_context* ctx)
{
	ctx->total[0] = 0;
	ctx->total[1] = 0;

	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xC3D2E1F0;
}

void sha1_process(sha1_context* ctx, uint8 data[64])
{
	uint32 temp, W[16], A, B, C, D, E;

	GET_UINT32(W[0], data, 0);
	GET_UINT32(W[1], data, 4);
	GET_UINT32(W[2], data, 8);
	GET_UINT32(W[3], data, 12);
	GET_UINT32(W[4], data, 16);
	GET_UINT32(W[5], data, 20);
	GET_UINT32(W[6], data, 24);
	GET_UINT32(W[7], data, 28);
	GET_UINT32(W[8], data, 32);
	GET_UINT32(W[9], data, 36);
	GET_UINT32(W[10], data, 40);
	GET_UINT32(W[11], data, 44);
	GET_UINT32(W[12], data, 48);
	GET_UINT32(W[13], data, 52);
	GET_UINT32(W[14], data, 56);
	GET_UINT32(W[15], data, 60);

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
           W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

	P(A, B, C, D, E, W[0]);
	P(E, A, B, C, D, W[1]);
	P(D, E, A, B, C, W[2]);
	P(C, D, E, A, B, W[3]);
	P(B, C, D, E, A, W[4]);
	P(A, B, C, D, E, W[5]);
	P(E, A, B, C, D, W[6]);
	P(D, E, A, B, C, W[7]);
	P(C, D, E, A, B, W[8]);
	P(B, C, D, E, A, W[9]);
	P(A, B, C, D, E, W[10]);
	P(E, A, B, C, D, W[11]);
	P(D, E, A, B, C, W[12]);
	P(C, D, E, A, B, W[13]);
	P(B, C, D, E, A, W[14]);
	P(A, B, C, D, E, W[15]);
	P(E, A, B, C, D, R(16));
	P(D, E, A, B, C, R(17));
	P(C, D, E, A, B, R(18));
	P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

	P(A, B, C, D, E, R(20));
	P(E, A, B, C, D, R(21));
	P(D, E, A, B, C, R(22));
	P(C, D, E, A, B, R(23));
	P(B, C, D, E, A, R(24));
	P(A, B, C, D, E, R(25));
	P(E, A, B, C, D, R(26));
	P(D, E, A, B, C, R(27));
	P(C, D, E, A, B, R(28));
	P(B, C, D, E, A, R(29));
	P(A, B, C, D, E, R(30));
	P(E, A, B, C, D, R(31));
	P(D, E, A, B, C, R(32));
	P(C, D, E, A, B, R(33));
	P(B, C, D, E, A, R(34));
	P(A, B, C, D, E, R(35));
	P(E, A, B, C, D, R(36));
	P(D, E, A, B, C, R(37));
	P(C, D, E, A, B, R(38));
	P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

	P(A, B, C, D, E, R(40));
	P(E, A, B, C, D, R(41));
	P(D, E, A, B, C, R(42));
	P(C, D, E, A, B, R(43));
	P(B, C, D, E, A, R(44));
	P(A, B, C, D, E, R(45));
	P(E, A, B, C, D, R(46));
	P(D, E, A, B, C, R(47));
	P(C, D, E, A, B, R(48));
	P(B, C, D, E, A, R(49));
	P(A, B, C, D, E, R(50));
	P(E, A, B, C, D, R(51));
	P(D, E, A, B, C, R(52));
	P(C, D, E, A, B, R(53));
	P(B, C, D, E, A, R(54));
	P(A, B, C, D, E, R(55));
	P(E, A, B, C, D, R(56));
	P(D, E, A, B, C, R(57));
	P(C, D, E, A, B, R(58));
	P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

	P(A, B, C, D, E, R(60));
	P(E, A, B, C, D, R(61));
	P(D, E, A, B, C, R(62));
	P(C, D, E, A, B, R(63));
	P(B, C, D, E, A, R(64));
	P(A, B, C, D, E, R(65));
	P(E, A, B, C, D, R(66));
	P(D, E, A, B, C, R(67));
	P(C, D, E, A, B, R(68));
	P(B, C, D, E, A, R(69));
	P(A, B, C, D, E, R(70));
	P(E, A, B, C, D, R(71));
	P(D, E, A, B, C, R(72));
	P(C, D, E, A, B, R(73));
	P(B, C, D, E, A, R(74));
	P(A, B, C, D, E, R(75));
	P(E, A, B, C, D, R(76));
	P(D, E, A, B, C, R(77));
	P(C, D, E, A, B, R(78));
	P(B, C, D, E, A, R(79));

#undef K
#undef F

	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
	ctx->state[4] += E;
}

void sha1_update(sha1_context* ctx, uint8* input, uint32 length)
{
	uint32 left, fill;

	if (!length) return;

	left = ctx->total[0] & 0x3F;
	fill = 64 - left;

	ctx->total[0] += length;
	ctx->total[0] &= 0xFFFFFFFF;

	if (ctx->total[0] < length)
		ctx->total[1]++;

	if (left && length >= fill)
	{
		memcpy((void*)(ctx->buffer + left),
			(void*)input, fill);
		sha1_process(ctx, ctx->buffer);
		length -= fill;
		input += fill;
		left = 0;
	}

	while (length >= 64)
	{
		sha1_process(ctx, input);
		length -= 64;
		input += 64;
	}

	if (length)
	{
		memcpy((void*)(ctx->buffer + left),
			(void*)input, length);
	}
}

static uint8 sha1_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sha1_finish(sha1_context* ctx, uint8 digest[20])
{
	uint32 last, padn;
	uint32 high, low;
	uint8 msglen[8];

	high = (ctx->total[0] >> 29)
		| (ctx->total[1] << 3);
	low = (ctx->total[0] << 3);

	PUT_UINT32(high, msglen, 0);
	PUT_UINT32(low, msglen, 4);

	last = ctx->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	sha1_update(ctx, sha1_padding, padn);
	sha1_update(ctx, msglen, 8);

	PUT_UINT32(ctx->state[0], digest, 0);
	PUT_UINT32(ctx->state[1], digest, 4);
	PUT_UINT32(ctx->state[2], digest, 8);
	PUT_UINT32(ctx->state[3], digest, 12);
	PUT_UINT32(ctx->state[4], digest, 16);
}

#ifdef TEST

#include <stdlib.h>
#include <stdio.h>

/*
 * those are the standard FIPS-180-1 test vectors
 */

static char* msg[] =
{
	"abc",
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	NULL
};

static char* val[] =
{
	"a9993e364706816aba3e25717850c26c9cd0d89d",
	"84983e441c3bd26ebaae4aa1f95129e5e54670f1",
	"34aa973cd4c4daa4f61eeb2bdbad27316534016f"
};

int main(int argc, char* argv[])
{
	FILE* f;
	int i, j;
	char output[41];
	sha1_context ctx;
	unsigned char buf[1000];
	unsigned char sha1sum[20];

	if (argc < 2)
	{
		printf("\n SHA-1 Validation Tests:\n\n");

		for (i = 0; i < 3; i++)
		{
			printf(" Test %d ", i + 1);

			sha1_starts(&ctx);

			if (i < 2)
			{
				sha1_update(&ctx, (uint8*)msg[i],
					strlen(msg[i]));
			}
			else
			{
				memset(buf, 'a', 1000);

				for (j = 0; j < 1000; j++)
				{
					sha1_update(&ctx, (uint8*)buf, 1000);
				}
			}

			sha1_finish(&ctx, sha1sum);

			for (j = 0; j < 20; j++)
			{
				sprintf(output + j * 2, "%02x", sha1sum[j]);
			}

			if (memcmp(output, val[i], 40))
			{
				printf("failed!\n");
				return(1);
			}

			printf("passed.\n");
		}

		printf("\n");
	}
	else
	{
		if (!(f = fopen(argv[1], "rb")))
		{
			perror("fopen");
			return(1);
		}

		sha1_starts(&ctx);

		while ((i = fread(buf, 1, sizeof(buf), f)) > 0)
		{
			sha1_update(&ctx, buf, i);
		}

		sha1_finish(&ctx, sha1sum);

		for (j = 0; j < 20; j++)
		{
			printf("%02x", sha1sum[j]);
		}

		printf("  %s\n", argv[1]);
	}

	return(0);
}

#endif