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

#include <linux/interrupt.h>

#include "g3dkmd_api.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_signal.h"
#include "g3dkmd_file_io.h"

#ifdef G3DKMD_SIGNAL_CL_PRINT
static void do_cl_work(pid_t pid, unsigned int write_stamp)
{
	struct siginfo info;
	struct task_struct *pTask;

	pTask = pid_task(find_vpid(pid), PIDTYPE_PID);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_SIG, "Fire signal for CL printf\n");

	if (pTask == NULL || pid == 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_SIG, "pTask is NULL or pid is zero\n");
		return;
	}

	info.si_signo = SIGUSR2;
	info.si_pid = pid;
	info.si_code = -1;
	info.si_int = write_stamp;

	send_sig_info(SIGUSR2, &info, pTask);
}

void trigger_cl_printf_stamp(pid_t signal_pid, unsigned int write_stamp)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_SIG, "trigger with pid = %d, write_stamp = %d\n",
	       signal_pid, write_stamp);
	do_cl_work(signal_pid, write_stamp);
}

#endif /* G3DKMD_SIGNAL_CL_RPINT */

#ifdef G3DKMD_CALLBACK_INT
/*
 * This will get called by the kernel as soon as it's safe
 * to do everything normally allowed by kernel modules.
 */
static void do_work(pid_t pid)
{
	struct siginfo info;
	struct task_struct *pTask;

	pTask = pid_task(find_vpid(pid), PIDTYPE_PID); /* find_task_by_vpid(vpid); */

	/* pr_debug("workqueue pid = %d, task = %p\n", pid, pTask); */

	if (pTask == NULL || pid == 0)
		return;

	info.si_signo = SIGUSR1;
	info.si_pid = pid;
	info.si_code = -1; /* __SI_RT; */
	info.si_int = 0xabcd; /* task id; // user data */
	send_sig_info(SIGUSR1, &info, pTask);
}

#ifdef G3DKMD_CALLBACK_FROM_KEYBOARD_INT

irqreturn_t irq_handler(int irq, void *dev)
{
	do_work((pid_t) dev);
	return IRQ_HANDLED;
}

int init_sim_keyboard_int(pid_t signal_pid)
{
	/*
	 * Since the keyboard handler won't co-exist with another handler,
	 * such as us, we have to disable it (free its IRQ) before we do
	 * anything.  Since we don't know where it is, there's no way to
	 * reinstate it later - so the computer will have to be rebooted
	 * when we're done.
	 */
	free_irq(1, NULL);

	/*
	 * Request IRQ 1, the keyboard IRQ, to go to our irq_handler.
	 * SA_SHIRQ means we're willing to have othe handlers on this IRQ.
	 * SA_INTERRUPT can be used to make the handler into a fast interrupt.
	 */
	return request_irq(1, /* The number of the keyboard IRQ on PCs */
			   irq_handler, our handler * / IRQF_SHARED, "g3dkmd_keyboard_irq_handler",
			   (void *)signal_pid);
}

#elif defined(G3DKMD_CALLBACK_FROM_TIMER)
static struct timer_list recovery_timer;	/* Timer list. */
static unsigned int _ui4Delay = 100;	/* Timer interval (ms). */
static unsigned int _ui4Counter;	/* Counter. */

#define MAX_TRIGGER_COUNTER 10000
static void timer_callback(unsigned long param)
{
	if (_ui4Counter < MAX_TRIGGER_COUNTER) {/* shall be finite timer trigger otherwise yolig3d can't be updated. */
		do_work((pid_t) timer.data);

		timer.expires = jiffies + _ui4Delay * HZ / 1000;
		add_timer(&timer);
	} else {
		pr_debug("timer_callback finish\n");
	}


	++_ui4Counter;
}

void init_sim_timer(pid_t pid)
{
	init_timer(&timer);

	_ui4Counter = 0;
	timer.expires = jiffies + _ui4Delay * HZ / 1000;
	timer.function = timer_callback;
	timer.data = pid;

	add_timer(&timer);
}

#else

void trigger_callback(pid_t signal_pid)
{
	pr_debug("trigger pid = %d\n", signal_pid);
	do_work(signal_pid);
}


#endif /* G3DKMD_CALLBACK_FROM_TIMER */
#endif /* G3DKMD_CALLBACK_INT */
