#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h> /* for cpu_clock */

#include "gpad_common.h"
#include "gpad_proc.h"
#include "gpad_pm.h"
#include "gpad_sdbg_dump.h"
#if defined(GPAD_SUPPORT_DVFS)
#include "gpad_ipem.h"
#endif
#if defined(GPAD_SUPPORT_DFD)
#include "gpad_dfd.h"
#endif
#include "gpad_kmif.h"

#define GPAD_PROC_DIR           "gpad"
#define GPAD_PM_PROC_FILE       "pm"
#define GPAD_SDBG_PROC_FILE     "dump"

#if defined(GPAD_SUPPORT_DVFS)
#define GPAD_IPEM_PROC_FILE    "ipem"
#define GPAD_IPEM_LOG_PROC_FILE "ipem_log"
#define GPAD_IPEM_SBQS_PROC_FILE "ipem_sbqs"
#define GPAD_IPEM_SBEPS_PROC_FILE "ipem_sbeps"
#endif /* GPAD_SUPPORT_DVFS */

#define GPAD_DFD_AREG_PROC_FILE "areg"
#define GPAD_DFD_FREG_PROC_FILE "freg"
#define GPAD_DFD_FF_PROC_FILE   "ff"
#define GPAD_DFD_CMN_PROC_FILE  "ux_cmn"
#define GPAD_DFD_DWP_PROC_FILE  "ux_dwp"
#if defined(GPAD_SUPPORT_DFD)
#define GPAD_DFD_SRAM_PROC_FILE "sram"
#endif /* GPAD_SUPPORT_DFD */
#define BYTE3(_v) (unsigned char)(((_v) >> 24) & 0xff)
#define BYTE2(_v) (unsigned char)(((_v) >> 16) & 0xff)
#define BYTE1(_v) (unsigned char)(((_v) >> 8) & 0xff)
#define BYTE0(_v) (unsigned char)((_v) & 0xff)

static struct proc_dir_entry *gpad_proc_dir_s;
static struct proc_dir_entry *gpad_pm_proc_file_s;
static struct proc_dir_entry *gpad_sdbg_proc_file_s;

#if defined(GPAD_SUPPORT_DVFS)
static struct proc_dir_entry *gpad_ipem_proc_file_s;
static struct proc_dir_entry *gpad_ipem_log_proc_file_s;
static struct proc_dir_entry *gpad_ipem_sbqs_proc_file_s;
static struct proc_dir_entry *gpad_ipem_sbeps_proc_file_s;
#endif /* GPAD_SUPPORT_DVFS */

static struct proc_dir_entry *gpad_dfd_areg_proc_file_s;
static struct proc_dir_entry *gpad_dfd_freg_proc_file_s;
static struct proc_dir_entry *gpad_dfd_ff_proc_file_s;
static struct proc_dir_entry *gpad_dfd_cmn_proc_file_s;
static struct proc_dir_entry *gpad_dfd_dwp_proc_file_s;

#if defined(GPAD_SUPPORT_DFD)
static struct proc_dir_entry *gpad_dfd_sram_proc_file_s;
#endif /* GPAD_SUPPORT_DFD */

static int gpad_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_pm_info    *pm_info = &dev->pm_info;
	struct gpad_pm_stat    *stat = &pm_info->stat;
	G3DKMD_KMIF_MEM_DESC   *desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc;
	unsigned int            gpu_ts = gpad_pm_get_timestamp();
	unsigned long long      cpu_ts = cpu_clock(0);

	seq_printf(m, "GPAD v%X.%X.%X\n", BYTE3(GPAD_VERSION), BYTE2(GPAD_VERSION), BYTE1(GPAD_VERSION));
	seq_printf(m, "==============================\n");
	seq_printf(m, "device-%d : %p\n", MINOR(dev->cdev.dev), dev);
	seq_printf(m, "Ring HA   : 0x%08X, VA: %p, Index: %d/%d\n",    desc->hw_addr, desc->vaddr, pm_info->hw_buffer_idx,    pm_info->samples_alloc);
	seq_printf(m, "timestamp : 0x%08X, CPU: %llx\n", gpu_ts, cpu_ts);
	seq_printf(m, "PM sample : %d\n", stat->read_samples);
	seq_printf(m, "busy time : %d, total time: %d, GPU loading: %d\n",    stat->busy_time, stat->total_time,    ((stat->total_time/100 > 0) ? ((stat->busy_time) / (stat->total_time / 100)) : 0));
	seq_printf(m, "ext_id_reg: 0x%08X\n", dev->ext_id_reg);
	seq_printf(m, "vertex    : %d\n", stat->vertex_cnt);
	seq_printf(m, "triangle  : %d\n", stat->triangle_cnt);
	seq_printf(m, "pixel     : %d\n", stat->pixel_cnt);
	seq_printf(m, "frame     : %d\n", stat->frame_cnt);

#if defined(GPAD_DEBUG_TIMESTAMP)
	{
		static unsigned int            last_gpu_ts;
		static unsigned long long      last_cpu_ts;
		int                            diff_gpu_ts;
		long long                      diff_cpu_ts;
		long long                      diff_ns;
		long long                      diff_ns_x;
		long long                      delta;

		if (last_gpu_ts) {
			diff_gpu_ts = (int)gpu_ts - (int)last_gpu_ts;
			diff_cpu_ts = (long long)cpu_ts - (long long)last_cpu_ts;
			diff_ns = (long long)diff_gpu_ts;
			diff_ns *= 1000000000;
			diff_ns_x = diff_ns;
			do_div(diff_ns, pm_info->gpu_freq);
			delta = diff_ns - diff_cpu_ts;

			seq_printf(m, "Diff GPU: %d, CPU: %lld\n", diff_gpu_ts, diff_cpu_ts);
			seq_printf(m, "=> %lld => GPU2ns=%lld ns, delta=%lld ns, gpu_freq: %d\n",    diff_ns_x, diff_ns, delta, pm_info->gpu_freq);
		}
		last_gpu_ts = gpu_ts;
		last_cpu_ts = cpu_ts;
	}
#endif /* GPAD_DEBUG_TIMESTAMP */

	return 0;
}
/** DFD **/
static int gpad_dfd_areg_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *dfd_info = &dev->sdbg_info;
	if (dfd_info->level == DUMP_DBGREG_LEVEL)
		gpad_dfd_dump_areg_all_reg(m, dfd_info);
	return 0;
}

static int gpad_dfd_freg_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *dfd_info = &dev->sdbg_info;
	if (dfd_info->level == DUMP_DBGREG_LEVEL)
		gpad_dfd_dump_freg_reg(m, dfd_info);
	return 0;
}

static int gpad_dfd_ff_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *dfd_info = &dev->sdbg_info;
	if (dfd_info->level == DUMP_DBGREG_LEVEL)
		gpad_dfd_dump_ff_debug_reg(m, dfd_info);
	return 0;
}

static int gpad_dfd_cmn_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *dfd_info = &dev->sdbg_info;
	if (dfd_info->level == DUMP_DBGREG_LEVEL)
		gpad_dfd_dump_ux_cmn_reg(m, dfd_info);
	return 0;
}

static int gpad_dfd_dwp_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *dfd_info = &dev->sdbg_info;
	if (dfd_info->level == DUMP_DBGREG_LEVEL)
		gpad_dfd_dump_dwp_debug_reg(m, dfd_info);
	return 0;
}

#if defined(GPAD_SUPPORT_DFD)
static int gpad_dfd_sram_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_dfd_info  *dfd_info = &dev->dfd_info;
	if (dfd_info->dump_sram)
		gpad_dfd_dump_sram(m, dfd_info);
	return 0;
}
#endif /* GPAD_SUPPORT_DFD */

/* ******* */
static int gpad_sdbg_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_sdbg_info  *sdbg_info = &dev->sdbg_info;
	seq_printf(m, "dump level    : %d\n", sdbg_info->level);
	seq_printf(m, "dump start    : %lld\n", sdbg_info->dump_timestamp_start);
	seq_printf(m, "dump end      : %lld\n", sdbg_info->dump_timestamp_end);
	gpad_sdbg_dump_areg(m, sdbg_info);

	/* Dump common register */
	gpad_sdbg_dump_common_reg(m, sdbg_info);

	/* Dump dualpool register */
	gpad_sdbg_dump_dualwavepool_reg(m, sdbg_info);

	/* Dump global register */
	gpad_sdbg_dump_global_reg(m, sdbg_info);

	/* Dump debug register */
	if (sdbg_info->level == DUMP_CORE_LEVEL || sdbg_info->level == DUMP_VERBOSE_LEVEL) {
		seq_printf(m, "==== Dump Debug Register ====\n");
		gpad_dfd_dump_dwp_debug_reg(m, sdbg_info);
	}

	if (sdbg_info->level == DUMP_VERBOSE_LEVEL) {
		/* Dump instruction */
		gpad_sdbg_dump_instruction_cache(m, sdbg_info);

		/* Dump wave information if read wave is enabled */
		if (unlikely(sdbg_info->wave_readable_flag != 0)) {
#define GPAD_SDBG_MAX_WAVEPOOL_COUNT  (2)
#define GPAD_SDBG_MAX_WAVE_COUNT      (12)
			int i, wpid, wid;
			seq_printf(m, "==== Dump Wave Information ====\n");
			for (i = 0; i < GPAD_SDBG_MAX_WAVEPOOL_COUNT; i++) {
				for (wid = 0; wid < GPAD_SDBG_MAX_WAVE_COUNT; wid++) {
					wpid = i * 2; /* available wapid: 0, 2 */
					gpad_sdbg_dump_wave_info(m, wpid, wid, sdbg_info);
				}
			}
		}
	}
	return 0;
}
/** DFD **/
static int gpad_dfd_areg_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_areg_proc_show, NULL);
}

static int gpad_dfd_freg_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_freg_proc_show, NULL);
}

static int gpad_dfd_ff_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_ff_proc_show, NULL);
}

static int gpad_dfd_cmn_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_cmn_proc_show, NULL);
}

static int gpad_dfd_dwp_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_dwp_proc_show, NULL);
}

#if defined(GPAD_SUPPORT_DFD)
static int gpad_dfd_sram_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_dfd_sram_proc_show, NULL);
}
#endif /* GPAD_SUPPORT_DFD */

/*********/

#if defined(GPAD_SUPPORT_DVFS)
static int gpad_ipem_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device *dev;
	unsigned int enable;
	unsigned long interval;
	unsigned int table_id;
	unsigned int dvfs_policy_id;
	unsigned int dpm_policy_id;
	struct gpad_ipem_dvfs_policy_info *dvfs_policies;
	struct gpad_ipem_dpm_policy_info *dpm_policies;

	dev = gpad_get_default_dev();

	enable = gpad_ipem_enable_get(dev);
	seq_printf(m, "enable:\t\t%u\n", enable);

	interval = gpad_ipem_timer_get(dev);
	interval = ((1000 * interval) / HZ);
	seq_printf(m, "interval:\t%lu (ms)\n", interval);

	table_id = gpad_ipem_table_get(dev);
	seq_printf(m, "table_id:\t%u\n", table_id);

	dvfs_policy_id = gpad_ipem_dvfs_policy_get(dev);
	dvfs_policies = gpad_ipem_dvfs_policies_get(dev);
	seq_printf(m, "dvfs_policy_id:\t%u (%s)\n", dvfs_policy_id, dvfs_policies[dvfs_policy_id].name);

	dpm_policy_id = gpad_ipem_dpm_policy_get(dev);
	dpm_policies = gpad_ipem_dpm_policies_get(dev);
	seq_printf(m, "dpm_policy_id:\t%u (%s)\n", dpm_policy_id, dpm_policies[dpm_policy_id].name);

	return 0;
}

static int gpad_ipem_log_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	unsigned int recordid;
	unsigned int recordit;
	gpad_ipem_action action;
	struct mtk_gpufreq_info *table;
	struct gpad_ipem_record_info *record;
	struct gpad_ipem_statistic_info *stat;
	USE_IPEM_LOCK;

	dev = gpad_get_default_dev();
	ipem_info = GPAD_IPEM_INFO_GET(dev);

	table = ipem_info->table_info.table;

	seq_printf(m, "[timestamp(ns)]\t[frequency(khz)]\t[voltage(mv)]\t[workid]\t[event]\t[action_llimit]\t[action_ulimit]\t[action_prev]\t[gpuload]\t[flush]\t[ht]\t[idle]\t[reward]\t[policy_do]\t[policy_update]\t[state]\t[action]\n");

	LOCK_IPEM();
	recordid = ipem_info->recorder_info.recordid;
	recordit = recordid;

	do {
		record = &ipem_info->recorder_info.records[recordit];
		stat = &record->statistic_info;
		action = stat->action;

		seq_printf(m, "%lu\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%p\t%p\t%u\t%u\n",    stat->timestamp,    table[action].freq_khz,    table[action].volt == 0 ? 0 : table[action].volt == GPU_POWER_VGPU_1_15V ? 1150 : table[action].volt == GPU_POWER_VGPU_1_25V ? 1250 : -1,    record->workid,    stat->event,    stat->action_llimit,    stat->action_ulimit,    stat->action_prev,    stat->gpuload,    stat->thermal,    stat->touch,    stat->flush,    stat->ht,    stat->idle,    stat->reward,    stat->policy_do,    stat->policy_update,    stat->state,    stat->action);

		recordit++;
		if (recordit >= (GPAD_IPEM_RECORDER_RECORDS_SIZE / sizeof(struct gpad_ipem_record_info)))
			recordit = 0;
	} while (recordit != recordid);
	UNLOCK_IPEM();

	return 0;
}

static int gpad_ipem_sbqs_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	unsigned int qssz;
	unsigned int qsit;
	gpad_ipem_fixedpt *qs;
	USE_IPEM_LOCK;

	dev = gpad_get_default_dev();
	ipem_info = GPAD_IPEM_INFO_GET(dev);

	qs = ipem_info->scoreboard_info.qs;

	LOCK_IPEM();
	qssz = (GPAD_IPEM_SCOREBOARD_QS_SIZE / sizeof(gpad_ipem_fixedpt));
	for (qsit = 0; qsit < qssz; qsit++)
		seq_printf(m, "0x%08x\n", qs[qsit]);
	UNLOCK_IPEM();

	return 0;
}

static int gpad_ipem_sbeps_proc_show(struct seq_file *m, void *v)
{
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	unsigned int epssz;
	unsigned int epsit;
	gpad_ipem_fixedpt *eps;
	USE_IPEM_LOCK;

	dev = gpad_get_default_dev();
	ipem_info = GPAD_IPEM_INFO_GET(dev);

	eps = ipem_info->scoreboard_info.epsilons;

	LOCK_IPEM();
	epssz = (GPAD_IPEM_SCOREBOARD_EPSILONS_SIZE / sizeof(gpad_ipem_fixedpt));
	for (epsit = 0; epsit < epssz; epsit++)
		seq_printf(m, "0x%08x\n", eps[epsit]);
	UNLOCK_IPEM();

	return 0;
}


static int gpad_ipem_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_ipem_proc_show, NULL);
}

static int gpad_ipem_log_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_ipem_log_proc_show, NULL);
}

static int gpad_ipem_sbqs_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_ipem_sbqs_proc_show, NULL);
}

static int gpad_ipem_sbeps_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_ipem_sbeps_proc_show, NULL);
}
#endif /* GPAD_SUPPORT_DVFS */

static int gpad_sdbg_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_sdbg_proc_show, NULL);
}

static int gpad_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, gpad_proc_show, NULL);
}

#if defined(GPAD_SUPPORT_DVFS)
static ssize_t gpad_ipem_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
	struct gpad_device *dev;
	unsigned int    enable;
	unsigned long   interval;
	unsigned int    table_id;
	unsigned int dvfs_policy_id;
	unsigned int dpm_policy_id;

	dev = gpad_get_default_dev();

	if (sscanf(buf, "%u %lu %u %u %u", &enable, &interval, &table_id, &dvfs_policy_id, &dpm_policy_id) == 5) {
		interval = ((HZ * interval) / 1000);
		gpad_ipem_timer_set(dev, interval);

		gpad_ipem_table_set(dev, table_id);

		gpad_ipem_dvfs_policy_set(dev, dvfs_policy_id);

		gpad_ipem_dpm_policy_set(dev, dpm_policy_id);

		if (enable == 0) {
			gpad_ipem_disable(dev);
		} else if (enable == 1) {
			/* gpad_ipem_enable(dev, 0); */
			gpad_ipem_enable(dev);
		} else {
			GPAD_LOG_INFO("[IPEM][PROC] invalid argument, usage: enable = [0/1]\n");
		}
	} else {
		GPAD_LOG_INFO("[IPEM][PROC] invalid argument, usage: [enable interval table_id dvfs_policy_id dpm_policy_id]\n");
	}

	return len;
}

static ssize_t gpad_ipem_sbqs_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	unsigned int qssz;
	unsigned int qsit;
	gpad_ipem_fixedpt q;
	gpad_ipem_fixedpt *qs;
	USE_IPEM_LOCK;
	char proc_buffer[20] = {0};
	int i;

	if (len > (sizeof(proc_buffer) - 1)) {
		GPAD_LOG_INFO("[IPEM][SBQS][PROC] input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < len; ++i) {
		proc_buffer[i] = buf[i];
	}
	proc_buffer[i] = 0;

	dev = gpad_get_default_dev();
	ipem_info = GPAD_IPEM_INFO_GET(dev);

	qs = ipem_info->scoreboard_info.qs;

	/* FIXME: the correct proc write */
	LOCK_IPEM();
	qssz = (GPAD_IPEM_SCOREBOARD_QS_SIZE / sizeof(gpad_ipem_fixedpt));
	for (qsit = 0; qsit < qssz; qsit++) {
		if (kstrtoint((const char *) proc_buffer, 16, &q) == 0)
			qs[qsit] = q;
		else
			GPAD_LOG_INFO("[IPEM][SBQS][PROC] invalid argument, usage: q\\n\n");
	}
	UNLOCK_IPEM();

	return len;
}

static ssize_t gpad_ipem_sbeps_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	unsigned int epssz;
	unsigned int epsit;
	gpad_ipem_fixedpt ep;
	gpad_ipem_fixedpt *eps;
	USE_IPEM_LOCK;
	char proc_buffer[20] = {0};
	int i;

	if (len > (sizeof(proc_buffer) - 1)) {
		GPAD_LOG_INFO("[IPEM][SBEPS][PROC] input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < len; ++i) {
		proc_buffer[i] = buf[i];
	}
	proc_buffer[i] = 0;

	dev = gpad_get_default_dev();
	ipem_info = GPAD_IPEM_INFO_GET(dev);

	eps = ipem_info->scoreboard_info.epsilons;

	/* FIXME: the correct proc write */
	LOCK_IPEM();
	epssz = (GPAD_IPEM_SCOREBOARD_EPSILONS_SIZE / sizeof(gpad_ipem_fixedpt));
	for (epsit = 0; epsit < epssz; epsit++) {
		if (kstrtoint((const char *) proc_buffer, 16, &ep) == 0)
			eps[epsit] = ep;
		else
			GPAD_LOG_INFO("[IPEM][SBEPS][PROC] invalid argument, usage: ep\\n\n");
	}
	UNLOCK_IPEM();

	return len;
}


void gpad_ipem_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .write   = gpad_ipem_proc_write, .open    = gpad_ipem_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_ipem_proc_file_s = proc_create(GPAD_IPEM_PROC_FILE, S_IRUGO|S_IWOTH, gpad_proc_dir_s, &fops);
	if (likely(gpad_ipem_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_PROC_FILE);
}

void gpad_ipem_log_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_ipem_log_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_ipem_log_proc_file_s = proc_create(GPAD_IPEM_LOG_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_ipem_log_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_LOG_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_LOG_PROC_FILE);
}

void gpad_ipem_sbqs_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .write   = gpad_ipem_sbqs_proc_write, .open    = gpad_ipem_sbqs_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_ipem_log_proc_file_s = proc_create(GPAD_IPEM_SBQS_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_ipem_log_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBQS_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBQS_PROC_FILE);
}

void gpad_ipem_sbeps_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .write   = gpad_ipem_sbeps_proc_write, .open    = gpad_ipem_sbeps_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_ipem_log_proc_file_s = proc_create(GPAD_IPEM_SBEPS_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_ipem_log_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBEPS_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBEPS_PROC_FILE);
}
#endif /* GPAD_SUPPORT_DVFS */

/** DFD **/
void gpad_dfd_areg_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_areg_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_areg_proc_file_s = proc_create(GPAD_DFD_AREG_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_areg_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_AREG_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_AREG_PROC_FILE);
}

void gpad_dfd_freg_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_freg_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_freg_proc_file_s = proc_create(GPAD_DFD_FREG_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_freg_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FREG_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FREG_PROC_FILE);
}

void gpad_dfd_ff_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_ff_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_ff_proc_file_s = proc_create(GPAD_DFD_FF_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_ff_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FF_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FF_PROC_FILE);
}

void gpad_dfd_cmn_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_cmn_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_cmn_proc_file_s = proc_create(GPAD_DFD_CMN_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_cmn_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_CMN_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_CMN_PROC_FILE);
}

void gpad_dfd_dwp_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_dwp_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_dwp_proc_file_s = proc_create(GPAD_DFD_DWP_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_dwp_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_DWP_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_DWP_PROC_FILE);
}

#if defined(GPAD_SUPPORT_DFD)
void gpad_dfd_sram_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_dfd_sram_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_dfd_sram_proc_file_s = proc_create(GPAD_DFD_SRAM_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_dfd_sram_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_SRAM_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_SRAM_PROC_FILE);
}
#endif /* GPAD_SUPPORT_DFD */

/*********/

void gpad_sdbg_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_sdbg_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_sdbg_proc_file_s = proc_create(GPAD_SDBG_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_sdbg_proc_file_s))
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_SDBG_PROC_FILE);
	else
		GPAD_LOG_ERR("failed create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_SDBG_PROC_FILE);
}

void __init gpad_proc_init(void)
{
	static const struct file_operations fops = {
		.owner   = THIS_MODULE, .open    = gpad_proc_open, .read    = seq_read, .llseek  = seq_lseek, .release = single_release, };

	gpad_proc_dir_s = proc_mkdir(GPAD_PROC_DIR, NULL);
	if (unlikely(!gpad_proc_dir_s)) {
		GPAD_LOG_ERR("failed to create /proc/%s\n", GPAD_PROC_DIR);
		return;
	}

	gpad_pm_proc_file_s = proc_create(GPAD_PM_PROC_FILE, S_IRUGO, gpad_proc_dir_s, &fops);
	if (likely(gpad_pm_proc_file_s)) {
		GPAD_LOG_INFO("create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_PM_PROC_FILE);
	} else {
		GPAD_LOG_ERR("failed to create /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_PM_PROC_FILE);
		return;
	}

	/* sdbg proc */
	gpad_sdbg_proc_init();
	/* dfd proc */
	gpad_dfd_areg_init();
	gpad_dfd_freg_init();
	gpad_dfd_ff_init();
	gpad_dfd_cmn_init();
	gpad_dfd_dwp_init();

#if defined(GPAD_SUPPORT_DFD)
	gpad_dfd_sram_init();
#endif /* GPAD_SUPPORT_DFD */

#if defined(GPAD_SUPPORT_DVFS)
	/* ipem proc */
	gpad_ipem_proc_init();
	gpad_ipem_log_proc_init();
	gpad_ipem_sbqs_proc_init();
	gpad_ipem_sbeps_proc_init();
#endif /* GPAD_SUPPORT_DVFS */
}

/** DFD **/
void gpad_dfd_areg_exit(void)
{
	if (likely(gpad_dfd_areg_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_AREG_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_areg_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_AREG_PROC_FILE);
	}
}

void gpad_dfd_freg_exit(void)
{
	if (likely(gpad_dfd_freg_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_FREG_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_freg_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FREG_PROC_FILE);
	}
}

void gpad_dfd_ff_exit(void)
{
	if (likely(gpad_dfd_ff_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_FF_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_ff_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_FF_PROC_FILE);
	}
}

void gpad_dfd_cmn_exit(void)
{
	if (likely(gpad_dfd_cmn_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_CMN_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_cmn_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_CMN_PROC_FILE);
	}
}

void gpad_dfd_dwp_exit(void)
{
	if (likely(gpad_dfd_dwp_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_DWP_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_dwp_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_DWP_PROC_FILE);
	}
}

#if defined(GPAD_SUPPORT_DFD)
void gpad_dfd_sram_exit(void)
{
	if (likely(gpad_dfd_sram_proc_file_s)) {
		remove_proc_entry(GPAD_DFD_SRAM_PROC_FILE, gpad_proc_dir_s);
		gpad_dfd_sram_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_DFD_SRAM_PROC_FILE);
	}
}
#endif /* GPAD_SUPPORT_DFD */

/*********/

#if defined(GPAD_SUPPORT_DVFS)
void gpad_ipem_proc_exit(void)
{
	if (likely(gpad_ipem_proc_file_s)) {
		remove_proc_entry(GPAD_IPEM_PROC_FILE, gpad_proc_dir_s);
		gpad_ipem_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_PROC_FILE);
	}
}

void gpad_ipem_log_proc_exit(void)
{
	if (likely(gpad_ipem_log_proc_file_s)) {
		remove_proc_entry(GPAD_IPEM_LOG_PROC_FILE, gpad_proc_dir_s);
		gpad_ipem_log_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_LOG_PROC_FILE);
	}
}

void gpad_ipem_sbqs_proc_exit(void)
{
	if (likely(gpad_ipem_sbqs_proc_file_s)) {
		remove_proc_entry(GPAD_IPEM_SBQS_PROC_FILE, gpad_proc_dir_s);
		gpad_ipem_sbqs_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBQS_PROC_FILE);
	}
}

void gpad_ipem_sbeps_proc_exit(void)
{
	if (likely(gpad_ipem_sbeps_proc_file_s)) {
		remove_proc_entry(GPAD_IPEM_SBEPS_PROC_FILE, gpad_proc_dir_s);
		gpad_ipem_sbeps_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_IPEM_SBEPS_PROC_FILE);
	}
}
#endif /* GPAD_SUPPORT_DVFS */

void gpad_sdbg_proc_exit(void)
{
	if (likely(gpad_sdbg_proc_file_s)) {
		remove_proc_entry(GPAD_SDBG_PROC_FILE, gpad_proc_dir_s);
		gpad_sdbg_proc_file_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_SDBG_PROC_FILE);
	}
}

void gpad_proc_exit(void)
{
	if (likely(gpad_proc_dir_s)) {
#if defined(GPAD_SUPPORT_DVFS)
		gpad_ipem_proc_exit();
		gpad_ipem_log_proc_exit();
		gpad_ipem_sbqs_proc_exit();
		gpad_ipem_sbeps_proc_exit();
#endif /* GPAD_SUPPORT_DVFS */

#if defined(GPAD_SUPPORT_DFD)
		gpad_dfd_sram_exit();
#endif /* GPAD_SUPPORT_DFD */

		gpad_sdbg_proc_exit();
		gpad_dfd_areg_exit();
		gpad_dfd_freg_exit();
		gpad_dfd_ff_exit();
		gpad_dfd_cmn_exit();
		gpad_dfd_dwp_exit();

		if (likely(gpad_pm_proc_file_s)) {
			remove_proc_entry(GPAD_PM_PROC_FILE, gpad_proc_dir_s);
			gpad_pm_proc_file_s = NULL;
			GPAD_LOG_INFO("remove /proc/%s/%s\n", GPAD_PROC_DIR, GPAD_PM_PROC_FILE);
		}

		remove_proc_entry(GPAD_PROC_DIR, NULL);
		gpad_proc_dir_s = NULL;
		GPAD_LOG_INFO("remove /proc/%s\n", GPAD_PROC_DIR);
	}
}
