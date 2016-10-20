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

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/random.h>
#include "gpad_log.h"
#include "gpad_ipem_lib.h"




/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */




/*
 * *******************************************************************
 *  fixed-point related
 * *******************************************************************
 */
gpad_ipem_fixedpt gpad_ipem_fixedpt_mul(gpad_ipem_fixedpt A, gpad_ipem_fixedpt B)
{
	return ((gpad_ipem_fixedptd)A * (gpad_ipem_fixedptd)B) >> GPAD_IPEM_FIXEDPT_FBITS;

}

gpad_ipem_fixedpt gpad_ipem_fixedpt_div(gpad_ipem_fixedpt A, gpad_ipem_fixedpt B)
{
	return ((gpad_ipem_fixedptd)A << GPAD_IPEM_FIXEDPT_FBITS) / (gpad_ipem_fixedptd)B;
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_sqrt(gpad_ipem_fixedpt A)
{
	int invert = 0;
	int iter = GPAD_IPEM_FIXEDPT_FBITS;
	int l, i;

	if (A < 0)
		return -1;
	if (A == 0 || A == GPAD_IPEM_FIXEDPT_ONE)
		return A;
	if (A < GPAD_IPEM_FIXEDPT_ONE && A > 6) {
		invert = 1;
		A = gpad_ipem_fixedpt_div(GPAD_IPEM_FIXEDPT_ONE, A);
	}
	if (A > GPAD_IPEM_FIXEDPT_ONE) {
		int s = A;

		iter = 0;
		while (s > 0) {
			s >>= 2;
			iter++;
		}
	}

	/* Newton's iterations */
	l = (A >> 1) + 1;
	for (i = 0; i < iter; i++)
		l = (l + gpad_ipem_fixedpt_div(A, l)) >> 1;
	if (invert)
		return gpad_ipem_fixedpt_div(GPAD_IPEM_FIXEDPT_ONE, l);
	return l;

}

gpad_ipem_fixedpt gpad_ipem_fixedpt_sin(gpad_ipem_fixedpt fp)
{
	int sign = 1;
	gpad_ipem_fixedpt sqr, result;
	const gpad_ipem_fixedpt SK[2] = {
		gpad_ipem_fixedpt_rconst(7.61e-03), gpad_ipem_fixedpt_rconst(1.6605e-01)
	};

	fp %= 2 * GPAD_IPEM_FIXEDPT_PI;
	if (fp < 0)
		fp = GPAD_IPEM_FIXEDPT_PI * 2 + fp;
	if ((fp > GPAD_IPEM_FIXEDPT_HALF_PI) && (fp <= GPAD_IPEM_FIXEDPT_PI))
		fp = GPAD_IPEM_FIXEDPT_PI - fp;
	else if ((fp > GPAD_IPEM_FIXEDPT_PI) && (fp <= (GPAD_IPEM_FIXEDPT_PI + GPAD_IPEM_FIXEDPT_HALF_PI))) {
		fp = fp - GPAD_IPEM_FIXEDPT_PI;
		sign = -1;
	} else if (fp > (GPAD_IPEM_FIXEDPT_PI + GPAD_IPEM_FIXEDPT_HALF_PI)) {
		fp = (GPAD_IPEM_FIXEDPT_PI << 1) - fp;
		sign = -1;
	}
	sqr = gpad_ipem_fixedpt_mul(fp, fp);
	result = SK[0];
	result = gpad_ipem_fixedpt_mul(result, sqr);
	result -= SK[1];
	result = gpad_ipem_fixedpt_mul(result, sqr);
	result += GPAD_IPEM_FIXEDPT_ONE;
	result = gpad_ipem_fixedpt_mul(result, fp);
	return sign * result;
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_cos(gpad_ipem_fixedpt A)
{
	return gpad_ipem_fixedpt_sin(GPAD_IPEM_FIXEDPT_HALF_PI - A);
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_tan(gpad_ipem_fixedpt A)
{
	return gpad_ipem_fixedpt_div(gpad_ipem_fixedpt_sin(A), gpad_ipem_fixedpt_cos(A));
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_exp(gpad_ipem_fixedpt fp)
{
	gpad_ipem_fixedpt xabs, k, z, R, xp;
	const gpad_ipem_fixedpt LN2 = gpad_ipem_fixedpt_rconst(0.69314718055994530942);
	const gpad_ipem_fixedpt LN2_INV = gpad_ipem_fixedpt_rconst(1.4426950408889634074);
	const gpad_ipem_fixedpt EXP_P[5] = {
		gpad_ipem_fixedpt_rconst(1.66666666666666019037e-01), gpad_ipem_fixedpt_rconst(-2.77777777770155933842e-03), gpad_ipem_fixedpt_rconst(6.61375632143793436117e-05), gpad_ipem_fixedpt_rconst(-1.65339022054652515390e-06), gpad_ipem_fixedpt_rconst(4.13813679705723846039e-08), };

	if (fp == 0)
		return GPAD_IPEM_FIXEDPT_ONE;
	xabs = gpad_ipem_fixedpt_abs(fp);
	k = gpad_ipem_fixedpt_mul(xabs, LN2_INV);
	k += GPAD_IPEM_FIXEDPT_ONE_HALF;
	k &= ~GPAD_IPEM_FIXEDPT_FMASK;
	if (fp < 0)
		k = -k;
	fp -= gpad_ipem_fixedpt_mul(k, LN2);
	z = gpad_ipem_fixedpt_mul(fp, fp);
	/* Taylor */
	R = GPAD_IPEM_FIXEDPT_TWO +
		gpad_ipem_fixedpt_mul(z, EXP_P[0] + gpad_ipem_fixedpt_mul(z, EXP_P[1] +
		gpad_ipem_fixedpt_mul(z, EXP_P[2] + gpad_ipem_fixedpt_mul(z, EXP_P[3] +
		gpad_ipem_fixedpt_mul(z, EXP_P[4])))));
	xp = GPAD_IPEM_FIXEDPT_ONE + gpad_ipem_fixedpt_div(gpad_ipem_fixedpt_mul(fp, GPAD_IPEM_FIXEDPT_TWO), R - fp);
	if (k < 0)
		k = GPAD_IPEM_FIXEDPT_ONE >> (-k >> GPAD_IPEM_FIXEDPT_FBITS);
	else
		k = GPAD_IPEM_FIXEDPT_ONE << (k >> GPAD_IPEM_FIXEDPT_FBITS);
	return gpad_ipem_fixedpt_mul(k, xp);
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_ln(gpad_ipem_fixedpt x)
{
	gpad_ipem_fixedpt log2, xi;
	gpad_ipem_fixedpt f, s, z, w, R;
	const gpad_ipem_fixedpt LN2 = gpad_ipem_fixedpt_rconst(0.69314718055994530942);
	const gpad_ipem_fixedpt LG[7] = {
		gpad_ipem_fixedpt_rconst(6.666666666666735130e-01), gpad_ipem_fixedpt_rconst(3.999999999940941908e-01), gpad_ipem_fixedpt_rconst(2.857142874366239149e-01), gpad_ipem_fixedpt_rconst(2.222219843214978396e-01), gpad_ipem_fixedpt_rconst(1.818357216161805012e-01), gpad_ipem_fixedpt_rconst(1.531383769920937332e-01), gpad_ipem_fixedpt_rconst(1.479819860511658591e-01)
	};

	if (x < 0)
		return 0;
	if (x == 0)
		return 0xffffffff;

	log2 = 0;
	xi = x;
	while (xi > GPAD_IPEM_FIXEDPT_TWO) {
		xi >>= 1;
		log2++;
	}
	f = xi - GPAD_IPEM_FIXEDPT_ONE;
	s = gpad_ipem_fixedpt_div(f, GPAD_IPEM_FIXEDPT_TWO + f);
	z = gpad_ipem_fixedpt_mul(s, s);
	w = gpad_ipem_fixedpt_mul(z, z);
	R = gpad_ipem_fixedpt_mul(w, LG[1] + gpad_ipem_fixedpt_mul(w, LG[3]
		+ gpad_ipem_fixedpt_mul(w, LG[5]))) + gpad_ipem_fixedpt_mul(z, LG[0]
		+ gpad_ipem_fixedpt_mul(w, LG[2] + gpad_ipem_fixedpt_mul(w, LG[4]
		+ gpad_ipem_fixedpt_mul(w, LG[6]))));
	return gpad_ipem_fixedpt_mul(LN2, (log2 << GPAD_IPEM_FIXEDPT_FBITS))
			+ f - gpad_ipem_fixedpt_mul(s, f - R);
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_log(gpad_ipem_fixedpt x, gpad_ipem_fixedpt base)
{
	return gpad_ipem_fixedpt_div(gpad_ipem_fixedpt_ln(x), gpad_ipem_fixedpt_ln(base));
}

gpad_ipem_fixedpt gpad_ipem_fixedpt_pow(gpad_ipem_fixedpt n, gpad_ipem_fixedpt exp)
{
	if (exp == 0)
		return GPAD_IPEM_FIXEDPT_ONE;
	if (n < 0)
		return 0;
	return gpad_ipem_fixedpt_exp(gpad_ipem_fixedpt_mul(gpad_ipem_fixedpt_ln(n), exp));
}

void gpad_ipem_fixedpt_str(gpad_ipem_fixedpt A, char *str, s32 max_dec)
{
	int ndec = 0, slen = 0;
	char tmp[12] = {0};
	gpad_ipem_fixedptud fr, ip;
	const gpad_ipem_fixedptud one = (gpad_ipem_fixedptud)1 << GPAD_IPEM_FIXEDPT_BITS;
	const gpad_ipem_fixedptud mask = one - 1;

	if (max_dec == -1)
#if GPAD_IPEM_FIXEDPT_WBITS > 16
		max_dec = 2;
#else
		max_dec = 4;
#endif
	else if (max_dec == -2)
		max_dec = 15;

	if (A < 0) {
		str[slen++] = '-';
		A *= -1;
	}

	ip = gpad_ipem_fixedpt_toint(A);
	do {
		tmp[ndec++] = '0' + ip % 10;
		ip /= 10;
	} while (ip != 0);

	while (ndec > 0)
		str[slen++] = tmp[--ndec];
	str[slen++] = '.';

	fr = (gpad_ipem_fixedpt_fracpart(A) << GPAD_IPEM_FIXEDPT_WBITS) & mask;
	do {
		fr = (fr & mask) * 10;

		str[slen++] = '0' + (fr >> GPAD_IPEM_FIXEDPT_BITS) % 10;
		ndec++;
	} while (fr != 0 && ndec < max_dec);

	if (ndec > 1 && str[slen-1] == '0')
		str[slen-1] = '\0'; /* cut off trailing 0 */
	else
		str[slen] = '\0';
}

char *gpad_ipem_fixedpt_cstr(const gpad_ipem_fixedpt A)
{
	static char str[48];

	gpad_ipem_fixedpt_str(A, str, 48);
	return str;
}




/*
 * *******************************************************************
 *  stateaction related
 * *******************************************************************
 */
void gpad_ipem_stateaction_enc(gpad_ipem_stateaction *stateaction, gpad_ipem_state state, gpad_ipem_action action)
{
	/* FIXME: a better way, much more adaptive */
	*stateaction = (state << 2) | action;
}

void gpad_ipem_stateaction_dec(gpad_ipem_stateaction stateaction, gpad_ipem_state *state, gpad_ipem_action *action)
{
	*state = stateaction >> 2;
	*action = stateaction & 0x3;
}




/*
 * *******************************************************************
 *  table-search related
 * *******************************************************************
 */
gpad_ipem_action gpad_ipem_tsearch_do(struct mtk_gpufreq_info *table, gpad_ipem_action action_llimit, gpad_ipem_action action_ulimit, u32 gpuload)
{
	/* implement Table-Search here */
	gpad_ipem_action action;

	WARN_ON(action_llimit > action_ulimit);

	while (action_llimit < action_ulimit) {
		action = (action_ulimit + action_llimit) / 2;

		if (gpuload < table[action].lower_bound)
			action_llimit = action + 1;
		else
			action_ulimit = action;
	}
	action = action_llimit;

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, gpuload = %u, action = %u\n", __func__, action_llimit, action_ulimit, gpuload, action);

	return action;
}




/*
 * *******************************************************************
 *  q-maximum related
 * *******************************************************************
 */
gpad_ipem_action gpad_ipem_qmax_do(gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end)
{
	/* implement Q-Maximum here */
	gpad_ipem_stateaction stateaction;
	gpad_ipem_state state_max;
	gpad_ipem_action action_max;
	gpad_ipem_stateaction stateaction_max;
	gpad_ipem_fixedpt q_max;

	WARN_ON(start > end);

	q_max = qs[start];
	stateaction_max = start;
	for (stateaction = start + 1; stateaction <= end; stateaction++) {
		if (qs[stateaction] >= q_max) {
			q_max = qs[stateaction];
			stateaction_max = stateaction;
		}
	}

	gpad_ipem_stateaction_dec(stateaction_max, &state_max, &action_max);

	GPAD_LOG_INFO("[IPEM] %s: start = %u, end = %u, q_max = %s, action_max = %u\n", __func__, start, end, gpad_ipem_fixedpt_cstr(q_max), action_max);

	return action_max;
}




/*
 * *******************************************************************
 *  epsilon-greedy related
 * *******************************************************************
 */
gpad_ipem_action gpad_ipem_egreedy_do(gpad_ipem_fixedpt epsilon, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end)
{
	/* implement Epsilon-Greedy here */
	gpad_ipem_fixedpt rand;
	gpad_ipem_action action;

	WARN_ON(start > end);

	get_random_bytes(&rand, sizeof(gpad_ipem_fixedpt));
	rand = gpad_ipem_fixedpt_fracpart(rand);

	if (rand < epsilon)
		action = rand % (end - start + 1);
	else
		action = gpad_ipem_qmax_do(qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: start = %u, end = %u, epsilon = %s, action = %u\n", __func__, start, end, gpad_ipem_fixedpt_cstr(epsilon), action);

	return action;
}




/*
 * *******************************************************************
 *  softmax related
 * *******************************************************************
 */
gpad_ipem_action gpad_ipem_softmax_do(gpad_ipem_fixedpt tau, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end)
{
	/* implement SoftMax here */
	gpad_ipem_fixedpt q;
	gpad_ipem_fixedpt sigma;
	gpad_ipem_stateaction stateaction;
	gpad_ipem_fixedpt rand;
	gpad_ipem_fixedpt accu;
	gpad_ipem_state state;
	gpad_ipem_action action;

	WARN_ON(start > end);

	sigma = 0;
	for (stateaction = start; stateaction <= end; stateaction++) {
		q = qs[stateaction];
		sigma += gpad_ipem_fixedpt_exp(gpad_ipem_fixedpt_div(q, tau));
	}

	get_random_bytes(&rand, sizeof(gpad_ipem_fixedpt));
	rand = gpad_ipem_fixedpt_fracpart(rand);

	stateaction = start;
	accu = 0;
	while (accu < rand) {
		q = qs[stateaction];
		accu += gpad_ipem_fixedpt_div(gpad_ipem_fixedpt_exp(gpad_ipem_fixedpt_div(q, tau)), sigma);
		stateaction++;
	}
	stateaction--;

	gpad_ipem_stateaction_dec(stateaction, &state, &action);

	GPAD_LOG_INFO("[IPEM] %s: start = %u, end = %u, tau = %s, action = %u\n", __func__, start, end, gpad_ipem_fixedpt_cstr(tau), action);

	return action;
}




/*
 * *******************************************************************
 *  value-difference based exploration related
 * *******************************************************************
 */
gpad_ipem_action gpad_ipem_vdbe_do(gpad_ipem_fixedpt *epsilons, gpad_ipem_fixedpt tau, gpad_ipem_fixedpt *qs, gpad_ipem_stateaction start, gpad_ipem_stateaction end)
{
	/* implement Value-difference Based Exploration Do here */
	gpad_ipem_fixedpt rand;
	gpad_ipem_fixedpt epsilon;
	gpad_ipem_state state;
	gpad_ipem_action action;

	WARN_ON(start > end);

	gpad_ipem_stateaction_dec(start, &state, &action);

	epsilon = epsilons[state];

	get_random_bytes(&rand, sizeof(gpad_ipem_fixedpt));
	rand = gpad_ipem_fixedpt_fracpart(rand);

	if (rand < epsilon)
		action = gpad_ipem_softmax_do(tau, qs, start, end);
	else
		action = gpad_ipem_qmax_do(qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: start = %u, end = %u, epsilon = %s, action = %u\n", __func__, start, end, gpad_ipem_fixedpt_cstr(epsilon), action);

	return action;
}

void gpad_ipem_vdbe_update(gpad_ipem_fixedpt *epsilons, gpad_ipem_fixedpt delta, gpad_ipem_fixedpt sigma, gpad_ipem_fixedpt q_delta, gpad_ipem_state state)
{
	/* implement Value-difference Based Exploration Update here */
	gpad_ipem_fixedpt q_delta_abs;
	gpad_ipem_fixedpt numerator;
	gpad_ipem_fixedpt denominator;
	gpad_ipem_fixedpt epsilon;

	q_delta_abs = gpad_ipem_fixedpt_abs(q_delta);

	numerator = GPAD_IPEM_FIXEDPT_ONE - gpad_ipem_fixedpt_div(-q_delta_abs, sigma);
	denominator = GPAD_IPEM_FIXEDPT_ONE + gpad_ipem_fixedpt_div(-q_delta_abs, sigma);

	epsilon = (gpad_ipem_fixedpt_mul(delta, gpad_ipem_fixedpt_div(numerator, denominator)))
	+ (gpad_ipem_fixedpt_mul((GPAD_IPEM_FIXEDPT_ONE - delta), epsilons[state]));

	epsilons[state] = epsilon;

	GPAD_LOG_INFO("[IPEM] %s: state = %u, epsilon = %s\n", __func__, state, gpad_ipem_fixedpt_cstr(epsilon));
}

