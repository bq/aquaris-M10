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

/*
 * Copyright (c) 2007  GPL Project Developer Who Made Changes <gpl@example.org>
 *
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *	 Copyright (c) 2010-2012 Ivan Voras <ivoras@freebsd.org>
 *	 Copyright (c) 2012 Tim Hartrick <tim@edgecast.com>
 *
 *	 Redistribution and use in source and binary forms, with or without
 *	 modification, are permitted provided that the following conditions
 *	 are met:
 *	 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 *	 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *
 *	 THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *	 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *	 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	 SUCH DAMAGE.
 */

#ifndef _GPAD_IPEM_LIB_H
#define _GPAD_IPEM_LIB_H
#include <linux/types.h>
#include "gs_gpufreq.h"



/*
 * *******************************************************************
 *  type
 * *******************************************************************
 */
typedef int32_t							gpad_ipem_fixedpt;
typedef int64_t							gpad_ipem_fixedptd;
typedef uint32_t						gpad_ipem_fixedptu;
typedef uint64_t						gpad_ipem_fixedptud;
/* TODO: port it to 64bits, it would libgcc.a */
/* typedef int64_t						gpad_ipem_fixedpt;
 * typedef __int128_t						gpad_ipem_fixedptd;
 * typedef uint64_t						gpad_ipem_fixedptu;
 * typedef __uint128_t						gpad_ipem_fixedptud;
 */


typedef u32 gpad_ipem_state;
typedef u32 gpad_ipem_action;
typedef u32 gpad_ipem_stateaction;

/* TODO: is it possile to fix this? */
struct gpad_device;
struct gpad_ipem_work_info;
typedef void				(*gpad_ipem_policy_init_func)(struct gpad_device *);
typedef void				(*gpad_ipem_policy_exit_func)(struct gpad_device *);
typedef gpad_ipem_action(*gpad_ipem_policy_do_func)(struct gpad_ipem_work_info *);
typedef void				(*gpad_ipem_policy_update_func)(struct gpad_ipem_work_info *);




/*
 * *******************************************************************
 *  macro
 * *******************************************************************
 */
#define GPAD_IPEM_FIXEDPT_BITS					32
#define GPAD_IPEM_FIXEDPT_WBITS					24

#define GPAD_IPEM_FIXEDPT_FBITS					(GPAD_IPEM_FIXEDPT_BITS - GPAD_IPEM_FIXEDPT_WBITS)
#define GPAD_IPEM_FIXEDPT_FMASK					(((gpad_ipem_fixedpt)1 << GPAD_IPEM_FIXEDPT_FBITS) - 1)

#define gpad_ipem_fixedpt_rconst(R)				((gpad_ipem_fixedpt)((R) * GPAD_IPEM_FIXEDPT_ONE + ((R) >= 0 ? 0.5 : -0.5)))
#define gpad_ipem_fixedpt_fromint(I)			((gpad_ipem_fixedpt)(I) << GPAD_IPEM_FIXEDPT_FBITS)
#define gpad_ipem_fixedpt_toint(F)				((F) >> GPAD_IPEM_FIXEDPT_FBITS)
#define gpad_ipem_fixedpt_add(A, B)				((A) + (B))
#define gpad_ipem_fixedpt_sub(A, B)				((A) - (B))
#define gpad_ipem_fixedpt_xmul(A, B)			((gpad_ipem_fixedpt)(((gpad_ipem_fixedptd)(A) * (gpad_ipem_fixedptd)(B)) >> GPAD_IPEM_FIXEDPT_FBITS))
#define gpad_ipem_fixedpt_xdiv(A, B)			((gpad_ipem_fixedpt)(((gpad_ipem_fixedptd)(A) << GPAD_IPEM_FIXEDPT_FBITS) / (gpad_ipem_fixedptd)(B)))
#define gpad_ipem_fixedpt_abs(A)				((A) < 0 ? -(A) : (A))
#define gpad_ipem_fixedpt_fracpart(A)				((gpad_ipem_fixedpt)(A) & GPAD_IPEM_FIXEDPT_FMASK)

#define GPAD_IPEM_FIXEDPT_ONE					((gpad_ipem_fixedpt)((gpad_ipem_fixedpt)1 << GPAD_IPEM_FIXEDPT_FBITS))
#define GPAD_IPEM_FIXEDPT_ONE_HALF				(GPAD_IPEM_FIXEDPT_ONE >> 1)
#define GPAD_IPEM_FIXEDPT_TWO					(GPAD_IPEM_FIXEDPT_ONE + GPAD_IPEM_FIXEDPT_ONE)
#define GPAD_IPEM_FIXEDPT_PI					gpad_ipem_fixedpt_rconst(3.14159265358979323846)
#define GPAD_IPEM_FIXEDPT_TWO_PI				gpad_ipem_fixedpt_rconst(2 * 3.14159265358979323846)
#define GPAD_IPEM_FIXEDPT_HALF_PI				gpad_ipem_fixedpt_rconst(3.14159265358979323846 / 2)
#define GPAD_IPEM_FIXEDPT_E					gpad_ipem_fixedpt_rconst(2.7182818284590452354)

/* Shorthand functions */
#define GPAD_IPEM_INFO_GET(_dev)				(&((_dev)->ipem_info))




/*
 * *******************************************************************
 *  enumeration
 * *******************************************************************
 */




/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_mul(gpad_ipem_fixedpt A, gpad_ipem_fixedpt B);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_div(gpad_ipem_fixedpt A, gpad_ipem_fixedpt B);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_sqrt(gpad_ipem_fixedpt A);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_sin(gpad_ipem_fixedpt fp);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_cos(gpad_ipem_fixedpt A);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_tan(gpad_ipem_fixedpt A);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_exp(gpad_ipem_fixedpt fp);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_ln(gpad_ipem_fixedpt x);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_log(gpad_ipem_fixedpt x, gpad_ipem_fixedpt base);
extern gpad_ipem_fixedpt gpad_ipem_fixedpt_pow(gpad_ipem_fixedpt n, gpad_ipem_fixedpt exp);
extern void gpad_ipem_fixedpt_str(gpad_ipem_fixedpt A, char *str, s32 max_dec);
extern char *gpad_ipem_fixedpt_cstr(const gpad_ipem_fixedpt A);

/* TODO, make me _inline_ */
extern void gpad_ipem_stateaction_enc(gpad_ipem_stateaction *stateaction, gpad_ipem_state state, gpad_ipem_action action);
extern void gpad_ipem_stateaction_dec(gpad_ipem_stateaction stateaction, gpad_ipem_state *state, gpad_ipem_action *action);

extern gpad_ipem_action gpad_ipem_tsearch_do(struct mtk_gpufreq_info *table, gpad_ipem_action action_llimit, gpad_ipem_action action_ulimit, u32 gpuload);
extern gpad_ipem_action gpad_ipem_qmax_do(gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end);
extern gpad_ipem_action gpad_ipem_egreedy_do(gpad_ipem_fixedpt epsilon, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end);
extern gpad_ipem_action gpad_ipem_softmax_do(gpad_ipem_fixedpt tau, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end);
extern gpad_ipem_action gpad_ipem_vdbe_do(gpad_ipem_fixedpt *epsilons, gpad_ipem_fixedpt tau, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end);
extern void gpad_ipem_vdbe_update(gpad_ipem_fixedpt *epsilons, gpad_ipem_fixedpt delta, gpad_ipem_fixedpt sigma, gpad_ipem_fixedpt q_delta, gpad_ipem_state state);




#endif /* #define _GPAD_IPEM_LIB_H */
