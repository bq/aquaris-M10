/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _G3DKMD_MACRO_H_
#define _G3DKMD_MACRO_H_

#include <asm/io.h>

#include "sapphire_reg.h"
#include "g3dkmd_define.h"

#ifdef PATTERN_DUMP
#include "g3dkmd_pattern.h"
#endif

#ifdef G3DKMD_SUPPORT_MSGQ
#include "g3dkmd_msgq.h"
#endif

#if defined(G3D_HW)
#define REG_BASE(mod)   mod##_REG_BASE
#define REG_ACCESS_DATA_IMP(mod)
#else
#define REG_BASE(mod)   s##mod##Register
#define REG_ACCESS_DATA_IMP(mod)\
	static unsigned char s##mod##Register[mod##_REG_MAX]
#endif

#define MOD_OFST(mod, reg)              ((reg) + (mod##_MOD_OFST & 0xFFFFul))
#define REG_MOD_OFST(mod, reg)          ((reg) + ((mod##_REG_BASE - YL_BASE) & 0xFFFFul))


#ifdef PATTERN_DUMP
#ifdef G3D_HW
#define REG_WRITE(func) \
do { \
	if (!g3dKmdIsBypass(BYPASS_REG)) \
		func; \
} while (0)
#else
#define REG_WRITE(func) func
#endif
#define RIU_WRITE(mod, reg, value) \
do { \
	if (!g3dKmdIsBypass(BYPASS_RIU)) \
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", REG_MOD_OFST(mod, reg), value); \
} while (0)
#else
#define REG_WRITE(func) func
#define RIU_WRITE(mod, reg, value)
#endif

#ifdef G3DKMD_SUPPORT_MSGQ
#define MSGQ_SETREG(mod, reg, value) g3dKmdMsgQSetReg(REG_MOD_OFST(mod, reg), value)
#else
#define MSGQ_SETREG(mod, reg, value)
#endif

#ifdef SUPPORT_MPD
#define CM_RIU_WRITE(mod, reg, value) \
do { \
	if (!g3dKmdIsBypass(BYPASS_RIU)) \
		CMRiuCommand(REG_MOD_OFST(mod, (reg)), (value)); \
} while (0)
#else
#define CM_RIU_WRITE(mod, reg, value)
#endif

#ifdef G3DKMD_AREG_RAND_FAIL_WORKAROUND

#define REG_ACCESS_FUNC_IMP(mod)\
	static inline unsigned char mod##_HAL_READ8(unsigned int reg)\
	{\
		return *((volatile unsigned char *)(REG_BASE(mod)+reg));\
	} \
	static inline void mod##_HAL_WRITE8(unsigned int reg, unsigned char val)\
	{\
		(*((volatile unsigned char *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned short mod##_HAL_READ16(unsigned int reg)\
	{\
		return *((volatile unsigned short *)(REG_BASE(mod)+reg));\
	} \
	static inline void mod##_HAL_WRITE16(unsigned int reg, unsigned short val)\
	{\
		(*((volatile unsigned short *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned int mod##_HAL_READ32(unsigned int reg) \
	{ \
		unsigned int i = 0, val = 0, pre_val = 0xFFFFFFFF;\
		for (i = 0; i < 6; i++) {\
			val = (*((volatile unsigned int *)(REG_BASE(mod)+reg)));\
			if (val != pre_val)\
				i = 0;\
			pre_val = val;\
		} \
		return val;\
	} \
	static inline void mod##_HAL_WRITE32(unsigned int reg, unsigned int val)\
	{\
		(*((volatile unsigned int *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned char mod##_REG_READ8(unsigned int reg)\
	{\
		return mod##_HAL_READ8((reg));\
	} \
	static inline unsigned short mod##_REG_READ16(unsigned int reg)\
	{\
		return mod##_HAL_READ16((reg));\
	} \
	static inline unsigned int mod##_REG_READ32(unsigned int reg)\
	{\
		return mod##_HAL_READ32((reg));\
	} \
	static inline unsigned int mod##_REG_READ32_MSK(unsigned int reg, unsigned int msk)\
	{\
		return (unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(msk);\
	} \
	static inline unsigned int mod##_REG_READ32_MSK_SFT(unsigned int reg, unsigned int msk, unsigned int sft) \
	{\
		return (unsigned int)(((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(msk)) >> sft);\
	} \
	static inline void mod##_REG_WRITE8(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value);\
		CM_RIU_WRITE(mod, reg, value); REG_WRITE(mod##_HAL_WRITE8((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE16(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value);\
		CM_RIU_WRITE(mod, reg, value); REG_WRITE(mod##_HAL_WRITE16((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE32(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value);\
		CM_RIU_WRITE(mod, reg, value); REG_WRITE(mod##_HAL_WRITE32((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE32_MSK(unsigned int reg, unsigned int value, unsigned int msk)\
	{\
		mod##_REG_WRITE32(reg, ((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(~msk))\
			| (((unsigned int)value) & (unsigned int)(msk)));\
	} \
	static inline void mod##_REG_WRITE32_MSK_SFT(unsigned int reg, unsigned int value, unsigned int msk,\
		unsigned int sft)\
	{\
		mod##_REG_WRITE32(reg, ((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(~msk))\
		| (((unsigned int)value << sft) & (unsigned int)(msk)));\
	}
#else

#define REG_ACCESS_FUNC_IMP(mod)\
	static inline unsigned char mod##_HAL_READ8(unsigned int reg)\
	{\
		return *((volatile unsigned char *)(REG_BASE(mod)+reg));\
	} \
	static inline void mod##_HAL_WRITE8(unsigned int reg, unsigned char val)\
	{\
		(*((volatile unsigned char *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned short mod##_HAL_READ16(unsigned int reg)\
	{\
		return *((volatile unsigned short *)(REG_BASE(mod)+reg));\
	} \
	static inline void mod##_HAL_WRITE16(unsigned int reg, unsigned short val)\
	{\
		(*((volatile unsigned short *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned int mod##_HAL_READ32(unsigned int reg)\
	{\
		return *((volatile unsigned int *)(REG_BASE(mod)+reg));\
	} \
	static inline void mod##_HAL_WRITE32(unsigned int reg, unsigned int val)\
	{\
		(*((volatile unsigned int *)(REG_BASE(mod)+reg))) = val;\
	} \
	static inline unsigned char mod##_REG_READ8(unsigned int reg)\
	{\
		return mod##_HAL_READ8((reg));\
	} \
	static inline unsigned short mod##_REG_READ16(unsigned int reg)\
	{\
		return mod##_HAL_READ16((reg));\
	} \
	static inline unsigned int mod##_REG_READ32(unsigned int reg)\
	{\
		return mod##_HAL_READ32((reg));\
	} \
	static inline unsigned int mod##_REG_READ32_MSK(unsigned int reg, unsigned int msk)\
	{\
		return (unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(msk);\
	} \
	static inline unsigned int mod##_REG_READ32_MSK_SFT(unsigned int reg, unsigned int msk, unsigned int sft)\
	{\
		return (unsigned int)(((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(msk)) >> sft);\
	} \
	static inline void mod##_REG_WRITE8(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value); CM_RIU_WRITE(mod, reg, value);\
		REG_WRITE(mod##_HAL_WRITE8((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE16(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value); CM_RIU_WRITE(mod, reg, value);\
		REG_WRITE(mod##_HAL_WRITE16((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE32(unsigned int reg, unsigned int value)\
	{\
		RIU_WRITE(mod, reg, value); MSGQ_SETREG(mod, reg, value); CM_RIU_WRITE(mod, reg, value);\
		REG_WRITE(mod##_HAL_WRITE32((reg), (value)));\
	} \
	static inline void mod##_REG_WRITE32_MSK(unsigned int reg, unsigned int value, unsigned int msk)\
	{\
		mod##_REG_WRITE32(reg, ((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(~msk)) \
		| (((unsigned int)value) & (unsigned int)(msk)));\
	} \
	static inline void mod##_REG_WRITE32_MSK_SFT(unsigned int reg, unsigned int value, unsigned int msk, \
		unsigned int sft)\
	{\
		mod##_REG_WRITE32(reg, ((unsigned int)mod##_HAL_READ32((reg)) & (unsigned int)(~msk)) \
		| (((unsigned int)value << sft) & (unsigned int)(msk)));\
	}
#endif
/*
#ifndef UNUSED
    #ifdef __GNUC__
#define UNUSED(x)   x __attribute__((unused))
    #else
#define UNUSED(x)   x
    #endif
#endif
*/
#ifndef UNUSED
#define UNUSED(X)       ((void)(X))
#endif /* UNUSED */

#endif /* _G3DKMD_MACRO_H_ */
