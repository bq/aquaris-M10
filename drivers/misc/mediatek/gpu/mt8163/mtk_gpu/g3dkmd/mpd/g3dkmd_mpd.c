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


#if (defined(linux) && !defined(linux_user_mode))
#include <asm/page.h>		/* for page */
#include <linux/list.h>		/* for list */
#include <linux/highmem.h>	/* for kmap()/kunmap() */
#include <linux/slab.h>		/* for kmalloc()/kfree() */
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"	/* for lock */


#include "g3dkmd_pattern.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_api_mmu.h"

/* #define S_DEBUG_FLOW */
/* #define S_DEBUG_IO */
/* #define S_DEBUG_DRAM */

/* #define S_DEBUG_SKIP_DRAM_CONTENT */

#define SDPF(...) pr_debug(__VA_ARGS__)
/* #define SUDPF(...) */

#if defined(SUPPORT_MPD) && (defined(linux) && !defined(linux_user_mode))

unsigned int g_globalCnt = 0;
unsigned int g_shouldDump = 0;

#if defined(ANDROID) && !defined(YL_FAKE_X11)
#define ANDROID_AND_NOT_LINUX_FPGA
#endif



static __DEFINE_SEM_LOCK(memBlockLock);

struct MPDMemblk {
	unsigned int PA;
	unsigned int G3DVA;
	unsigned int size;
	struct list_head node;
};

enum STATE {
	ST_START,
	ST_1004_01,
	ST_0050_02,
	ST_6100_01,
	ST_1110_XX,
	ST_1114_XX,
	ST_1118_X1,
	ST_1008_01,
	ST_5024_XX,
	ST_5028_XX,
};

LIST_HEAD(g_KMemBlks);

#define MSK_AREG_CQ_HTBUF_WPTR_UPD     0x00000004
#define REG_AREG_CQ_CTRL               0x1118
#define REG_AREG_G3D_ENG_FIRE          0x1008

#define MMU_SMALL_PAGE_SIZE      4096UL

#define PAGE_OFFSET_MASK ((1<<PAGE_SHIFT)-1)


#define MAX_SUPPORT_PID 1024

#define MAX_STR_LEN 256


struct MPD_Control {
	char filePrefix[MAX_STR_LEN];

	int isEnableMPD;

	int isDumpAllPID;
	int targetPID;
	char targetAP[MAX_STR_LEN];

	int isLimitDumpNumber;
	int dumpNumbers;
	int skipNumbers;

};

struct MPD_Control g_control = {

#ifdef ANDROID_AND_NOT_LINUX_FPGA
	.filePrefix = {'/', 'd', 'a', 't', 'a', '/', 0},
#else
	.filePrefix = {0},
#endif

	.isEnableMPD = 0,

	.isDumpAllPID = 1,
	.targetPID = -1,
	.targetAP = {0},

	.isLimitDumpNumber = 0,
	.dumpNumbers = 0,
	.skipNumbers = 0,

};

enum MPD_SAVED_REG {
	REG_1F0C = 0,
	REG_1F20,
	REG_1198,
	REG_119C,
	REG_5000,
	REG_5004,
	REG_4180,
	REG_1110,
	REG_1114,

	SAVE_REG_NUM,
};

int saved_data[SAVE_REG_NUM];

struct PID_Data {
	pid_t pid;
	void *riu_fd;
	int frameCnt;
	int wpCnt;
};

struct PID_Data g_pids[MAX_SUPPORT_PID];
unsigned int g_pid_num = 0;


struct PID_Data *getPIDData(void)
{
	int i;
	pid_t pid = current->tgid;

	for (i = 0; i < g_pid_num; i++) {
		if (g_pids[i].pid == pid)
			return &g_pids[i];
	}

	return 0;

}

void insertPIDData(struct PID_Data data)
{
	YL_KMD_ASSERT(g_pid_num < MAX_SUPPORT_PID);

	if (g_pid_num < MAX_SUPPORT_PID) {
		g_pids[g_pid_num].pid = data.pid;
		g_pids[g_pid_num].riu_fd = data.riu_fd;
		g_pids[g_pid_num].frameCnt = data.frameCnt;
		g_pids[g_pid_num].wpCnt = data.wpCnt;
		g_pid_num++;
	}

}

void removePIDData(void)
{


}

void *openDramFD(void)
{
	void *fd = 0;
	pid_t pid = current->tgid;
	char filename[MAX_BUFFERFILE_NAME];
	struct PID_Data *data = getPIDData();

	YL_KMD_ASSERT(data);

	sprintf(filename, "%sp%d_g%03d_dram_buffer_kmd_%05d_%05d.hex", g_control.filePrefix, pid,
		g_globalCnt, data->frameCnt, data->wpCnt);

#if defined(S_DEBUG_IO)
	SDPF("[MPD %d]Open %s\n", pid, filename);
#endif

	fd = g_pKmdFileIo->fileOpen(filename);

	return fd;
}

void *openSaveRegFD(void)
{
	void *fd = 0;
	pid_t pid = current->tgid;
	char filename[MAX_BUFFERFILE_NAME];
	struct PID_Data *data = getPIDData();

	YL_KMD_ASSERT(data);

	sprintf(filename, "%sp%d_g%03d_saved_reg_%05d_%05d.hex", g_control.filePrefix, pid,
		g_globalCnt, data->frameCnt, data->wpCnt);

#if defined(S_DEBUG_IO)
	SDPF("[MPD %d]Open %s\n", pid, filename);
#endif

	fd = g_pKmdFileIo->fileOpen(filename);

	return fd;
}

void *getRiuFD(void)
{
	void *riu_fd = 0;
	pid_t pid = current->tgid;
	char filename[MAX_BUFFERFILE_NAME];
	struct PID_Data temp;
	struct PID_Data *data = getPIDData();

	if (data) {
		riu_fd = data->riu_fd;

		if (!riu_fd) {
			sprintf(filename, "%sp%d_g%03d_riu_g3d_%05d_%05d.hex", g_control.filePrefix,
				pid, g_globalCnt, data->frameCnt, data->wpCnt);

#if defined(S_DEBUG_IO)
			SDPF("[MPD %d]Open %s\n", pid, filename);
#endif
			riu_fd = g_pKmdFileIo->fileOpen(filename);
			data->riu_fd = riu_fd;
		}

	} else {
		sprintf(filename, "%sp%d_g%03d_riu_g3d_%05d_%05d.hex", g_control.filePrefix, pid,
			g_globalCnt, 0, 0);
#if defined(S_DEBUG_IO)
		SDPF("[MPD %d]Open %s\n", pid, filename);
#endif
		riu_fd = g_pKmdFileIo->fileOpen(filename);

		temp.pid = pid;
		temp.riu_fd = riu_fd;
		temp.frameCnt = 0;
		temp.wpCnt = 0;
		insertPIDData(temp);
	}

	return riu_fd;
}

void closeRiuFD(void)
{
	unsigned int i;

	for (i = 0; i < g_pid_num; i++) {
		if (g_pids[i].riu_fd != 0) {
#if defined(S_DEBUG_IO)
			SDPF("[MPD %d] close pid %d riu fd\n", current->tgid, g_pids[i].pid);
#endif
			g_pKmdFileIo->fileClose(g_pids[i].riu_fd);
			g_pids[i].riu_fd = 0;
		}
	}
}

void CMDumpKMemory(void *fd, void *data, unsigned int pa, unsigned int g3dva, unsigned int size)
{
	unsigned int i, addr, boundary128, remain;
	unsigned int data4[4];
	unsigned char data1[16];
	unsigned int *ptr;
	unsigned char *cptr;
	const int PTR_SIZE = sizeof(unsigned int);

	g_pKmdFileIo->fPrintf(fd, "F0E0F0E0_F0E0F0E0_F0E0F0E0_%08X  // G3DVA=%08x\n", pa >> 4,
			      g3dva);
#ifdef S_DEBUG_SKIP_DRAM_CONTENT
	g_pKmdFileIo->fPrintf(fd, "(memory content omitted)\n");
#endif

	YL_KMD_ASSERT((pa & 0xf) == 0);
	boundary128 = size & (~0xf);

	for (i = 0; i < boundary128; i += 16) {
		ptr = (unsigned int *)data + i / PTR_SIZE;
		addr = pa + i;
		data4[0] = *ptr;
		data4[1] = *(ptr + 1);
		data4[2] = *(ptr + 2);
		data4[3] = *(ptr + 3);

#ifndef S_DEBUG_SKIP_DRAM_CONTENT
		g_pKmdFileIo->fPrintf(fd, "%08x_%08x_%08x_%08x // addr=%07x\n",
				      data4[3], data4[2], data4[1], data4[0], (addr >> 4));
#endif

	}

	if (boundary128 != size) {

		remain = size - boundary128;
		cptr = (unsigned char *)data + boundary128;

		for (i = 0; i < 16; i++) {
			if (i < remain)
				data1[i] = *(cptr + i);
			else
				data1[i] = 0x5a;
		}

		addr = pa + boundary128;

#ifndef S_DEBUG_SKIP_DRAM_CONTENT
		g_pKmdFileIo->fPrintf(fd,
				"%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x // addr=%07x\n",
				data1[15], data1[14], data1[13], data1[12], data1[11],
				data1[10], data1[9], data1[8], data1[7], data1[6], data1[5],
				data1[4], data1[3], data1[2], data1[1], data1[0],
				(addr >> 4));
#endif

	}


}

void CMDumpAllKMemory(void)
{
	struct list_head *iter;
	struct MPDMemblk *mem;

	void *hMmu = g3dKmdMmuGetHandle();
	unsigned int pa;
	struct page *page;
	void *va;
	void *fd = openDramFD();
	unsigned int offset;
	int remain_size;
	int dump_size;
	int pa_4k_offset;

#if defined(S_DEBUG_FLOW) || defined(S_DEBUG_DRAM)
	SDPF("[MPD %d]CMDumpAllKMemory()\n", current->tgid);
#endif

#if 1				/* FPGA */
	{
		void *swAddr, *hwAddr;
		unsigned int size, i;
		unsigned int *lone_va;

		/* get MMU table */
		g3dKmdMmuGetTable(hMmu, &swAddr, &hwAddr, &size);

#if defined(S_DEBUG_DRAM)
		SDPF("[MPD %d]ready to dump mem g3dva=n/a, size=%x, pa=%08x (mmu L1)\n",
		     current->tgid, size, (unsigned int)(uintptr_t) hwAddr);
#endif

		/* dump L1 */
		CMDumpKMemory(fd, (void *)swAddr, (unsigned int)(uintptr_t) hwAddr,
			      (unsigned int)(uintptr_t) hwAddr, size);

		/* parse L1 */
		lone_va = (unsigned int *)swAddr;

		for (i = 0; i < size / 4; i++) {
			unsigned int lone_entry = lone_va[i];

			if ((lone_entry & 0x3) == 0x1) {
				unsigned int ltwo_pa = lone_entry & 0xFFFFFC00UL;

#if defined(S_DEBUG_DRAM)
				SDPF("[MPD %d]ready to dump mem g3dva=n/a, size=%x, pa=%08x (mmu L2)\n",
					current->tgid, 1024, ltwo_pa);
#endif

				struct page *l2_page = (pfn_to_page(ltwo_pa >> PAGE_SHIFT));
				unsigned int *ltwo_va =
				    (unsigned int *)vmap(&l2_page, 1, VM_MAP, PAGE_KERNEL);
				void *va;

				va = (void *)((uintptr_t) ltwo_va | (ltwo_pa & 0xC00));

				CMDumpKMemory(fd, va, ltwo_pa, ltwo_pa, 1024);

				vunmap(ltwo_va);

			}
		}
	}
#endif


	__SEM_LOCK(memBlockLock);
	list_for_each(iter, &g_KMemBlks) {
		mem = list_entry(iter, struct MPDMemblk, node);

#if 0
		if (mem->G3DVA == mem->PA) {
			/* this memory block don't have actual G3DVA, dump all block with single PA */

			pa = mem->PA;
#if defined(S_DEBUG_DRAM)
			SDPF("[MPD %d]ready to dump mem g3dva=n/a, size=%x, pa=%08x\n",
				current->tgid, mem->size, pa);
#endif
			page = (pfn_to_page(pa >> PAGE_SHIFT));
			va = (void *)((uintptr_t) kmap(page) + (pa & PAGE_OFFSET_MASK));
			CMDumpKMemory(fd, (void *)va, pa, mem->G3DVA, mem->size);
			kunmap(page);
		} else {
			/* this memory block need convert each 4K block to PA */
#else
		if (mem->G3DVA != mem->PA) {
#endif
#if defined(S_DEBUG_DRAM)
			SDPF("[MPD %d]ready to dump mem g3dva=%08x, size=%x, (pa=%08x)\n",
				current->tgid, mem->G3DVA, mem->size, mem->PA);
#endif

			remain_size = mem->size;
			while (remain_size > 0) {
				offset = mem->size - remain_size;

				pa = g3dKmdMmuG3dVaToPa(hMmu, mem->G3DVA + offset);
				pa_4k_offset = pa % MMU_SMALL_PAGE_SIZE;

				if (remain_size + pa_4k_offset > MMU_SMALL_PAGE_SIZE) {
					dump_size = MMU_SMALL_PAGE_SIZE - pa_4k_offset;
				} else {
					dump_size =
					    (remain_size >
					     MMU_SMALL_PAGE_SIZE ? MMU_SMALL_PAGE_SIZE :
					     remain_size);
				}

#if defined(S_DEBUG_DRAM)
				SDPF("[MPD %d] convert g3dva=%08x, pa=%08x, size=%x\n",
				     current->tgid, mem->G3DVA + offset, pa, dump_size);
#endif

				page = (pfn_to_page(pa >> PAGE_SHIFT));
				va = (void *)((uintptr_t) kmap(page) + (pa & PAGE_OFFSET_MASK));
				CMDumpKMemory(fd, (void *)va, pa, mem->G3DVA + offset, dump_size);
				kunmap(page);
				remain_size -= dump_size;
			}
		}
	}
	__SEM_UNLOCK(memBlockLock);

	g_pKmdFileIo->fileClose(fd);
#if defined(S_DEBUG_IO)
	SDPF("[MPD %d]close dram fd\n", current->tgid);
#endif


	fd = openSaveRegFD();

	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x1F0C, saved_data[REG_1F0C]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x1F20, saved_data[REG_1F20]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x1198, saved_data[REG_1198]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x119C, saved_data[REG_119C]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x5000, saved_data[REG_5000]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x5004, saved_data[REG_5004]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x4180, saved_data[REG_4180]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x1110, saved_data[REG_1110]);
	g_pKmdFileIo->fPrintf(fd, "%04x_%08x\n", 0x1114, saved_data[REG_1114]);

	g_pKmdFileIo->fileClose(fd);
#if defined(S_DEBUG_IO)
	SDPF("[MPD %d]close reg fd\n", current->tgid);
#endif

}


void StateTransition(int command, int data)
{
	void *fp = getRiuFD();
	static int m_rState = ST_START;

	switch (m_rState) {
	case ST_START:
		{
			switch (command) {
			case 0x1110:
				m_rState = ST_1110_XX;
				break;
			case 0x5024:
				m_rState = ST_5024_XX;
				break;
			case 0x6100:
				if (data == 1) {
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0xfff8,
							      0x00000001);
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0x6100,
							      0x00000000);
				}
				break;
			case 0x1004:
				if (data == 1) {
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0xfff8,
							      0x00000002);
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0x0050,
							      0x00000002);
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0xfff8,
							      0x00000002);
					g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0x1180,
							      0x00000002);
				}
				break;
			}
		}
		break;
	case ST_1110_XX:
		if (command == 0x1114)
			m_rState = ST_1114_XX;
		else
			m_rState = ST_START;
		break;
	case ST_1114_XX:
		if (command == 0x1118 && (data & 0x00000001) == 0x1)
			m_rState = ST_1118_X1;
		else
			m_rState = ST_START;
		break;
	case ST_1118_X1:
		if (command == 0x1008 && data == 0x1)
			m_rState = ST_1008_01;
		else
			m_rState = ST_START;
		break;
	case ST_1008_01:
		if (command == 0x1118 && (data & 0x00000001) == 0x0) {
			g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0xfff8, 0x00000080);
			g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0x1208, 0x00000080);
		}
		m_rState = ST_START;
		break;
	case ST_5024_XX:
		if (command == 0x5028)
			m_rState = ST_5028_XX;
		else
			m_rState = ST_START;
		break;
	case ST_5028_XX:
		if (command == 0x5020 && data == 0x1) {
			g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0xfff8, 0x00000001);
			g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", 0x512c, 0x00000001);
		}
		m_rState = ST_START;
		break;
	}
}


void CMMPDControl(g3dkmd_IOCTL_MPD_Control *cmd)
{
	char buf[MAX_STR_LEN];

	switch (cmd->op) {
	case MPD_SET_ENABLE:
		g_control.isEnableMPD = cmd->arg;
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_SET_ENABLE, %d)\n", current->tgid, cmd->arg);
#endif
		break;

	case MPD_SET_DUMP_PATH:
		YL_KMD_ASSERT(cmd->str);
		YL_KMD_ASSERT(cmd->str_len < MAX_STR_LEN);
		if (copy_from_user
		    ((void *)g_control.filePrefix, (const void __user *)cmd->str,
		     cmd->str_len) == 0) {
			g_control.filePrefix[cmd->str_len] = '\0';
#ifdef S_DEBUG_FLOW
			SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_PATH, %s)\n", current->tgid,
			     g_control.filePrefix);
#endif
		} else {
			SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_PATH) copy_from_user failed\n",
			     current->tgid);
			break;
		}

		break;

	case MPD_SET_DUMP_APNAME:
		YL_KMD_ASSERT(cmd->str);
		YL_KMD_ASSERT(cmd->str_len < MAX_STR_LEN);
		if (copy_from_user
		    ((void *)g_control.targetAP, (const void __user *)cmd->str,
		     cmd->str_len) == 0) {
			g_control.targetAP[cmd->str_len] = '\0';
			g_control.targetPID = 0;
#ifdef S_DEBUG_FLOW
			SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_APNAME, %s)\n", current->tgid,
			     g_control.targetAP);
#endif
		} else {
			SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_APNAME) copy_from_user failed\n",
			     current->tgid);
			break;
		}

		break;

	case MPD_SET_DUMP_PID:
		g_control.isDumpAllPID = 0;
		g_control.targetPID = cmd->arg;
		g_control.targetAP[0] = '\0';
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_PID, %d)\n", current->tgid, cmd->arg);
#endif
		break;

	case MPD_SET_SKIP_FRAME:
		g_control.skipNumbers = cmd->arg;
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_SET_SKIP_FRAME, %d)\n", current->tgid, cmd->arg);
#endif
		break;

	case MPD_SET_DUMP_FRAME:

		if (cmd->arg != 0) {
			g_control.isLimitDumpNumber = 1;
			g_control.dumpNumbers = cmd->arg;
		} else {
			g_control.isLimitDumpNumber = 0;
			g_control.dumpNumbers = 0;
		}
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_FRAME, %d)\n", current->tgid, cmd->arg);
#endif
		break;
	case MPD_SET_DUMP_ALLPID:
		g_control.isDumpAllPID = cmd->arg;
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_SET_DUMP_ALLPID, %d)\n", current->tgid, cmd->arg);
#endif
		break;

	case MPD_NOTICE_APNAME:
		YL_KMD_ASSERT(cmd->str);
		YL_KMD_ASSERT(cmd->str_len < MAX_STR_LEN);
		if (copy_from_user((void *)buf, (const void __user *)cmd->str, cmd->str_len) == 0) {
			buf[cmd->str_len] = 0;
			g_control.isDumpAllPID = 0;
			g_control.targetPID = 0;
#ifdef S_DEBUG_FLOW
			SDPF("[MPD %d]CMMPDControl(MPD_NOTICE_APNAME, %s)\n", current->tgid, buf);
#endif
		} else {
			SDPF("[MPD %d]CMMPDControl(MPD_NOTICE_APNAME) copy_from_user failed\n",
				current->tgid);
			break;
		}



#ifdef ANDROID
		if (strcmp(buf, g_control.targetAP) == 0) {
#else
		if (strstr(buf, g_control.targetAP)) {
#endif
			g_control.targetPID = current->tgid;
#ifdef S_DEBUG_FLOW
			SDPF("[MPD %d] trigger target ap %s\n", current->tgid, g_control.targetAP);
#endif
		}


		break;

	case MPD_QUERY_ENABLE:
		cmd->arg = g_shouldDump;
#ifdef S_DEBUG_FLOW
		SDPF("[MPD %d]CMMPDControl(MPD_QUERY_ENABLE), ret %d\n", current->tgid, cmd->arg);
#endif
		break;

	case MPD_QUERY_DUMP_PATH:
		if (copy_to_user
		    ((void __user *)(uintptr_t) cmd->str, (const void *)g_control.filePrefix,
		     strlen(g_control.filePrefix) + 1) == 0) {
			cmd->str_len = strlen(g_control.filePrefix);
#ifdef S_DEBUG_FLOW
			SDPF("[MPD %d]CMMPDControl(MPD_QUERY_DUMP_PATH), ret %s\n", current->tgid,
			     g_control.filePrefix);
#endif
		} else {
			SDPF("[MPD %d]CMMPDControl(MPD_QUERY_DUMP_PATH) copy_to_user failed\n",
			     current->tgid);
			break;
		}
		break;

	default:
		YL_KMD_ASSERT(0);
	}

}

void MPDReleaseUser(void)
{
	/* TODO: release user data */

}

void CMRiuCommand(int command, int data)
{
	static unsigned int control;
	void *fp;
	struct PID_Data *pid_data;

	int isTargetPID = g_control.isDumpAllPID || g_control.targetPID == current->tgid;
	int isTargetRange = !g_control.skipNumbers && (!g_control.isLimitDumpNumber
						       || g_control.dumpNumbers > 0);

	if (command == REG_AREG_G3D_ENG_FIRE)
		g_shouldDump = g_control.isEnableMPD && isTargetPID && isTargetRange;


	switch (command) {
	case 0x1F0C:
		saved_data[REG_1F0C] = data;
		break;
	case 0x1F20:
		saved_data[REG_1F20] = data;
		break;
	case 0x1198:
		saved_data[REG_1198] = data;
		break;
	case 0x119C:
		saved_data[REG_119C] = data;
		break;
	case 0x5000:
		saved_data[REG_5000] = data;
		break;
	case 0x5004:
		saved_data[REG_5004] = data;
		break;
	case 0x4180:
		saved_data[REG_4180] = data;
		break;
	case 0x1110:
		saved_data[REG_1110] = data;
		break;
	case 0x1114:
		saved_data[REG_1114] = data;
		break;

	case REG_AREG_CQ_CTRL:
		control = data;
		break;
	}

	if ((command == REG_AREG_G3D_ENG_FIRE) && (data == 1)
	    && (control & MSK_AREG_CQ_HTBUF_WPTR_UPD) && (g_control.skipNumbers > 0)) {
		g_globalCnt++;
		g_control.skipNumbers--;

	}

	if (!g_control.isEnableMPD || !isTargetPID || !isTargetRange)
		return;


#if defined(S_DEBUG_FLOW)
	SDPF("[MPD %d]CMRiuCommand(%04x, %08x)\n", current->tgid, command, data);
#endif

	fp = getRiuFD();

	g_pKmdFileIo->fPrintf(fp, "%04X_%08X\n", command, data);	/* Frank Add */

	StateTransition(command, data);

	if (REG_AREG_G3D_ENG_FIRE == command) {
		if (data == 1 && (control & MSK_AREG_CQ_HTBUF_WPTR_UPD)) {
			CMDumpAllKMemory();
			pid_data = getPIDData();

			/* TODO: check whether frame swap, if (framw_swap) {wpCnt=0;frameCnt++;} */
			pid_data->wpCnt++;

#if defined(S_DEBUG_IO)
			SDPF("[MPD %d]increse wpCnt=%d\n", current->tgid, pid_data->wpCnt);
#endif


		}
		closeRiuFD();


		g_globalCnt++;
		if (g_control.isLimitDumpNumber) {
			g_control.dumpNumbers--;

			if (g_control.dumpNumbers <= 0) {
				g_control.isLimitDumpNumber = 0;
				g_control.isEnableMPD = 0;
			}
		}

	}

}


void CMSetMemory_Kernel(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata)
{
	struct MPDMemblk *tmp;

#if defined(S_DEBUG_FLOW) || defined(S_DEBUG_DRAM)
	SDPF("[MPD %d]CMSetMemory_Kernel(PA=%08x, G3DVA=%08x, size=%x)\n", current->tgid, PA, G3DVA,
	     size);
#endif


	tmp = G3DKMDMALLOC(sizeof(struct MPDMemblk));
	if (tmp) {
		tmp->PA = PA;
		tmp->G3DVA = G3DVA;
		tmp->size = size;


		__SEM_LOCK(memBlockLock);
		list_add(&tmp->node, &g_KMemBlks);
		__SEM_UNLOCK(memBlockLock);

	}

}


void CMFreeMemory_Kernel(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata)
{
	struct list_head *iter, *safe;
	struct MPDMemblk *mem;

#if defined(S_DEBUG_DRAM)
	SDPF("[MPD %d]CMFreeMemory_Kernel(PA=%08x, G3DVA=%08x, size=%x)\n", current->tgid, PA,
	     G3DVA, size);
#endif

	__SEM_LOCK(memBlockLock);
	list_for_each_safe(iter, safe, &g_KMemBlks) {
		mem = list_entry(iter, struct MPDMemblk, node);

		if (mem->G3DVA == G3DVA) {
			list_del(&mem->node);
			G3DKMDFREE(mem);
			break;
		}

	}
	__SEM_UNLOCK(memBlockLock);
}


#endif /* SUPPORT_MPD */
