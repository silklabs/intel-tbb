/*
    Copyright 2005-2013 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

// Header that sets HAVE_tsx if TSX is available
#define HAVE_TSX ( __TBB_x86_32 || __TBB_x86_64 )

#if HAVE_TSX 

// TODO: extend it for other compilers when we add API for XTEST
#if __INTEL_COMPILER

#include "harness_defs.h"

inline static bool IsInsideTx()
{
#if _MSC_VER
    __int8 res = 0;
    __asm {
      _asm _emit 0x0F 
      _asm _emit 0x01 
      _asm _emit 0xD6
      _asm setz ah
      _asm mov  res, ah
    }
    return res==0;
#else
    int8_t res = 0;
    __asm__ __volatile__ (".byte 0x0F; .byte 0x01; .byte 0xD6;\n"
                          "setz %0" : "=r"(res) : : "memory" );
#endif
    return res==0;
}

#if _MSC_VER
#include <intrin.h> // for __cpuid
#endif
bool have_TSX() {
    bool result = false;
    const int hle_ebx_mask = 1<<4;
    const int rtm_ebx_mask = 1<<11;
#if _MSC_VER
    int info[4] = {0,0,0,0};
    const int EBX = 1;
    __cpuidex(info, 7, 0);
    result = (info[EBX] & hle_ebx_mask)!=0;
    if( result ) ASSERT( (info[EBX] & rtm_ebx_mask)!=0, NULL );
#elif __GNUC__
    int EBX = 0;
    int32_t reg_eax = 7;
    int32_t reg_ecx = 0;
    __asm__ __volatile__ ( "movl %%ebx, %%esi\n"
                           "cpuid\n"
                           "movl %%ebx, %0\n"
                           "movl %%esi, %%ebx\n"
                           : "=a"(EBX) : "0" (reg_eax), "c" (reg_ecx) : "esi" );
    result = (EBX & hle_ebx_mask)!=0 ;
    if( result ) ASSERT( (EBX & rtm_ebx_mask)!=0, NULL );
#endif
    return result;
}

#endif /* __INTEL_COMPILER */

#endif /* HAVE_TSX */
