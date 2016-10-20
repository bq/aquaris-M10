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

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_api.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_msgq.h"

#ifdef G3DKMD_SUPPORT_MSGQ

static __DEFINE_SEM_LOCK(_msgQLock);

/***** Define *****/
#define G3DKMD_MSGQ_SIZE        0x1000
#define G3DKMD_MSGQ_GUARD_BAND  0x10
#define G3DKMD_MSGQ_ALIGN       0x4

struct G3DKMD_MSGQ_INFO {
	G3DKMD_MSGQ_TYPE type;
	unsigned int size;
	void *data;
};

#define _align(ptr, ofst)   (((unsigned int)(ptr) + (ofst - 1)) & (~(ofst - 1)))
#define _get_rptr()         ((struct G3DKMD_MSGQ_INFO *)(_msgQ_buf + _msgQ_rptr))
#define _get_wptr()         ((struct G3DKMD_MSGQ_INFO *)(_msgQ_buf + _msgQ_wptr))
#define _is_empty()         (_msgQ_rptr == _msgQ_wptr)
#define _req_sz(size)       (_align((sizeof(struct G3DKMD_MSGQ_INFO) + (size)), G3DKMD_MSGQ_ALIGN))
#define _ptr_adv(ptr, size) do { (ptr) += _req_sz((size)); if (ptr >= G3DKMD_MSGQ_SIZE) ptr = 0; } while (0)

/***** global variable *****/
static unsigned int _msgQ_buf, _msgQ_rptr, _msgQ_wptr;

/***** local functions *****/
static struct G3DKMD_MSGQ_INFO *_g3dKmdMsgQGetEmptyBuf(unsigned int size)
{
	unsigned int empty_size, required_size;

	required_size = _req_sz(size) + G3DKMD_MSGQ_GUARD_BAND;

	/*
	 * if the last empty size is less than our required size,
	 * just mark it as SKIP, in order to prevent "ring-back" case
	 */
	if ((_msgQ_rptr < _msgQ_wptr) && ((G3DKMD_MSGQ_SIZE - _msgQ_wptr) < required_size)) {
		struct G3DKMD_MSGQ_INFO *pInfo;

		if (_msgQ_rptr < G3DKMD_MSGQ_GUARD_BAND)
			return NULL;

		pInfo = _get_wptr();
		pInfo->type = G3DKMD_MSGQ_TYPE_SKIP;
		pInfo->size = G3DKMD_MSGQ_SIZE - _msgQ_wptr;
		_msgQ_wptr = 0;
	}

	empty_size =
	    (((_msgQ_rptr <= _msgQ_wptr) ? G3DKMD_MSGQ_SIZE : 0) + _msgQ_rptr) - _msgQ_wptr;

	if (empty_size >= required_size)
		return _get_wptr();

	return NULL;
}

static G3DKMD_MSGQ_RTN _g3dKmdMsgQGetMsgInfo(G3DKMD_MSGQ_TYPE *type, unsigned int *size)
{
	struct G3DKMD_MSGQ_INFO *pInfo;

	while (G3DKMD_TRUE) {
		if (_is_empty())
			return G3DKMD_MSGQ_RTN_EMPTY;

		pInfo = _get_rptr();

		if (pInfo->type == G3DKMD_MSGQ_TYPE_SKIP)
			_ptr_adv(_msgQ_rptr, pInfo->size);
		else {
			*type = pInfo->type;
			*size = pInfo->size;
			break;
		}
	}

	return G3DKMD_MSGQ_RTN_OK;
}

/***** global functions *****/
void g3dKmdMsgQInit(void)
{
	_msgQ_buf = (unsigned int)G3DKMDVMALLOC(G3DKMD_MSGQ_SIZE);
	YL_KMD_ASSERT(_msgQ_buf);

	_msgQ_rptr = _msgQ_wptr = 0; /* (_msgQ_rptr == _msgQ_wprt) means buffer empty */
}

void g3dKmdMsgQUninit(void)
{
	G3DKMDVFREE(((void *)_msgQ_buf));
	_msgQ_buf = _msgQ_rptr = _msgQ_wptr = 0;
}

G3DKMD_MSGQ_RTN g3dKmdMsgQSend(G3DKMD_MSGQ_TYPE type, unsigned int size, void *buf)
{
	struct G3DKMD_MSGQ_INFO *pInfo;
	G3DKMD_MSGQ_RTN rtnVal = G3DKMD_MSGQ_RTN_OK;

	if ((size == 0) || (!buf))
		return G3DKMD_MSGQ_RTN_ERR;

	__SEM_LOCK(_msgQLock);

	pInfo = _g3dKmdMsgQGetEmptyBuf(size);
	if (!pInfo) {
		rtnVal = G3DKMD_MSGQ_RTN_FULL;
		goto JMP_RTN;
	}

	pInfo->type = type;
	pInfo->size = size;
	pInfo->data = (void *)((unsigned int)pInfo + sizeof(struct G3DKMD_MSGQ_INFO));
	memcpy(pInfo->data, buf, size);

	_ptr_adv(_msgQ_wptr, size);

#ifdef _DEBUG
	{
		unsigned int struct_size;

		switch (type) {
		case G3DKMD_MSGQ_TYPE_ALLOC_MEM:
			struct_size = sizeof(G3DKMD_MSGQ_ALLOC_MEM);
			break;
		case G3DKMD_MSGQ_TYPE_FREE_MEM:
			struct_size = sizeof(G3DKMD_MSGQ_FREE_MEM);
			break;
		default:
			struct_size = 0;
		}
		YL_KMD_ASSERT(!struct_size || (struct_size == size));
	}
#endif

JMP_RTN:
	__SEM_UNLOCK(_msgQLock);

	return rtnVal;
}

G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQGetMsgInfo(G3DKMD_MSGQ_TYPE *type,
								unsigned int *size)
{
	G3DKMD_MSGQ_RTN rtnVal;

	if (!type || !size)
		return G3DKMD_MSGQ_RTN_ERR;

	__SEM_LOCK(_msgQLock);

	rtnVal = _g3dKmdMsgQGetMsgInfo(type, size);

	__SEM_UNLOCK(_msgQLock);

	return rtnVal;
}

#if (defined(linux) && !defined(linux_user_mode))
G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQReceive(G3DKMD_MSGQ_TYPE *type,
							     unsigned int *size, void __user *buf)
#else
G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQReceive(G3DKMD_MSGQ_TYPE *type,
							     unsigned int *size, void *buf)
#endif
{
	struct G3DKMD_MSGQ_INFO *pInfo;
	G3DKMD_MSGQ_RTN rtnVal;

	if (!type || !size)
		return G3DKMD_MSGQ_RTN_ERR;

	__SEM_LOCK(_msgQLock);

	rtnVal = _g3dKmdMsgQGetMsgInfo(type, size);
	if (rtnVal != G3DKMD_MSGQ_RTN_OK)
		goto JMP_RTN;

	pInfo = _get_rptr();
	if (*size < pInfo->size) {
		rtnVal = G3DKMD_MSGQ_RTN_ERR;
		goto JMP_RTN;
	}

	*type = pInfo->type;
	*size = pInfo->size;

#if (defined(linux) && !defined(linux_user_mode))
	if ((uintptr_t) buf < PAGE_OFFSET) { /* pointer is user space */
		copy_to_user(buf, (void *)pInfo->data, pInfo->size);
	} else
#endif
	{ /* pointer is kernel space */
		memcpy(buf, pInfo->data, pInfo->size);
	}

	_ptr_adv(_msgQ_rptr, pInfo->size);

JMP_RTN:
	__SEM_UNLOCK(_msgQLock);

	return rtnVal;
}

/***** Helper functions *****/
void g3dKmdMsgQSetReg(unsigned int reg, unsigned int value)
{
	G3DKMD_MSGQ_SET_REG msg;

	msg.reg = reg;
	msg.value = value;
	while (g3dKmdMsgQSend(G3DKMD_MSGQ_TYPE_SET_REG, sizeof(G3DKMD_MSGQ_SET_REG), &msg) ==
	       G3DKMD_MSGQ_RTN_FULL) {
		G3DKMD_SLEEP(1);
	}
}

void g3dKmdMsgQAllocMem(unsigned int hwAddr, uint64_t phyAddr, unsigned int size)
{
	G3DKMD_MSGQ_ALLOC_MEM msg;

	msg.hwAddr = hwAddr;
	msg.phyAddr = phyAddr;
	msg.size = size;
	while (g3dKmdMsgQSend(G3DKMD_MSGQ_TYPE_ALLOC_MEM, sizeof(G3DKMD_MSGQ_ALLOC_MEM), &msg) ==
	       G3DKMD_MSGQ_RTN_FULL) {
		G3DKMD_SLEEP(1);
	}
}

void g3dKmdMsgQFreeMem(unsigned int hwAddr, unsigned int size)
{
	G3DKMD_MSGQ_FREE_MEM msg;

	msg.hwAddr = hwAddr;
	msg.size = size;
	while (g3dKmdMsgQSend(G3DKMD_MSGQ_TYPE_FREE_MEM, sizeof(G3DKMD_MSGQ_FREE_MEM), &msg) ==
	       G3DKMD_MSGQ_RTN_FULL) {
		G3DKMD_SLEEP(1);
	}
}
#else
G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQGetMsgInfo(G3DKMD_MSGQ_TYPE *type,
								unsigned int *size)
{
	return G3DKMD_MSGQ_RTN_ERR;
}

G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQReceive(G3DKMD_MSGQ_TYPE *type,
							     unsigned int *size, void *buf)
{
	return G3DKMD_MSGQ_RTN_ERR;
}
#endif /* G3DKMD_SUPPORT_MSGQ */
