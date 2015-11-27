#ifndef _G3DKMD_SCHEDULER_H_
#define _G3DKMD_SCHEDULER_H_

#define G3DKMD_SCHEDULER_FREQ      10	// ms
#define G3DKMD_TIME_SLICE_BASE     10   // ms
#define G3DKMD_TIME_SLICE_INC      1   	// ms
#define G3DKMD_TIME_SLICE_NEW_THD  100  // ms

void g3dKmdCreateScheduler(void);
void g3dKmdReleaseScheduler(void);
void g3dKmdTriggerScheduler(void);
void g3dKmdContextSwitch(unsigned debug_info_enable);

#endif //_G3DKMD_SCHEDULER_H_
