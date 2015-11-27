#ifndef _G3DKMD_SIGNAL_H_
#define _G3DKMD_SIGNAL_H_

#ifdef G3DKMD_SIGNAL_CL_PRINT
void trigger_cl_printf_stamp(pid_t pid, unsigned int write_stamp);
#endif

#ifdef G3DKMD_CALLBACK_INT
int init_sim_keyboard_int(pid_t singal_pid);
void init_sim_timer(pid_t pid);
void trigger_callback(pid_t signal_pid);
#endif

#endif //_G3DKMD_SIGNAL_H_