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

#ifndef _GPAD_KMIF_H
#define _GPAD_KMIF_H

/*!
 * GPU top level Performance Monitor counters.
 */
typedef struct _G3DKMD_KMIF_TOP_PM_COUNTERS_ {
	unsigned int     gpu_loading; /**< GPU busy percentage. */
	unsigned int     frame_cnt;   /**< Frame count. */
	unsigned int     vertex_cnt;  /**< Vertex count. */
	unsigned int     pixel_cnt;   /**< Pixpel count. */
	unsigned int     time;        /**< Time measured in ns. */
} G3DKMD_KMIF_TOP_PM_COUNTERS;


/*!
 * Callback functions to register with g3dkmd.
 */
typedef struct _G3DKMD_KMIF_OPS_ {
	void     (*notify_g3d_hang)(void);  /**< Called when g3d hang happens. */
	void     (*notify_g3d_reset)(void); /**< Called after g3d reset completes. */
	void     (*notify_g3d_early_suspend)(void); /**< Called when entering early-suspend. */
	void     (*notify_g3d_late_resume)(void); /**< Called when entering late-resume. */
	void     (*notify_g3d_power_low)(void); /**< Called when turning low GPU power. */
	void     (*notify_g3d_power_high)(void); /**< Called when turning high GPU power. */
	void     (*notify_g3d_power_off)(void); /**< Called when turning off GPU power. */
	void     (*notify_g3d_power_on)(void); /**< Called when turning on GPU power. */
	void     (*notify_g3d_power_off_begin)(void); /**< Called when turning off GPU power. */
	void     (*notify_g3d_power_off_end)(void); /**< Called when turning off GPU power. */
	void     (*notify_g3d_power_on_begin)(void); /**< Called when turning on GPU power. */
	void     (*notify_g3d_power_on_end)(void); /**< Called when turning on GPU power. */
	void     (*notify_g3d_dfd_interrupt)(unsigned int); /**< Called in interrupt tasklet when DFD is enabled. */
	void     (*notify_g3d_performance_mode)(unsigned int); /**< Reference counted performance mode. Input : 0 or 1 */

	void     (*get_top_pm_counters)(G3DKMD_KMIF_TOP_PM_COUNTERS *);
	/**< Query latest top level PM counter values. */
} G3DKMD_KMIF_OPS;

/*!
 * Memory descriptor of a memory allocated for HW access.
 */
typedef struct _G3DKMD_KMIF_MEM_DESC_ {
	unsigned int    size;       /**< Length of buffer. */
	unsigned int    hw_addr;    /**< MMU mapped address for HW access. */
	void           *vaddr;      /**< Virtual address of the buffer. */
} G3DKMD_KMIF_MEM_DESC;

/*!
 * Register gpad module with g3dkmd module.
 *
 * @param ops   Calblacks function to register with g3dkmd.
 *
 * @return 0 if succeeded, error code otherwise.
 */
extern int g3dKmdKmifRegister(G3DKMD_KMIF_OPS *ops);

/*!
 * Deregister gpad module with g3dkmd module.
 *
 * @return 0 if succeeded, error code otherwise.
 */
extern int g3dKmdKmifDeregister(void);

/*!
 * Allocate memory on specified alignment boundry for HW access.
 *
 * @param size   Bytes of the requested memory allocation.
 * @param align  The alignment value, which must be power of 2.
 *
 * @return Pointer to the memory descriptor if succeeded, NULL otherwise.
 */
extern G3DKMD_KMIF_MEM_DESC *g3dKmdKmifAllocMemory(unsigned int size, unsigned int align);

/*!
 * Free memory allocated by g3dKmdKmifAllocMemory().
 *
 * @param desc   Memory descritor to free.
 *
 * @return Pointer to the memory descriptor if succeeded, NULL otherwise.
 */
extern void g3dKmdKmifFreeMemory(G3DKMD_KMIF_MEM_DESC *desc);

/*!
 * Query G3D register base address.
 *
 * @return G3D register base address in kernel logical address.
 */
extern unsigned char *g3dKmdKmifGetRegBase(void);

/*!
 * Query MMU  register base address.
 *
 * @return MMU register base address in kernel logical address.
 */
extern unsigned char *g3dKmdKmifGetMMURegBase(void);

/*!
 * Translate MVA to PA.
 *
 * @return PA of the input MVA (query from m4u).
 */
unsigned int g3dKmdKmifMvaToPa(unsigned int mva);

/*!
 * Notify the shader is in debug mode or not.
 * Note that, in shader debug mode:
 * 1. gpad module won't get g3d hang event notifiation.
 * 2. g3dkmd can work as usual without doing any hw recovery, even it got g3d hang interrupt.
 *
 * @param enable 1 if we are in shader debug mode, 0 otherwise.
 */
extern void g3dKmdKmifSetShaderDebugMode(int enable);

/*!
 * Notify the dfd is in debug mode or not.
 * Note that, in dfd debug mode:
 * 1. g3dkmd should mask internal dump interrupt
 *
 * @param enable 1 if we are in dfd debug mode, 0 otherwise.
 */
extern void g3dKmdKmifSetDFDDebugMode(int enable);

/*!
 * Allocate memory on specified alignment boundry for HW access for DFD.
 *
 * @param size   Bytes of the requested memory allocation.
 * @param align  The alignment value, which must be power of 2.
 *
 * @return Pointer to the memory descriptor if succeeded, NULL otherwise.
 */
extern G3DKMD_KMIF_MEM_DESC *g3dKmdKmifDFDAllocMemory(unsigned int size, unsigned int align);

/*!
 * Free memory allocated by g3dKmdKmifDFDAllocMemory().
 *
 * @param desc   Memory descritor to free.
 *
 * @return Pointer to the memory descriptor if succeeded, NULL otherwise.
 */
extern void g3dKmdKmifDFDFreeMemory(G3DKMD_KMIF_MEM_DESC *desc);

/*!
 * Query GPU driver's memory usage.
 *
 * @return GPU driver memory usage.
 */
extern unsigned int g3dKmdKmifQueryMemoryUsage_MET(void);

/*!
 * Enable perf mode
 *
 */
extern void g3dKmdPerfModeEnable(void);

/*!
 * Disable perf mode
 *
 */
extern void g3dKmdPerfModeDisable(void);

#endif /* _GPAD_KMIF_H */
