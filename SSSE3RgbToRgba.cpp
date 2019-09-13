//-----------------------------------------------------------------------
//
// This file is part of the SimpleV4L2 Project
//
//  by Amos Tibaldi - tibaldi at users.sourceforge.net
//
// http://sourceforge.net/projects/simplev4l2/
//
// http://simplev4l2.sourceforge.net/
//
//
// COPYRIGHT: http://www.gnu.org/licenses/gpl.html
//            COPYRIGHT-gpl-3.0.txt
//
//     The SimpleV4L2 Project
//         Simple V4L2 Grabber and RGB Buffer SSSE3 Optimized Video Player
//
//     Copyright (C) 2013 Amos Tibaldi
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

#include <tmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __SSSE3__
#error "SSSE3 required..."
#endif

    void print128_num(__m128i var)
    {
        uint8_t *val = (uint8_t*) &var;
        printf("__m128i: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
               val[15], val[14], val[13], val[12], val[11], val[10], val[9], val[8],  val[7], val[6], val[5], val[4], val[3], val[2], val[1], val[0]);
    }

    __attribute__ ((aligned (16)))uint8_t U128I01[16] = { 0x00, 0x01, 0x02, 0xF0, 0x03, 0x04, 0x05, 0xF0, 0x06, 0x07, 0x08, 0xF0, 0x09, 0x0A, 0x0B, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf01 = _mm_load_si128((__m128i*) &U128I01);
    __attribute__ ((aligned (16)))uint8_t U128I02[16] = { 0x0C, 0x0D, 0x0E, 0xF0, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf02 = _mm_load_si128((__m128i*) &U128I02);
    __attribute__ ((aligned (16)))uint8_t U128I03[16] = { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x01, 0xF0, 0x02, 0x03, 0x04, 0xF0, 0x05, 0x06, 0x07, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf03 = _mm_load_si128((__m128i*) &U128I03);
    __attribute__ ((aligned (16)))uint8_t U128I04[16] = { 0x08, 0x09, 0x0A, 0xF0, 0x0B, 0x0C, 0x0D, 0xF0, 0x0E, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf04 = _mm_load_si128((__m128i*) &U128I04);
    __attribute__ ((aligned (16)))uint8_t U128I05[16] = { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0xF0, 0x01, 0x02, 0x03, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf05 = _mm_load_si128((__m128i*) &U128I05);
    __attribute__ ((aligned (16)))uint8_t U128I06[16] = { 0x04, 0x05, 0x06, 0xF0, 0x07, 0x08, 0x09, 0xF0, 0x0A, 0x0B, 0x0C, 0xF0, 0x0D, 0x0E, 0x0F, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf06 = _mm_load_si128((__m128i*) &U128I06);
    __attribute__ ((aligned (16)))uint8_t U128I07[16] = { 0x00, 0x01, 0x02, 0xF0, 0x03, 0x04, 0x05, 0xF0, 0x06, 0x07, 0x08, 0xF0, 0x09, 0x0A, 0x0B, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf07 = _mm_load_si128((__m128i*) &U128I07);
    __attribute__ ((aligned (16)))uint8_t U128I08[16] = { 0x0C, 0x0D, 0x0E, 0xF0, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf08 = _mm_load_si128((__m128i*) &U128I08);
    __attribute__ ((aligned (16)))uint8_t U128I09[16] = { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x01, 0xF0, 0x02, 0x03, 0x04, 0xF0, 0x05, 0x06, 0x07, 0xF0 };
    __attribute__ ((aligned (16)))__m128i Xmm_Shuf09 = _mm_load_si128((__m128i*) &U128I09);

    void SSSE3RgbToRgba(int xPixSize, int yPixSize, unsigned char * MAlign16RGBBuffer, unsigned char * MAlign16RGBABuffer)
    {
        __m128i * psrc = (__m128i*)MAlign16RGBBuffer;
        __m128i * pdst = (__m128i*)MAlign16RGBABuffer;
        unsigned int addingToSrcPointer = (xPixSize*yPixSize*3)/16;
        __m128i * pLastSrc = psrc + addingToSrcPointer - 1;
        __m128i Xmm_A, Xmm_B, Xmm_C;

        for(; true; )
        {
            Xmm_A = _mm_load_si128(psrc);
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf01);
            _mm_store_si128(pdst, Xmm_B);
            pdst++;
            Xmm_C = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf02);
            psrc++;
            Xmm_A = _mm_load_si128(psrc);
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf03);
again_HISPEED_RGBTORGBA:
            Xmm_B = _mm_or_si128(Xmm_B, Xmm_C);
            _mm_store_si128(pdst, Xmm_B);
            pdst++;
            Xmm_C = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf04);
            if(psrc == pLastSrc) {
                goto end_HISPEED_RGBTORGBA;
            }
            psrc++;
            Xmm_A = _mm_loadu_si128(psrc);
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf05);
            Xmm_B = _mm_or_si128(Xmm_B, Xmm_C);
            _mm_store_si128(pdst, Xmm_B);
            pdst++;
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf06);
            _mm_store_si128(pdst, Xmm_B);
            pdst++;
            if(psrc == pLastSrc) {
                goto end_HISPEED_RGBTORGBA;
            }
            psrc++;
            Xmm_A = _mm_load_si128(psrc);
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf07);
            _mm_store_si128(pdst, Xmm_B);
            pdst++;
            Xmm_C = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf08);
            if(psrc == pLastSrc) {
                goto end_HISPEED_RGBTORGBA;
            }
            psrc++;
            Xmm_A = _mm_load_si128(psrc);
            Xmm_B = _mm_shuffle_epi8(Xmm_A, Xmm_Shuf09);
            goto again_HISPEED_RGBTORGBA;
        }

end_HISPEED_RGBTORGBA:
	int a = 1;
       // _mm_store_si128(pdst, Xmm_C);

    }

#ifdef __cplusplus
}
#endif



