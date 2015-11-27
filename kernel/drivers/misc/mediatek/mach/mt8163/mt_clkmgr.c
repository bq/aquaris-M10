#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>

#include <mach/mt_typedefs.h>
#include <mach/sync_write.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_dcm.h>
#include <mach/mt_spm.h>
#include <mach/mt_spm_mtcmos.h>
#include <mach/mt_spm_sleep.h>
#include <mach/mt_freqhopping.h>
#include <mach/mt_gpufreq.h>
#include <mach/irqs.h>

#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#ifdef CONFIG_OF
void __iomem *clk_apmixed_base;
void __iomem *clk_cksys_base;
void __iomem *clk_infracfg_ao_base;
void __iomem *clk_audio_base;
void __iomem *clk_mfgcfg_base;
void __iomem *clk_mmsys_config_base;
void __iomem *clk_imgsys_base;
void __iomem *clk_vdec_gcon_base;
void __iomem *clk_venc_gcon_base;
#endif

/* #define CLK_LOG_TOP */
/* #define CLK_LOG */
/* #define DISP_CLK_LOG */
/* #define SYS_LOG */
/* #define MUX_LOG_TOP */
/* #define MUX_LOG */
/* #define PLL_LOG_TOP */
/* #define PLL_LOG */

/* #define Bring_Up */
#define PLL_CLK_LINK

#define FMETER		1
#define NEW_MUX		1
#define MSDC_WORKAROUND	0
#define ALL_CLK_ON	0

/************************************************
 **********         log debug          **********
 ************************************************/

#define USING_XLOG

#ifdef USING_XLOG
#include <linux/xlog.h>

#define TAG	"Power/clkmgr"

#define clk_err(fmt, args...) xlog_printk(ANDROID_LOG_ERROR, TAG, fmt, ##args)
#define clk_warn(fmt, args...) xlog_printk(ANDROID_LOG_WARN, TAG, fmt, ##args)
#define clk_info(fmt, args...) xlog_printk(ANDROID_LOG_INFO, TAG, fmt, ##args)
#define clk_dbg(fmt, args...) xlog_printk(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define clk_ver(fmt, args...) xlog_printk(ANDROID_LOG_VERBOSE, TAG, fmt, ##args)

#else /* USING_XLOG */

#define TAG	"[Power/clkmgr] "

#define clk_err(fmt, args...) pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...) pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...) pr_notice(TAG fmt, ##args)
#define clk_dbg(fmt, args...) pr_info(TAG fmt, ##args)
#define clk_ver(fmt, args...) pr_debug(TAG fmt, ##args)

#endif

/************************************************
 **********      register access       **********
 ************************************************/

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)
#define clk_setl(addr, val) mt_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val) mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

/************************************************
 **********      struct definition     **********
 ************************************************/

/* #define CONFIG_CLKMGR_STAT */

struct pll;
struct pll_ops {
	int (*get_state)(struct pll *pll);
	void (*enable)(struct pll *pll);
	void (*disable)(struct pll *pll);
	void (*fsel)(struct pll *pll, unsigned int value);
	int (*dump_regs)(struct pll *pll, unsigned int *ptr);
	int (*hp_enable)(struct pll *pll);
	int (*hp_disable)(struct pll *pll);
};

struct pll {
	const char *name;
	int type;
	int mode;
	int feat;
	int state;
	unsigned int cnt;
	unsigned int en_mask;
	void __iomem *base_addr;
	void __iomem *pwr_addr;
	struct pll_ops *ops;
	unsigned int hp_id;
	int hp_switch;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};

struct subsys;
struct subsys_ops {
	int (*enable)(struct subsys *sys);
	int (*disable)(struct subsys *sys);
	int (*get_state)(struct subsys *sys);
	int (*dump_regs)(struct subsys *sys, unsigned int *ptr);
};

struct subsys {
	const char *name;
	int type;
	int force_on;
	unsigned int cnt;
	unsigned int state;
	unsigned int default_sta;
	unsigned int sta_mask;
	void __iomem *ctl_addr;
	struct subsys_ops *ops;
	struct cg_grp *start;
	unsigned int nr_grps;
	struct clkmux *mux;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};

#if NEW_MUX

enum mux_id_t {
	MUX_NULL,

	/* CLK_CFG_0 */
	MUX_AXI,
	MUX_MEM,
	MUX_DDRPHY,
	MUX_MM,

	/* CLK_CFG_1 */
	MUX_PWM,
	MUX_VDEC,
	MUX_MFG,

	/* CLK_CFG_2 */
	MUX_CAMTG,
	MUX_UART,
	MUX_SPI,

	/* CLK_CFG_3 */
	MUX_MSDC30_0,
	MUX_MSDC30_1,
	MUX_MSDC30_2,

	/* CLK_CFG_4 */
	MUX_MSDC50_3_HCLK,
	MUX_MSDC50_3,
	MUX_AUDIO,
	MUX_AUDINTBUS,

	/* CLK_CFG_5 */
	MUX_PMICSPI,
	MUX_SCP,
	MUX_ATB,
	MUX_MJC,

	/* CLK_CFG_6 */
	MUX_DPI0,
	MUX_SCAM,
	MUX_AUD1,
	MUX_AUD2,

	/* CLK_CFG_7 */
	MUX_DPI1,
	MUX_UFOENC,
	MUX_UFODEC,
	MUX_ETH,

	/* CLK_CFG_8 */
	MUX_ONFI,
	MUX_SNFI,
	MUX_HDMI,
	MUX_RTC,

	MUX_END
};

struct clkmux {
	enum mux_id_t mux_id;
	const char *name;
	unsigned int cnt;
	struct clkmux *siblings;
	struct pll *pll;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};

#else /* !NEW_MUX */

struct clkmux;

struct clkmux_ops {
	void (*sel)(struct clkmux *mux, unsigned int clksrc);
	void (*enable)(struct clkmux *mux);
	void (*disable)(struct clkmux *mux);
};

struct clkmux {
	const char *name;
	unsigned int cnt;
	void __iomem *base_addr;
	unsigned int sel_mask;
	unsigned int pdn_mask;
	unsigned int offset;
	unsigned int nr_inputs;
	unsigned int upd_bit;
	struct clkmux_ops *ops;
	struct clkmux *siblings;
	struct pll *pll;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};

#endif /* NEW_MUX */

struct cg_grp;
struct cg_grp_ops {
	int (*prepare)(struct cg_grp *grp);
	int (*finished)(struct cg_grp *grp);
	unsigned int (*get_state)(struct cg_grp *grp);
	int (*dump_regs)(struct cg_grp *grp, unsigned int *ptr);
};

struct cg_grp {
	const char *name;
	void __iomem *set_addr;
	void __iomem *clr_addr;
	void __iomem *sta_addr;
	void __iomem *dummy_addr;
	void __iomem *bw_limit_addr;
	unsigned int mask;
	unsigned int state;
	struct cg_grp_ops *ops;
	struct subsys *sys;
};

struct cg_clk;
struct cg_clk_ops {
	int (*get_state)(struct cg_clk *clk);
	int (*check_validity)(struct cg_clk *clk);
	int (*enable)(struct cg_clk *clk);
	int (*disable)(struct cg_clk *clk);
};

struct cg_clk {
	int cnt;
	unsigned int state;
	unsigned int mask;
	int force_on;
	struct cg_clk_ops *ops;
	struct cg_grp *grp;
	struct clkmux *mux;
	struct pll *pll;
	struct cg_clk *siblings;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};

#ifdef CONFIG_CLKMGR_STAT
struct stat_node {
	struct list_head link;
	unsigned int cnt_on;
	unsigned int cnt_off;
	char name[0];
};
#endif

/************************************************
 **********      global variablies     **********
 ************************************************/

#define PWR_DOWN	0
#define PWR_ON		1

static int initialized;
static int es_flag;
static int slp_chk_mtcmos_pll_stat;

static struct pll plls[NR_PLLS];
static struct subsys syss[NR_SYSS];
static struct clkmux muxs[NR_MUXS];
static struct cg_grp grps[NR_GRPS];
static struct cg_clk clks[NR_CLKS];

/************************************************
 **********      spin lock protect     **********
 ************************************************/

static DEFINE_SPINLOCK(clock_lock);

#define clkmgr_lock(flags) spin_lock_irqsave(&clock_lock, flags)
#define clkmgr_unlock(flags) spin_unlock_irqrestore(&clock_lock, flags)
#define clkmgr_locked() spin_is_locked(&clock_lock)

int clkmgr_is_locked(void)
{
	return clkmgr_locked();
}
EXPORT_SYMBOL(clkmgr_is_locked);

/************************************************
 **********     clkmgr stat debug      **********
 ************************************************/

#ifdef CONFIG_CLKMGR_STAT

void update_stat_locked(struct list_head *head, char *name, int op)
{
	struct list_head *pos = NULL;
	struct stat_node *node = NULL;
	int len = strlen(name);
	int new_node = 1;

	list_for_each(pos, head) {
		node = list_entry(pos, struct stat_node, link);
		if (!strncmp(node->name, name, len)) {
			new_node = 0;
			break;
		}
	}

	if (new_node) {
		node = kzalloc(sizeof(*node) + len + 1, GFP_ATOMIC);
		if (!node) {
			clk_err("[%s]: malloc stat node for %s fail\n", __func__, name);
			return;
		}

		memcpy(node->name, name, len);
		list_add_tail(&node->link, head);
	}

	if (op)
		node->cnt_on++;
	else
		node->cnt_off++;
}

#endif /* CONFIG_CLKMGR_STAT */

#if defined(CLK_LOG) || defined(CLK_LOG_TOP)

int ignore_clk_log(enum cg_clk_id clkid, struct cg_clk *clk)
{
	enum cg_clk_id ignored_clk[] = {
		MT_CG_INFRA_I2C0,
		MT_CG_INFRA_I2C1,
		MT_CG_INFRA_I2C2,
	};

	size_t i;

	for (i = 0; i < ARRAY_SIZE(ignored_clk); i++) {
		if (clkid == ignored_clk[i] || clk == &clks[ignored_clk[i]])
			return 1;
	}

	return 0;
}

#endif /* defined(CLK_LOG) || defined(CLK_LOG_TOP) */

/************************************************
 **********    function declaration    **********
 ************************************************/

static int pll_enable_locked(struct pll *pll);
static int pll_disable_locked(struct pll *pll);

static int sys_enable_locked(struct subsys *sys);
static int sys_disable_locked(struct subsys *sys, int force_off);

static void mux_enable_locked(struct clkmux *mux);
static void mux_disable_locked(struct clkmux *mux);

static int clk_enable_locked(struct cg_clk *clk);
static int clk_disable_locked(struct cg_clk *clk);

static inline int pll_enable_internal(struct pll *pll, char *name)
{
	int err;

	err = pll_enable_locked(pll);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&pll->head, name, 1);
#endif
	return err;
}

static inline int pll_disable_internal(struct pll *pll, char *name)
{
	int err;

	err = pll_disable_locked(pll);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&pll->head, name, 0);
#endif
	return err;
}

static inline int subsys_enable_internal(struct subsys *sys, char *name)
{
	int err;

	err = sys_enable_locked(sys);
#ifdef CONFIG_CLKMGR_STAT
	/* update_stat_locked(&sys->head, name, 1); */
#endif
	return err;
}

static inline int subsys_disable_internal(struct subsys *sys, int force_off, char *name)
{
	int err;

	err = sys_disable_locked(sys, force_off);
#ifdef CONFIG_CLKMGR_STAT
	/* update_stat_locked(&sys->head, name, 0); */
#endif
	return err;
}

static inline void mux_enable_internal(struct clkmux *mux, char *name)
{
	mux_enable_locked(mux);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&mux->head, name, 1);
#endif
}

static inline void mux_disable_internal(struct clkmux *mux, char *name)
{
	mux_disable_locked(mux);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&mux->head, name, 0);
#endif
}

static inline int clk_enable_internal(struct cg_clk *clk, char *name)
{
	int err;

	err = clk_enable_locked(clk);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&clk->head, name, 1);
#endif
	return err;
}

static inline int clk_disable_internal(struct cg_clk *clk, char *name)
{
	int err;

	err = clk_disable_locked(clk);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&clk->head, name, 0);
#endif
	return err;
}

/************************************************
 **********          pll part          **********
 ************************************************/

#define PLL_TYPE_SDM    0
#define PLL_TYPE_LC     1

#define HAVE_RST_BAR    (0x1 << 0)
#define HAVE_PLL_HP     (0x1 << 1)
#define HAVE_FIX_FRQ    (0x1 << 2)
#define Others          (0x1 << 3)

#define RST_BAR_MASK    0x1000000

static struct pll_ops arm_pll_ops;
static struct pll_ops sdm_pll_ops;

static struct pll plls[NR_PLLS] = {

	[ARMPLL] = {
		.name = __stringify(ARMPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &arm_pll_ops,
		.hp_id = FH_ARMCA7_PLLID,
		.hp_switch = 1,
	},
	[MAINPLL] = {
		.name = __stringify(MAINPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP | HAVE_RST_BAR,
		.en_mask = 0xF0000101,
		.ops = &sdm_pll_ops,
		.hp_id = FH_MAIN_PLLID,
		.hp_switch = 1,
	},
	[MSDCPLL] = {
		.name = __stringify(MSDCPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
		.hp_id = FH_MSDC_PLLID,
		.hp_switch = 1,
	},
	[UNIVPLL] = {
		.name = __stringify(UNIVPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_RST_BAR | HAVE_FIX_FRQ,
		.en_mask = 0xFE000001,
		.ops = &sdm_pll_ops,
	},
	[MMPLL] = {
		.name = __stringify(MMPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
		.hp_id = FH_MM_PLLID,
		.hp_switch = 1,
	},
	[VENCPLL] = {
		.name = __stringify(VENCPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
		.hp_id = FH_VENC_PLLID,
		.hp_switch = 1,
	},
	[TVDPLL] = {
		.name = __stringify(TVDPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
		.hp_id = FH_TVD_PLLID,
		.hp_switch = 1,
	},
	[MPLL] = {
		.name = __stringify(MPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
		.hp_id = FH_M_PLLID,
		.hp_switch = 1,
	},
	[AUD1PLL] = {
		.name = __stringify(AUD1PLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
	},
	[AUD2PLL] = {
		.name = __stringify(AUD2PLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
	},
	[LVDSPLL] = {
		.name = __stringify(LVDSPLL),
		.type = PLL_TYPE_SDM,
		.feat = HAVE_PLL_HP,
		.en_mask = 0x00000001,
		.ops = &sdm_pll_ops,
	},
};

static struct pll *id_to_pll(unsigned int id)
{
	return id < NR_PLLS ? plls + id : NULL;
}

#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)

#define SDM_PLL_N_INFO_MASK 0x001FFFFF
#define UNIV_SDM_PLL_N_INFO_MASK 0x001fc000
#define SDM_PLL_N_INFO_CHG  0x80000000
#define ARMPLL_POSDIV_MASK  0x07000000

static int pll_get_state_op(struct pll *pll)
{
	return clk_readl(pll->base_addr) & 0x1;
}

static void sdm_pll_enable_op(struct pll *pll)
{
#ifdef PLL_LOG
	clk_dbg("[%s]: pll->name=%s\n", __func__, pll->name);
#endif

	clk_setl(pll->pwr_addr, PLL_PWR_ON);
	udelay(2);
	clk_clrl(pll->pwr_addr, PLL_ISO_EN);

	clk_setl(pll->base_addr, pll->en_mask);
	udelay(20);

	if (pll->feat & HAVE_RST_BAR)
		clk_setl(pll->base_addr, RST_BAR_MASK);
}

static void sdm_pll_disable_op(struct pll *pll)
{
#ifdef PLL_LOG
	clk_dbg("[%s]: pll->name=%s\n", __func__, pll->name);
#endif

	if (pll->feat & HAVE_RST_BAR)
		clk_clrl(pll->base_addr, RST_BAR_MASK);

	clk_clrl(pll->base_addr, 0x1);

	clk_setl(pll->pwr_addr, PLL_ISO_EN);
	clk_clrl(pll->pwr_addr, PLL_PWR_ON);
}

static void sdm_pll_fsel_op(struct pll *pll, unsigned int value)
{
	unsigned int ctrl_value;

	ctrl_value = clk_readl(pll->base_addr + 4);
	if (pll->base_addr == UNIVPLL_CON0) {
		ctrl_value &= ~UNIV_SDM_PLL_N_INFO_MASK;
		ctrl_value |= value & UNIV_SDM_PLL_N_INFO_MASK;
	} else {
		ctrl_value &= ~SDM_PLL_N_INFO_MASK;
		ctrl_value |= value & SDM_PLL_N_INFO_MASK;
	}

	ctrl_value |= SDM_PLL_N_INFO_CHG;

	clk_writel(pll->base_addr + 4, ctrl_value);
	udelay(20);
}

static int sdm_pll_dump_regs_op(struct pll *pll, unsigned int *ptr)
{
	*(ptr) = clk_readl(pll->base_addr);
	*(++ptr) = clk_readl(pll->base_addr + 4);
	*(++ptr) = clk_readl(pll->pwr_addr);

	return 3;
}

static int sdm_pll_hp_enable_op(struct pll *pll)
{
	int err = 0;

	if (!pll->hp_switch || (pll->state == PWR_DOWN))
		return 0;

#ifndef Bring_Up
	if (initialized)
		err = freqhopping_config(pll->hp_id, 0, PWR_ON);
#endif

	return err;
}

static int sdm_pll_hp_disable_op(struct pll *pll)
{
	int err = 0;

	if (!pll->hp_switch || (pll->state == PWR_ON))
		return 0;

#ifndef Bring_Up
	if (initialized)
		err = freqhopping_config(pll->hp_id, 0, PWR_DOWN);
#endif

	return err;
}

static struct pll_ops sdm_pll_ops = {
	.get_state = pll_get_state_op,
	.enable = sdm_pll_enable_op,
	.disable = sdm_pll_disable_op,
	.fsel = sdm_pll_fsel_op,
	.dump_regs = sdm_pll_dump_regs_op,
	.hp_enable = sdm_pll_hp_enable_op,
	.hp_disable = sdm_pll_hp_disable_op,
};

static void arm_pll_fsel_op(struct pll *pll, unsigned int value)
{
	unsigned int ctrl_value;

	ctrl_value = clk_readl(pll->base_addr + 4);
	ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
	ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
	ctrl_value |= SDM_PLL_N_INFO_CHG;

	clk_writel(pll->base_addr + 4, ctrl_value);
	udelay(20);
}

static struct pll_ops arm_pll_ops = {
	.get_state = pll_get_state_op,
	.enable = sdm_pll_enable_op,
	.disable = sdm_pll_disable_op,
	.fsel = arm_pll_fsel_op,
	.dump_regs = sdm_pll_dump_regs_op,
	.hp_enable = sdm_pll_hp_enable_op,
	.hp_disable = sdm_pll_hp_disable_op,
};

static int get_pll_state_locked(struct pll *pll)
{
	if (likely(initialized))
		return pll->state;
	else
		return pll->ops->get_state(pll);
}

static int pll_enable_locked(struct pll *pll)
{
	pll->cnt++;

#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	if (pll->cnt > 1)
		return 0;

	if (pll->state == PWR_DOWN) {
		pll->ops->enable(pll);
		pll->state = PWR_ON;
	}

	if (pll->ops->hp_enable)
		pll->ops->hp_enable(pll);

#ifdef PLL_LOG_TOP
	clk_info("[%s]: End. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif
	return 0;
}

static int pll_disable_locked(struct pll *pll)
{
#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	BUG_ON(!pll->cnt);
	pll->cnt--;

#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	if (pll->cnt > 0)
		return 0;

	if (pll->state == PWR_ON) {
		pll->ops->disable(pll);
		pll->state = PWR_DOWN;
	}

	if (pll->ops->hp_disable)
		pll->ops->hp_disable(pll);

#ifdef PLL_LOG_TOP
	clk_info("[%s]: End. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif
	return 0;
}

static int pll_fsel_locked(struct pll *pll, unsigned int value)
{
	pll->ops->fsel(pll, value);

	if (pll->ops->hp_enable)
		pll->ops->hp_enable(pll);

	return 0;
}

int pll_is_on(int id)
{
	int state;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!pll);

	clkmgr_lock(flags);
	state = get_pll_state_locked(pll);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(pll_is_on);

int enable_pll(int id, char *name)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);
	BUG_ON(!name);
#ifdef PLL_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	err = pll_enable_internal(pll, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(enable_pll);

int disable_pll(int id, char *name)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);
	BUG_ON(!name);

#ifdef PLL_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	err = pll_disable_internal(pll, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(disable_pll);

int pll_fsel(int id, unsigned int value)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	clkmgr_lock(flags);
	err = pll_fsel_locked(pll, value);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(pll_fsel);

int pll_hp_switch_on(int id, int hp_on)
{
	int err = 0;
	unsigned long flags;
	int old_value;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	if (pll->type != PLL_TYPE_SDM) {
		err = -EINVAL;
		goto out;
	}

	clkmgr_lock(flags);

	old_value = pll->hp_switch;
	if (old_value == 0) {
		pll->hp_switch = 1;
		if (hp_on)
			err = pll->ops->hp_enable(pll);
	}

	clkmgr_unlock(flags);

out:
	return err;
}
EXPORT_SYMBOL(pll_hp_switch_on);

int pll_hp_switch_off(int id, int hp_off)
{
	int err = 0;
	unsigned long flags;
	int old_value;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	if (pll->type != PLL_TYPE_SDM) {
		err = -EINVAL;
		goto out;
	}

	clkmgr_lock(flags);

	old_value = pll->hp_switch;
	if (old_value == 1) {
		if (hp_off)
			err = pll->ops->hp_disable(pll);

		pll->hp_switch = 0;
	}

	clkmgr_unlock(flags);

out:
	return err;
}
EXPORT_SYMBOL(pll_hp_switch_off);

int pll_dump_regs(int id, unsigned int *ptr)
{
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	return pll->ops->dump_regs(pll, ptr);
}
EXPORT_SYMBOL(pll_dump_regs);

const char *pll_get_name(int id)
{
	struct pll *pll = id_to_pll(id);

	BUG_ON(!initialized);
	BUG_ON(!pll);

	return pll->name;
}

void set_mipi26m(int en)
{
	unsigned long flags;

#ifdef Bring_Up
	return;
#endif

	clkmgr_lock(flags);

	if (en)
		clk_setl(AP_PLL_CON0, 1 << 6);
	else
		clk_clrl(AP_PLL_CON0, 1 << 6);

	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(set_mipi26m);

void set_ada_ssusb_xtal_ck(int en)
{
	unsigned long flags;

#ifdef Bring_Up
	return;
#endif

	clkmgr_lock(flags);

	if (en) {
		clk_setl(AP_PLL_CON2, 1 << 0);
		udelay(100);
		clk_setl(AP_PLL_CON2, 1 << 1);
		clk_setl(AP_PLL_CON2, 1 << 2);
	} else
		clk_clrl(AP_PLL_CON2, 0x7);

	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(set_ada_ssusb_xtal_ck);

/************************************************
 **********         subsys part        **********
 ************************************************/

#define SYS_TYPE_MODEM	0
#define SYS_TYPE_MEDIA	1
#define SYS_TYPE_OTHER	2
#define SYS_TYPE_CONN	3

static struct subsys_ops conn_sys_ops;
static struct subsys_ops dis_sys_ops;
static struct subsys_ops mfg_sys_ops;
static struct subsys_ops isp_sys_ops;
static struct subsys_ops vde_sys_ops;
static struct subsys_ops ven_sys_ops;
static struct subsys_ops aud_sys_ops;

static struct subsys syss[NR_SYSS] = {
	[SYS_CONN] = {
		.name = __stringify(SYS_CONN),
		.type = SYS_TYPE_CONN,
		.default_sta = PWR_DOWN,
		.sta_mask = 1U << 1,
		.ops = &conn_sys_ops,
	},
	[SYS_DIS] = {
		.name = __stringify(SYS_DIS),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 3,
		.ops = &dis_sys_ops,
		.start = &grps[CG_DISP0],
		.nr_grps = 2,
		.mux = &muxs[MT_MUX_MM],
	},
	[SYS_MFG] = {
		.name = __stringify(SYS_MFG),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 4,
		.ops = &mfg_sys_ops,
		.start = &grps[CG_MFG],
		.nr_grps = 1,
		.mux = &muxs[MT_MUX_MFG],
	},
	[SYS_ISP] = {
		.name = __stringify(SYS_ISP),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 5,
		.ops = &isp_sys_ops,
		.start = &grps[CG_IMAGE],
		.nr_grps = 1,
	},
	[SYS_VDE] = {
		.name = __stringify(SYS_VDE),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 7,
		.ops = &vde_sys_ops,
		.start = &grps[CG_VDEC0],
		.nr_grps = 2,
		.mux = &muxs[MT_MUX_VDEC],
	},
	[SYS_VEN] = {
		.name = __stringify(SYS_VEN),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 21,
		.ops = &ven_sys_ops,
		.start = &grps[CG_VENC],
		.nr_grps = 1,
	},
	[SYS_AUD] = {
		.name = __stringify(SYS_AUD),
		.type = SYS_TYPE_MEDIA,
		.default_sta = PWR_ON,
		.sta_mask = 1U << 24,
		.ops = &aud_sys_ops,
		.start = &grps[CG_AUDIO],
		.nr_grps = 1,
		.mux = &muxs[MT_MUX_AUDINTBUS],
	}
};

static void larb_backup(int larb_idx);
static void larb_restore(int larb_idx);

static struct subsys *id_to_sys(unsigned int id)
{
	return id < NR_SYSS ? syss + id : NULL;
}

static int conn_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_connsys(STA_POWER_ON);
}

static int conn_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_connsys(STA_POWER_DOWN);
}

static int dis_sys_enable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_disp(STA_POWER_ON);
	clk_writel(MMSYS_DUMMY, 0xFFFFFFFF);
	larb_restore(MT_LARB_DISP);

	return err;
}

static int dis_sys_disable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_DISP);
	err = spm_mtcmos_ctrl_disp(STA_POWER_DOWN);

	return err;
}

static int mfg_sys_enable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_ON);
	err = spm_mtcmos_ctrl_mfg(STA_POWER_ON);

	return err;
}

static int mfg_sys_disable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
	err = spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_DOWN);

	return err;
}

static int isp_sys_enable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_isp(STA_POWER_ON);
	larb_restore(MT_LARB_IMG);

	return err;
}

static int isp_sys_disable_op(struct subsys *sys)
{
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_IMG);
	return spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
}

static int vde_sys_enable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_vdec(STA_POWER_ON);
	larb_restore(MT_LARB_VDEC);

	return err;
}

static int vde_sys_disable_op(struct subsys *sys)
{
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_VDEC);
	return spm_mtcmos_ctrl_vdec(STA_POWER_DOWN);
}

static int ven_sys_enable_op(struct subsys *sys)
{
	int err;

#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_venc(STA_POWER_ON);
	larb_restore(MT_LARB_VENC);

	return err;
}

static int ven_sys_disable_op(struct subsys *sys)
{
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_VENC);
	return spm_mtcmos_ctrl_venc(STA_POWER_DOWN);
}

static int aud_sys_enable_op(struct subsys *sys)
{
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	return spm_mtcmos_ctrl_aud(STA_POWER_ON);
}

static int aud_sys_disable_op(struct subsys *sys)
{
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	return spm_mtcmos_ctrl_aud(STA_POWER_DOWN);
}

static int sys_get_state_op(struct subsys *sys)
{
	unsigned int sta = clk_readl(SPM_PWR_STATUS);
	unsigned int sta_s = clk_readl(SPM_PWR_STATUS_2ND);

	return (sta & sys->sta_mask) && (sta_s & sys->sta_mask);
}

static int sys_dump_regs_op(struct subsys *sys, unsigned int *ptr)
{
	*(ptr) = clk_readl(sys->ctl_addr);
	return 1;
}

static struct subsys_ops conn_sys_ops = {
	.enable = conn_sys_enable_op,
	.disable = conn_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops dis_sys_ops = {
	.enable = dis_sys_enable_op,
	.disable = dis_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops mfg_sys_ops = {
	.enable = mfg_sys_enable_op,
	.disable = mfg_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops isp_sys_ops = {
	.enable = isp_sys_enable_op,
	.disable = isp_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops vde_sys_ops = {
	.enable = vde_sys_enable_op,
	.disable = vde_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops ven_sys_ops = {
	.enable = ven_sys_enable_op,
	.disable = ven_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops aud_sys_ops = {
	.enable = aud_sys_enable_op,
	.disable = aud_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static int get_sys_state_locked(struct subsys *sys)
{
	if (likely(initialized))
		return sys->state;
	else
		return sys->ops->get_state(sys);
}

int subsys_is_on(int id)
{
	int state;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!sys);

	clkmgr_lock(flags);
	state = get_sys_state_locked(sys);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(subsys_is_on);

/* #define STATE_CHECK_DEBUG */

static int sys_enable_locked(struct subsys *sys)
{
	int err;
	int local_state = sys->state;

#ifdef STATE_CHECK_DEBUG
	int reg_state = sys->ops->get_state(sys);

	BUG_ON(local_state != reg_state);
#endif

#ifdef SYS_LOG
	clk_info("[%s]: Start. sys->name=%s, sys->state=%d\n", __func__, sys->name, sys->state);
#endif

	if (local_state == PWR_ON)
		return 0;

	if (sys->mux)
		mux_enable_internal(sys->mux, "sys");

	err = sys->ops->enable(sys);
	WARN_ON(err);

	if (!err)
		sys->state = PWR_ON;

#ifdef SYS_LOG
	clk_info("[%s]: End. sys->name=%s, sys->state=%d\n", __func__, sys->name, sys->state);
#endif
	return err;
}

static int sys_disable_locked(struct subsys *sys, int force_off)
{
	int err;
	int local_state = sys->state;	/* get_subsys_local_state(sys); */
	int i;
	struct cg_grp *grp;

#ifdef STATE_CHECK_DEBUG
	int reg_state = sys->ops->get_state(sys);	/* get_subsys_reg_state(sys); */

	BUG_ON(local_state != reg_state);
#endif

#ifdef SYS_LOG
	clk_info("[%s]: Start. sys->name=%s, sys->state=%d, force_off=%d\n", __func__, sys->name,
		 sys->state, force_off);
#endif
	if (!force_off) {
		/* could be power off or not */
		for (i = 0; i < sys->nr_grps; i++) {
			grp = sys->start + i;
			if (grp->state)
				return 0;
		}
	}

	if (local_state == PWR_DOWN)
		return 0;

	err = sys->ops->disable(sys);
	WARN_ON(err);

	if (!err)
		sys->state = PWR_DOWN;

	if (sys->mux)
		mux_disable_internal(sys->mux, "sys");

#ifdef SYS_LOG
	clk_info("[%s]: End. sys->name=%s, sys->state=%d, force_off=%d\n", __func__, sys->name,
		 sys->state, force_off);
#endif

	return err;
}

int enable_subsys(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_enable_internal(sys, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(enable_subsys);

int disable_subsys(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 0, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(disable_subsys);

int disable_subsys_force(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 1, name);
	clkmgr_unlock(flags);

	return err;
}

int subsys_dump_regs(int id, unsigned int *ptr)
{
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	return sys->ops->dump_regs(sys, ptr);
}
EXPORT_SYMBOL(subsys_dump_regs);

const char *subsys_get_name(int id)
{
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!initialized);
	BUG_ON(!sys);

	return sys->name;
}

#define JIFFIES_PER_LOOP 10

int conn_power_on(void)
{
	int err = 0;
	unsigned long flags;
	struct subsys *sys = id_to_sys(SYS_CONN);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_CONN);

	clkmgr_lock(flags);
	err = subsys_enable_internal(sys, "conn");
	clkmgr_unlock(flags);

	clk_info("[%s]\n", __func__);

	WARN_ON(err);

	return err;
}
EXPORT_SYMBOL(conn_power_on);

int conn_power_off(void)
{
	int err = 0;
	unsigned long flags;
	struct subsys *sys = id_to_sys(SYS_CONN);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_CONN);

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 0, "conn");
	clkmgr_unlock(flags);

	clk_info("[%s]\n", __func__);

	WARN_ON(err);

	return err;
}
EXPORT_SYMBOL(conn_power_off);

static DEFINE_MUTEX(larb_monitor_lock);
static LIST_HEAD(larb_monitor_handlers);

void register_larb_monitor(struct larb_monitor *handler)
{
	struct list_head *pos;

#ifdef Bring_Up
	return;
#endif

	clk_info("register_larb_monitor\n");

	mutex_lock(&larb_monitor_lock);

	list_for_each(pos, &larb_monitor_handlers) {
		struct larb_monitor *l;

		l = list_entry(pos, struct larb_monitor, link);
		if (l->level > handler->level)
			break;
	}

	list_add_tail(&handler->link, pos);

	mutex_unlock(&larb_monitor_lock);
}
EXPORT_SYMBOL(register_larb_monitor);

void unregister_larb_monitor(struct larb_monitor *handler)
{
#ifdef Bring_Up
	return;
#endif

	mutex_lock(&larb_monitor_lock);
	list_del(&handler->link);
	mutex_unlock(&larb_monitor_lock);
}
EXPORT_SYMBOL(unregister_larb_monitor);

static void larb_clk_prepare(int larb_idx)
{
	switch (larb_idx) {
	case MT_LARB_DISP:
		/* display */
		clk_writel(DISP_CG_CLR0, 0x3);
		break;
	case MT_LARB_VDEC:
		/* vde */
		clk_writel(LARB_CKEN_SET, 0x1);
		break;
	case MT_LARB_IMG:
		/* isp */
		clk_writel(IMG_CG_CLR, 0x1);
		break;
	case MT_LARB_VENC:
		/* venc */
		clk_writel(VENC_CG_SET, 0x11);
		break;
	default:
		BUG();
	}
}

static void larb_clk_finish(int larb_idx)
{
	switch (larb_idx) {
	case MT_LARB_DISP:
		/* display */
		clk_writel(DISP_CG_SET0, 0x3);
		break;
	case MT_LARB_VDEC:
		/* vde */
		clk_writel(LARB_CKEN_CLR, 0x1);
		break;
	case MT_LARB_IMG:
		/* isp */
		clk_writel(IMG_CG_SET, 0x1);
		break;
	case MT_LARB_VENC:
		/* venc */
		clk_writel(VENC_CG_CLR, 0x11);
		break;
	default:
		BUG();
	}
}

static void larb_backup(int larb_idx)
{
	struct larb_monitor *pos;

	clk_dbg("[%s]: backup larb%d\n", __func__, larb_idx);

	larb_clk_prepare(larb_idx);

	list_for_each_entry(pos, &larb_monitor_handlers, link) {
		if (pos->backup != NULL)
			pos->backup(pos, larb_idx);
	}

	larb_clk_finish(larb_idx);
}

static void larb_restore(int larb_idx)
{
	struct larb_monitor *pos;

	clk_dbg("[%s]: restore larb%d\n", __func__, larb_idx);

	larb_clk_prepare(larb_idx);

	list_for_each_entry(pos, &larb_monitor_handlers, link) {
		if (pos->restore != NULL)
			pos->restore(pos, larb_idx);
	}

	larb_clk_finish(larb_idx);
}

/************************************************
 **********         clkmux part        **********
 ************************************************/

#if FMETER

#define BAK_MEMPLL		0

#define PLL_HP_EN		(clk_apmixed_base + 0xF00)

enum FMETER_TYPE {
	ABIST,
	CKGEN
};

enum ABIST_CLK {
	FM_ABIST_CLK_NULL,

	FM_AD_ARMCA7PLL_754M_CORE_CK	=  2,
	FM_AD_OSC_CK			=  3,
	FM_AD_MAIN_H546M_CK		=  4,
	FM_AD_MAIN_H364M_CK		=  5,
	FM_AD_MAIN_H218P4M_CK		=  6,
	FM_AD_MAIN_H156M_CK		=  7,
	FM_AD_UNIV_178P3M_CK		=  8,
	FM_AD_UNIV_48M_CK		=  9,
	FM_AD_UNIV_624M_CK		= 10,
	FM_AD_UNIV_416M_CK		= 11,
	FM_AD_UNIV_249P6M_CK		= 12,
	FM_AD_APLL1_180P6336M_CK	= 13,
	FM_AD_APLL2_196P608M_CK		= 14,
	FM_AD_LTEPLL_FS26M_CK		= 15,
	FM_RTC32K_CK_I			= 16,
	FM_AD_MMPLL_700M_CK		= 17,
	FM_AD_VENCPLL_410M_CK		= 18,
	FM_AD_TVDPLL_594M_CK		= 19,
	FM_AD_MPLL_208M_CK		= 20,
	FM_AD_MSDCPLL_806M_CK		= 21,
	FM_AD_USB_48M_CK		= 22,
	FM_MIPI_DSI_TST_CK		= 24,
	FM_AD_PLLGP_TST_CK		= 25,
	FM_AD_LVDSTX_MONCLK		= 26,
	FM_AD_DSI0_LNTC_DSICLK		= 27,
	FM_AD_DSI0_MPPLL_TST_CK		= 28,
	FM_AD_CSI0_DELAY_TSTCLK		= 29,
	FM_AD_CSI1_DELAY_TSTCLK		= 30,
	FM_AD_HDMITX_MON_CK		= 31,
	FM_AD_ARMPLL_1508M_CK		= 32,
	FM_AD_MAINPLL_1092M_CK		= 33,
	FM_AD_MAINPLL_PRO_CK		= 34,
	FM_AD_UNIVPLL_1248M_CK		= 35,
	FM_AD_LVDSPLL_150M_CK		= 36,
	FM_AD_LVDSPLL_ETH_CK		= 37,
	FM_AD_SSUSB_48M_CK		= 38,
	FM_AD_MIPI_26M_CK		= 39,
	FM_AD_MEM_26M_CK		= 40,
	FM_AD_MEMPLL_MONCLK		= 41,
	FM_AD_MEMPLL2_MONCLK		= 42,
	FM_AD_MEMPLL3_MONCLK		= 43,
	FM_AD_MEMPLL4_MONCLK		= 44,
	FM_AD_MEMPLL_REFCLK_BUF		= 45,
	FM_AD_MEMPLL_FBCLK_BUF		= 46,
	FM_AD_MEMPLL2_REFCLK_BUF	= 47,
	FM_AD_MEMPLL2_FBCLK_BUF		= 48,
	FM_AD_MEMPLL3_REFCLK_BUF	= 49,
	FM_AD_MEMPLL3_FBCLK_BUF		= 50,
	FM_AD_MEMPLL4_REFCLK_BUF	= 51,
	FM_AD_MEMPLL4_FBCLK_BUF		= 52,
	FM_AD_USB20_MONCLK		= 53,
	FM_AD_USB20_MONCLK_1P		= 54,
	FM_AD_MONREF_CK			= 55,
	FM_AD_MONFBK_CK			= 56,

	FM_ABIST_CLK_END,
};

const char *ABIST_CLK_NAME[] = {
	[FM_AD_ARMCA7PLL_754M_CORE_CK]	= "AD_ARMCA7PLL_754M_CORE_CK",
	[FM_AD_OSC_CK]			= "AD_OSC_CK",
	[FM_AD_MAIN_H546M_CK]		= "AD_MAIN_H546M_CK",
	[FM_AD_MAIN_H364M_CK]		= "AD_MAIN_H364M_CK",
	[FM_AD_MAIN_H218P4M_CK]		= "AD_MAIN_H218P4M_CK",
	[FM_AD_MAIN_H156M_CK]		= "AD_MAIN_H156M_CK",
	[FM_AD_UNIV_178P3M_CK]		= "AD_UNIV_178P3M_CK",
	[FM_AD_UNIV_48M_CK]		= "AD_UNIV_48M_CK",
	[FM_AD_UNIV_624M_CK]		= "AD_UNIV_624M_CK",
	[FM_AD_UNIV_416M_CK]		= "AD_UNIV_416M_CK",
	[FM_AD_UNIV_249P6M_CK]		= "AD_UNIV_249P6M_CK",
	[FM_AD_APLL1_180P6336M_CK]	= "AD_APLL1_180P6336M_CK",
	[FM_AD_APLL2_196P608M_CK]	= "AD_APLL2_196P608M_CK",
	[FM_AD_LTEPLL_FS26M_CK]		= "AD_LTEPLL_FS26M_CK",
	[FM_RTC32K_CK_I]		= "rtc32k_ck_i",
	[FM_AD_MMPLL_700M_CK]		= "AD_MMPLL_700M_CK",
	[FM_AD_VENCPLL_410M_CK]		= "AD_VENCPLL_410M_CK",
	[FM_AD_TVDPLL_594M_CK]		= "AD_TVDPLL_594M_CK",
	[FM_AD_MPLL_208M_CK]		= "AD_MPLL_208M_CK",
	[FM_AD_MSDCPLL_806M_CK]		= "AD_MSDCPLL_806M_CK",
	[FM_AD_USB_48M_CK]		= "AD_USB_48M_CK",
	[FM_MIPI_DSI_TST_CK]		= "MIPI_DSI_TST_CK",
	[FM_AD_PLLGP_TST_CK]		= "AD_PLLGP_TST_CK",
	[FM_AD_LVDSTX_MONCLK]		= "AD_LVDSTX_MONCLK",
	[FM_AD_DSI0_LNTC_DSICLK]	= "AD_DSI0_LNTC_DSICLK",
	[FM_AD_DSI0_MPPLL_TST_CK]	= "AD_DSI0_MPPLL_TST_CK",
	[FM_AD_CSI0_DELAY_TSTCLK]	= "AD_CSI0_DELAY_TSTCLK",
	[FM_AD_CSI1_DELAY_TSTCLK]	= "AD_CSI1_DELAY_TSTCLK",
	[FM_AD_HDMITX_MON_CK]		= "AD_HDMITX_MON_CK",
	[FM_AD_ARMPLL_1508M_CK]		= "AD_ARMPLL_1508M_CK",
	[FM_AD_MAINPLL_1092M_CK]	= "AD_MAINPLL_1092M_CK",
	[FM_AD_MAINPLL_PRO_CK]		= "AD_MAINPLL_PRO_CK",
	[FM_AD_UNIVPLL_1248M_CK]	= "AD_UNIVPLL_1248M_CK",
	[FM_AD_LVDSPLL_150M_CK]		= "AD_LVDSPLL_150M_CK",
	[FM_AD_LVDSPLL_ETH_CK]		= "AD_LVDSPLL_ETH_CK",
	[FM_AD_SSUSB_48M_CK]		= "AD_SSUSB_48M_CK",
	[FM_AD_MIPI_26M_CK]		= "AD_MIPI_26M_CK",
	[FM_AD_MEM_26M_CK]		= "AD_MEM_26M_CK",
	[FM_AD_MEMPLL_MONCLK]		= "AD_MEMPLL_MONCLK",
	[FM_AD_MEMPLL2_MONCLK]		= "AD_MEMPLL2_MONCLK",
	[FM_AD_MEMPLL3_MONCLK]		= "AD_MEMPLL3_MONCLK",
	[FM_AD_MEMPLL4_MONCLK]		= "AD_MEMPLL4_MONCLK",
	[FM_AD_MEMPLL_REFCLK_BUF]	= "AD_MEMPLL_REFCLK_BUF",
	[FM_AD_MEMPLL_FBCLK_BUF]	= "AD_MEMPLL_FBCLK_BUF",
	[FM_AD_MEMPLL2_REFCLK_BUF]	= "AD_MEMPLL2_REFCLK_BUF",
	[FM_AD_MEMPLL2_FBCLK_BUF]	= "AD_MEMPLL2_FBCLK_BUF",
	[FM_AD_MEMPLL3_REFCLK_BUF]	= "AD_MEMPLL3_REFCLK_BUF",
	[FM_AD_MEMPLL3_FBCLK_BUF]	= "AD_MEMPLL3_FBCLK_BUF",
	[FM_AD_MEMPLL4_REFCLK_BUF]	= "AD_MEMPLL4_REFCLK_BUF",
	[FM_AD_MEMPLL4_FBCLK_BUF]	= "AD_MEMPLL4_FBCLK_BUF",
	[FM_AD_USB20_MONCLK]		= "AD_USB20_MONCLK",
	[FM_AD_USB20_MONCLK_1P]		= "AD_USB20_MONCLK_1P",
	[FM_AD_MONREF_CK]		= "AD_MONREF_CK",
	[FM_AD_MONFBK_CK]		= "AD_MONFBK_CK",
};

enum CKGEN_CLK {
	FM_CKGEN_CLK_NULL,

	FM_HD_FAXI_CK			=  1,
	FM_HF_FDDRPHYCFG_CK		=  2,
	FM_F_FPWM_CK			=  3,
	FM_HF_FVDEC_CK			=  4,
	FM_HF_FMM_CK			=  5,
	FM_HF_FCAMTG_CK			=  6,
	FM_F_FUART_CK			=  7,
	FM_HF_FSPI_CK			=  8,
	FM_HF_FMSDC_30_0_CK		=  9,
	FM_HF_FMSDC_30_1_CK		= 10,
	FM_HF_FMSDC_30_2_CK		= 11,
	FM_HF_FMSDC_50_3_CK		= 12,
	FM_HF_FMSDC_50_3_HCLK_CK	= 13,
	FM_HF_FAUDIO_CK			= 14,
	FM_HF_FAUD_INTBUS_CK		= 15,
	FM_HF_FPMICSPI_CK		= 16,
	FM_HF_FSCP_CK			= 17,
	FM_HF_FATB_CK			= 18,
	FM_HF_FSNFI_CK			= 19,
	FM_HF_FDPI0_CK			= 20,
	FM_HF_FAUD_1_CK_PRE		= 21,
	FM_HF_FAUD_2_CK_PRE		= 22,
	FM_HF_FSCAM_CK			= 23,
	FM_HF_FMFG_CK			= 24,
	FM_ECO_AO12_MEM_CLKMUX_CK	= 25,
	FM_ECO_A012_MEM_DCM_CK		= 26,
	FM_HF_FDPI1_CK			= 27,
	FM_HF_FUFOENC_CK		= 28,
	FM_HF_FUFODEC_CK		= 29,
	FM_HF_FETH_CK			= 30,
	FM_HF_FONFI_CK			= 31,

	FM_CKGEN_CLK_END,
};

const char *CKGEN_CLK_NAME[] = {
	[FM_HD_FAXI_CK]			= "hd_faxi_ck",
	[FM_HF_FDDRPHYCFG_CK]		= "hf_fddrphycfg_ck",
	[FM_F_FPWM_CK]			= "f_fpwm_ck",
	[FM_HF_FVDEC_CK]		= "hf_fvdec_ck",
	[FM_HF_FMM_CK]			= "hf_fmm_ck",
	[FM_HF_FCAMTG_CK]		= "hf_fcamtg_ck",
	[FM_F_FUART_CK]			= "f_fuart_ck",
	[FM_HF_FSPI_CK]			= "hf_fspi_ck",
	[FM_HF_FMSDC_30_0_CK]		= "hf_fmsdc_30_0_ck",
	[FM_HF_FMSDC_30_1_CK]		= "hf_fmsdc_30_1_ck",
	[FM_HF_FMSDC_30_2_CK]		= "hf_fmsdc_30_2_ck",
	[FM_HF_FMSDC_50_3_CK]		= "hf_fmsdc_50_3_ck",
	[FM_HF_FMSDC_50_3_HCLK_CK]	= "hf_fmsdc_50_3_hclk_ck",
	[FM_HF_FAUDIO_CK]		= "hf_faudio_ck",
	[FM_HF_FAUD_INTBUS_CK]		= "hf_faud_intbus_ck",
	[FM_HF_FPMICSPI_CK]		= "hf_fpmicspi_ck",
	[FM_HF_FSCP_CK]			= "hf_fscp_ck",
	[FM_HF_FATB_CK]			= "hf_fatb_ck",
	[FM_HF_FSNFI_CK]		= "hf_fsnfi_ck",
	[FM_HF_FDPI0_CK]		= "hf_fdpi0_ck",
	[FM_HF_FAUD_1_CK_PRE]		= "hf_faud_1_ck_pre",
	[FM_HF_FAUD_2_CK_PRE]		= "hf_faud_2_ck_pre",
	[FM_HF_FSCAM_CK]		= "hf_fscam_ck",
	[FM_HF_FMFG_CK]			= "hf_fmfg_ck",
	[FM_ECO_AO12_MEM_CLKMUX_CK]	= "eco_ao12_mem_clkmux_ck",
	[FM_ECO_A012_MEM_DCM_CK]	= "eco_a012_mem_dcm_ck",
	[FM_HF_FDPI1_CK]		= "hf_fdpi1_ck",
	[FM_HF_FUFOENC_CK]		= "hf_fufoenc_ck",
	[FM_HF_FUFODEC_CK]		= "hf_fufodec_ck",
	[FM_HF_FETH_CK]			= "hf_feth_ck",
	[FM_HF_FONFI_CK]		= "hf_fonfi_ck",
};

static void set_fmeter_divider_ca7(u32 k1)
{
	clk_writel_mask(CLK_MISC_CFG_0, 23, 16, k1);
}

static void set_fmeter_divider(u32 k1)
{
	clk_writel_mask(CLK_MISC_CFG_0, 31, 24, k1);
}

static BOOL wait_fmeter_done(u32 tri_bit)
{
	static int max_wait_count;

	int wait_count = (max_wait_count > 0) ? (max_wait_count * 2 + 2) : 100;
	int i;

	/* wait fmeter */
	for (i = 0; i < wait_count && (clk_readl(CLK26CALI_0) & tri_bit); i++)
		udelay(20);

	if (!(clk_readl(CLK26CALI_0) & tri_bit)) {
		max_wait_count = max(max_wait_count, i);
		return TRUE;
	}

	return FALSE;
}

static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 cksw_ckgen[] = {15, 8, clk};
	u32 cksw_abist[] = {23, 16, clk};
	u32 fqmtr_ck =	(type == CKGEN) ? 1 : 0;
	u32 *cksw_hlv =	(type == CKGEN) ? cksw_ckgen	: cksw_abist;
	u32 tri_bit =	(type == CKGEN) ? BIT(4)	: BIT(4);
	u32 freq = 0;

	u32 clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);	/* backup reg value */
	u32 clk_cfg_val = clk_readl(CLK_DBG_CFG);

	/* setup fmeter */
	clk_writel_mask(CLK_DBG_CFG, 1, 0, fqmtr_ck);	/* fqmtr_ck_sel */
	clk_setl(CLK26CALI_0, BIT(12));			/* enable fmeter_en */

	set_fmeter_divider(k1);				/* set div (0 = /1) */
	set_fmeter_divider_ca7(k1);
	clk_writel_mask(CLK_DBG_CFG, cksw_hlv[0], cksw_hlv[1], cksw_hlv[2]);

	clk_setl(CLK26CALI_0, tri_bit);			/* start fmeter */

	if (wait_fmeter_done(tri_bit)) {
		u32 cnt = clk_readl(CLK26CALI_1) & 0xFFFF;

		/* freq = counter * 26M / 1024 (KHz) */
		freq = (cnt * 26000) * (k1 + 1) / 1024;
	}

	/* restore register settings */
	clk_writel(CLK_DBG_CFG, clk_cfg_val);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);

	return freq;
}

static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 last_freq = 0;
	u32 freq = fmeter_freq(type, k1, clk);
	u32 maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, k1, clk);
		maxfreq = max(freq, last_freq);
	}

	return freq;
}

static u32 measure_abist_freq(enum ABIST_CLK clk)
{
	return measure_stable_fmeter_freq(ABIST, 0, clk);
}

static u32 measure_ckgen_freq(enum CKGEN_CLK clk)
{
	return measure_stable_fmeter_freq(CKGEN, 0, clk);
}

static void measure_abist_clock(enum ABIST_CLK clk, struct seq_file *s)
{
	u32 freq = measure_abist_freq(clk);

	seq_printf(s, "%2d: %-25s %7u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void measure_ckgen_clock(enum CKGEN_CLK clk, struct seq_file *s)
{
	u32 freq = measure_ckgen_freq(clk);

	seq_printf(s, "%2d: %-25s %7u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

struct bak_regs1 {
	u32 pll_hp_en;
#if BAK_MEMPLL
	u32 mempll0;
	u32 mempll3;
	u32 mempll5;
	u32 mempll7;
#endif /* BAK_MEMPLL */
	u32 clk_scp_cfg_1;
};

static void before_fmeter(struct bak_regs1 *regs)
{
	regs->pll_hp_en = clk_readl(PLL_HP_EN);
	regs->clk_scp_cfg_1 = clk_readl(CLK_SCP_CFG_1);

#if BAK_MEMPLL
	regs->mempll0 = clk_readl(MEMPLL0);
	regs->mempll3 = clk_readl(MEMPLL3);
	regs->mempll5 = clk_readl(MEMPLL5);
	regs->mempll7 = clk_readl(MEMPLL7);
#endif /* BAK_MEMPLL */

	/* disable scpsys DCM */
	clk_clrl(CLK_SCP_CFG_1, BIT(2));

#if BAK_MEMPLL
	/* enable RG_MEMPLL*_MONCK_EN and RG_MEMPLL*_MONREF_EN */
	clk_setl(MEMPLL0, BIT(3) | BIT(5));
	clk_setl(MEMPLL3, BIT(19) | BIT(21));
	clk_setl(MEMPLL5, BIT(19) | BIT(21));
	clk_setl(MEMPLL7, BIT(19) | BIT(21));
#endif /* BAK_MEMPLL */

	/* disable PLL hopping */
	clk_writel(PLL_HP_EN, 0x0);
	udelay(10);
}

static void after_fmeter(struct bak_regs1 *regs)
{
	clk_writel(CLK_SCP_CFG_1, regs->clk_scp_cfg_1);
	clk_writel(PLL_HP_EN, regs->pll_hp_en);

#if BAK_MEMPLL
	clk_writel(MEMPLL0, regs->mempll0);
	clk_writel(MEMPLL3, regs->mempll3);
	clk_writel(MEMPLL5, regs->mempll5);
	clk_writel(MEMPLL7, regs->mempll7);
#endif /* BAK_MEMPLL */
}

#endif /* FMETER */

#if NEW_MUX

enum mux_clksrc_id {
	mux_clksrc_null,

	clk26m,
	clkrtc_ext,
	clkrtc_int,
	f26m_mem_ckgen_ck_occ,
	apll1_ck,
	apll2_ck,
	dmpll_ck,
	fhdmi_cts_ck,
	fhdmi_cts_ck_d2,
	fhdmi_cts_ck_d3,
	lvdspll_ck,
	lvdspll_d2,
	lvdspll_d4,
	lvdspll_d8,
	lvdspll_eth_ck,
	mmpll_ck,
	mpll_208m_ck,
	msdcpll_ck,
	msdcpll_d2,
	msdcpll_d4,
	syspll1_d16,
	syspll1_d2,
	syspll1_d4,
	syspll1_d8,
	syspll2_d2,
	syspll2_d4,
	syspll2_d8,
	syspll3_d2,
	syspll3_d4,
	syspll4_d2,
	syspll4_d4,
	syspll_d2,
	syspll_d2p5,
	syspll_d3,
	syspll_d5,
	syspll_d7,
	tvdpll_d16,
	tvdpll_d2,
	tvdpll_d4,
	tvdpll_d8,
	univpll1_d2,
	univpll1_d4,
	univpll2_d2,
	univpll2_d4,
	univpll2_d8,
	univpll3_d2,
	univpll3_d4,
	univpll3_d8,
	univpll_d2,
	univpll_d26,
	univpll_d2p5,
	univpll_d3,
	univpll_d5,
	univpll_d7,
	vencpll_ck,

	mux_clksrc_end
};

struct bit_range_t {
	u8 h;
	u8 l;
};

#define CLK_CFG_0_OFS		0x040
#define CLK_CFG_1_OFS		0x050
#define CLK_CFG_2_OFS		0x060
#define CLK_CFG_3_OFS		0x070
#define CLK_CFG_4_OFS		0x080
#define CLK_CFG_5_OFS		0x090
#define CLK_CFG_6_OFS		0x0A0
#define CLK_CFG_7_OFS		0x0B0
#define CLK_CFG_8_OFS		0x0C0

#define MAX_MUX_CLKSRC		16

struct mux_t {
	const char *name;
	u32 reg;
	struct bit_range_t sel_bits;
	u8 pdn_bit;
	u8 upd_bit;
	enum CKGEN_CLK ckgen_clk;
	enum mux_clksrc_id clksrc[MAX_MUX_CLKSRC + 1];
};

static struct mux_t g_mux[] = {
	[MUX_AXI] = {
		.name = "MUX_AXI",
		.ckgen_clk = FM_HD_FAXI_CK,
		.reg = CLK_CFG_0_OFS,	.sel_bits = { 0,  0},	.pdn_bit =  7,
		.upd_bit = 0,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll1_d4,
		},
	},
	[MUX_MEM] = {
		.name = "MUX_MEM",
		.ckgen_clk = FM_CKGEN_CLK_NULL,
		.reg = CLK_CFG_0_OFS,	.sel_bits = { 9,  8},	.pdn_bit = 15,
		.upd_bit = 1,
		.clksrc = {
			[0] = clk26m,
			[1] = dmpll_ck,
			[2] = f26m_mem_ckgen_ck_occ,
		},
	},
	[MUX_DDRPHY] = {
		.name = "MUX_DDRPHY",
		.ckgen_clk = FM_HF_FDDRPHYCFG_CK,
		.reg = CLK_CFG_0_OFS,	.sel_bits = {16, 16},	.pdn_bit = 23,
		.upd_bit = 2,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll1_d8,
		},
	},
	[MUX_MM] = {
		.name = "MUX_MM",
		.ckgen_clk = FM_HF_FMM_CK,
		.reg = CLK_CFG_0_OFS,	.sel_bits = {25, 24},	.pdn_bit = 31,
		.upd_bit = 3,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll_d3,
			[2] = univpll_d3,
			[3] = vencpll_ck,
		},
	},
	[MUX_PWM] = {
		.name = "MUX_PWM",
		.ckgen_clk = FM_F_FPWM_CK,
		.reg = CLK_CFG_1_OFS,	.sel_bits = { 1,  0},	.pdn_bit =  7,
		.upd_bit = 4,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d2,
			[2] = univpll2_d4,
		},
	},
	[MUX_VDEC] = {
		.name = "MUX_VDEC",
		.ckgen_clk = FM_HF_FVDEC_CK,
		.reg = CLK_CFG_1_OFS,	.sel_bits = {10,  8},	.pdn_bit = 15,
		.upd_bit = 5,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll_d3,
			[2] = syspll1_d2,
			[3] = univpll1_d2,
			[4] = syspll1_d4,
		},
	},
	[MUX_MFG] = {
		.name = "MUX_MFG",
		.ckgen_clk = FM_HF_FMFG_CK,
		.reg = CLK_CFG_1_OFS,	.sel_bits = {25, 24},	.pdn_bit = 31,
		.upd_bit = 7,
		.clksrc = {
			[0] = clk26m,
			[1] = mmpll_ck,
			[2] = univpll_d3,
			[3] = syspll_d3,
		},
	},
	[MUX_CAMTG] = {
		.name = "MUX_CAMTG",
		.ckgen_clk = FM_HF_FCAMTG_CK,
		.reg = CLK_CFG_2_OFS,	.sel_bits = { 1,  0},	.pdn_bit =  7,
		.upd_bit = 8,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll_d26,
			[2] = univpll2_d2,
		},
	},
	[MUX_UART] = {
		.name = "MUX_UART",
		.ckgen_clk = FM_F_FUART_CK,
		.reg = CLK_CFG_2_OFS,	.sel_bits = { 8,  8},	.pdn_bit = 15,
		.upd_bit = 9,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d8,
		},
	},
	[MUX_SPI] = {
		.name = "MUX_SPI",
		.ckgen_clk = FM_HF_FSPI_CK,
		.reg = CLK_CFG_2_OFS,	.sel_bits = {16, 16},	.pdn_bit = 23,
		.upd_bit = 10,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll3_d2,
		},
	},
	[MUX_MSDC30_0]  = {
		.name = "MUX_MSDC30_0",
		.ckgen_clk = FM_HF_FMSDC_30_0_CK,
		.reg = CLK_CFG_3_OFS,	.sel_bits = { 2,  0},	.pdn_bit =  7,
		.upd_bit = 12,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d2,
			[2] = msdcpll_d2,
			[3] = univpll1_d4,
			[4] = syspll2_d2,
			[5] = syspll_d7,
			[6] = univpll_d7,
		},
	},
	[MUX_MSDC30_1] = {
		.name = "MUX_MSDC30_1",
		.ckgen_clk = FM_HF_FMSDC_30_1_CK,
		.reg = CLK_CFG_3_OFS,	.sel_bits = {10,  8},	.pdn_bit = 15,
		.upd_bit = 13,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d2,
			[2] = msdcpll_d2,
			[3] = univpll1_d4,
			[4] = syspll2_d2,
			[5] = syspll_d7,
			[6] = univpll_d7,
		},
	},
	[MUX_MSDC30_2] = {
		.name = "MUX_MSDC30_2",
		.ckgen_clk = FM_HF_FMSDC_30_2_CK,
		.reg = CLK_CFG_3_OFS,	.sel_bits = {18, 16},	.pdn_bit = 23,
		.upd_bit = 14,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d2,
			[2] = msdcpll_d2,
			[3] = univpll1_d4,
			[4] = syspll2_d2,
			[5] = syspll_d7,
			[6] = univpll_d7,
		},
	},
	[MUX_MSDC50_3_HCLK]  = {
		.name = "MUX_MSDC50_3_HCLK",
		.ckgen_clk = FM_HF_FMSDC_50_3_HCLK_CK,
		.reg = CLK_CFG_4_OFS,	.sel_bits = { 1,  0},	.pdn_bit =  7,
		.upd_bit = 15,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll1_d2,
			[2] = syspll2_d2,
			[3] = syspll4_d2,
		},
	},
	[MUX_MSDC50_3] = {
		.name = "MUX_MSDC50_3",
		.ckgen_clk = FM_HF_FMSDC_50_3_CK,
		.reg = CLK_CFG_4_OFS,	.sel_bits = {11,  8},	.pdn_bit = 15,
		.upd_bit = 16,
		.clksrc = {
			[0] = clk26m,
			[1] = msdcpll_ck,
			[2] = msdcpll_d2,
			[3] = univpll1_d4,
			[4] = syspll2_d2,
			[5] = syspll_d7,
			[6] = msdcpll_d4,
			[7] = univpll_d2,
			[8] = univpll1_d2,
		},
	},
	[MUX_AUDIO] = {
		.name = "MUX_AUDIO",
		.ckgen_clk = FM_HF_FAUDIO_CK,
		.reg = CLK_CFG_4_OFS,	.sel_bits = {17, 16},	.pdn_bit = 23,
		.upd_bit = 17,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll3_d4,
			[2] = syspll4_d4,
			[3] = syspll1_d16,
		},
	},
	[MUX_AUDINTBUS] = {
		.name = "MUX_AUDINTBUS",
		.ckgen_clk = FM_HF_FAUD_INTBUS_CK,
		.reg = CLK_CFG_4_OFS,	.sel_bits = {25, 24},	.pdn_bit = 31,
		.upd_bit = 18,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll1_d4,
			[2] = syspll4_d2,
		},
	},
	[MUX_PMICSPI] = {
		.name = "MUX_PMICSPI",
		.ckgen_clk = FM_HF_FPMICSPI_CK,
		.reg = CLK_CFG_5_OFS,	.sel_bits = { 0,  0},	.pdn_bit =  7,
		.upd_bit = 19,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll_d26,
		},
	},
	[MUX_SCP] = {
		.name = "MUX_SCP",
		.ckgen_clk = FM_HF_FSCP_CK,
		.reg = CLK_CFG_5_OFS,	.sel_bits = { 9,  8},	.pdn_bit = 15,
		.upd_bit = 20,
		.clksrc = {
			[0] = clk26m,
			[1] = mpll_208m_ck,
			[2] = syspll1_d4,
		},
	},
	[MUX_ATB] = {
		.name = "MUX_ATB",
		.ckgen_clk = FM_HF_FATB_CK,
		.reg = CLK_CFG_5_OFS,	.sel_bits = {16, 16},	.pdn_bit = 23,
		.upd_bit = 21,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll1_d2,
		},
	},
	[MUX_MJC] = {
		.name = "MUX_MJC",
		.ckgen_clk = FM_CKGEN_CLK_NULL,
		.reg = CLK_CFG_5_OFS,	.sel_bits = {25, 24},	.pdn_bit = 31,
		.upd_bit = 22,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll_d5,
			[2] = univpll_d5,
		},
	},
	[MUX_DPI0] = {
		.name = "MUX_DPI0",
		.ckgen_clk = FM_HF_FDPI0_CK,
		.reg = CLK_CFG_6_OFS,	.sel_bits = { 2,  0},	.pdn_bit =  7,
		.upd_bit = 23,
		.clksrc = {
			[0] = clk26m,
			[1] = lvdspll_ck,
			[2] = lvdspll_d2,
			[3] = lvdspll_d4,
			[4] = lvdspll_d8,
		},
	},
	[MUX_SCAM] = {
		.name = "MUX_SCAM",
		.ckgen_clk = FM_HF_FSCAM_CK,
		.reg = CLK_CFG_6_OFS,	.sel_bits = { 8,  8},	.pdn_bit = 15,
		.upd_bit = 24,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll3_d2,
		},
	},
	[MUX_AUD1] = {
		.name = "MUX_AUD1",
		.ckgen_clk = FM_HF_FAUD_1_CK_PRE,
		.reg = CLK_CFG_6_OFS,	.sel_bits = {16, 16},	.pdn_bit = 23,
		.upd_bit = 25,
		.clksrc = {
			[0] = clk26m,
			[1] = apll1_ck,
		},
	},
	[MUX_AUD2] = {
		.name = "MUX_AUD2",
		.ckgen_clk = FM_HF_FAUD_2_CK_PRE,
		.reg = CLK_CFG_6_OFS,	.sel_bits = {24, 24},	.pdn_bit = 31,
		.upd_bit = 26,
		.clksrc = {
			[0] = clk26m,
			[1] = apll2_ck,
		},
	},
	[MUX_DPI1] = {
		.name = "MUX_DPI1",
		.ckgen_clk = FM_HF_FDPI1_CK,
		.reg = CLK_CFG_7_OFS,	.sel_bits = { 2,  0},	.pdn_bit =  7,
		.upd_bit = 6,
		.clksrc = {
			[0] = clk26m,
			[1] = tvdpll_d2,
			[2] = tvdpll_d4,
			[3] = tvdpll_d8,
			[4] = tvdpll_d16,
		},
	},
	[MUX_UFOENC] = {
		.name = "MUX_UFOENC",
		.ckgen_clk = FM_HF_FUFOENC_CK,
		.reg = CLK_CFG_7_OFS,	.sel_bits = {10,  8},	.pdn_bit = 15,
		.upd_bit = 27,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll_d2,
			[2] = syspll_d2,
			[3] = univpll_d2p5,
			[4] = syspll_d2p5,
			[5] = univpll_d3,
			[6] = syspll_d3,
			[7] = syspll1_d2,
		},
	},
	[MUX_UFODEC] = {
		.name = "MUX_UFODEC",
		.ckgen_clk = FM_HF_FUFODEC_CK,
		.reg = CLK_CFG_7_OFS,	.sel_bits = {18, 16},	.pdn_bit = 23,
		.upd_bit = 28,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll_d2,
			[2] = univpll_d2p5,
			[3] = syspll_d2p5,
			[4] = univpll_d3,
			[5] = syspll_d3,
			[6] = syspll1_d2,
		},
	},
	[MUX_ETH] = {
		.name = "MUX_ETH",
		.ckgen_clk = FM_HF_FETH_CK,
		.reg = CLK_CFG_7_OFS,	.sel_bits = {27, 24},	.pdn_bit = 31,
		.upd_bit = 29,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll3_d4,
			[2] = univpll2_d8,
			[3] = lvdspll_eth_ck,
			[4] = univpll_d26,
			[5] = syspll2_d8,
			[6] = syspll4_d4,
			[7] = univpll3_d8,
			[8] = clk26m,
		},
	},
	[MUX_ONFI] = {
		.name = "MUX_ONFI",
		.ckgen_clk = FM_HF_FONFI_CK,
		.reg = CLK_CFG_8_OFS,	.sel_bits = { 2,  0},	.pdn_bit =  7,
		.upd_bit = 30,
		.clksrc = {
			[0] = clk26m,
			[1] = syspll2_d2,
			[2] = syspll_d7,
			[3] = univpll3_d2,
			[4] = syspll2_d4,
			[5] = univpll3_d4,
			[6] = syspll4_d4,
		},
	},
	[MUX_SNFI] = {
		.name = "MUX_SNFI",
		.ckgen_clk = FM_HF_FSNFI_CK,
		.reg = CLK_CFG_8_OFS,	.sel_bits = {11,  8},	.pdn_bit = 15,
		.upd_bit = 31,
		.clksrc = {
			[0] = clk26m,
			[1] = univpll2_d8,
			[2] = univpll3_d4,
			[3] = syspll4_d2,
			[4] = univpll2_d4,
			[5] = univpll3_d2,
			[6] = syspll1_d4,
			[7] = univpll1_d4,
			[8] = clk26m,
		},
	},
	[MUX_HDMI] = {
		.name = "MUX_HDMI",
		.ckgen_clk = FM_CKGEN_CLK_NULL,
		.reg = CLK_CFG_8_OFS,	.sel_bits = {17, 16},	.pdn_bit = 23,
		.upd_bit = 11,
		.clksrc = {
			[0] = clk26m,
			[1] = fhdmi_cts_ck,
			[2] = fhdmi_cts_ck_d2,
			[3] = fhdmi_cts_ck_d3,
		},
	},
	[MUX_RTC] = {
		.name = "MUX_RTC",
		.ckgen_clk = FM_CKGEN_CLK_NULL,
		.reg = CLK_CFG_8_OFS,	.sel_bits = {25, 24},	.pdn_bit = 31,
		.upd_bit = 32,
		.clksrc = {
			[0] = clkrtc_int,
			[1] = clkrtc_ext,
			[2] = clk26m,
			[3] = univpll3_d8,
		},
	},
};

struct mux_clksrc_t {
	const char *name;
	struct pll *pll;
};

static struct mux_clksrc_t g_mux_clksrc[] = {
	[clk26m]		= { .name = "clk26m",                },
	[clkrtc_ext]		= { .name = "clkrtc_ext",            },
	[clkrtc_int]		= { .name = "clkrtc_int",            },
	[f26m_mem_ckgen_ck_occ]	= { .name = "f26m_mem_ckgen_ck_occ", },
	[apll1_ck]		= { .name = "apll1_ck",		.pll = &plls[APLL1], },
	[apll2_ck]		= { .name = "apll2_ck",		.pll = &plls[APLL2], },
	[dmpll_ck]		= { .name = "dmpll_ck",              },
	[fhdmi_cts_ck]		= { .name = "fhdmi_cts_ck",          },
	[fhdmi_cts_ck_d2]	= { .name = "fhdmi_cts_ck_d2",       },
	[fhdmi_cts_ck_d3]	= { .name = "fhdmi_cts_ck_d3",       },
	[lvdspll_ck]		= { .name = "lvdspll_ck",	.pll = &plls[LVDSPLL], },
	[lvdspll_d2]		= { .name = "lvdspll_d2",	.pll = &plls[LVDSPLL], },
	[lvdspll_d4]		= { .name = "lvdspll_d4",	.pll = &plls[LVDSPLL], },
	[lvdspll_d8]		= { .name = "lvdspll_d8",	.pll = &plls[LVDSPLL], },
	[lvdspll_eth_ck]	= { .name = "lvdspll_eth_ck",	.pll = &plls[LVDSPLL], },
	[mmpll_ck]		= { .name = "mmpll_ck",		.pll = &plls[MMPLL], },
	[mpll_208m_ck]		= { .name = "mpll_208m_ck",	.pll = &plls[MPLL], },
	[msdcpll_ck]		= { .name = "msdcpll_ck",	.pll = &plls[MSDCPLL], },
	[msdcpll_d2]		= { .name = "msdcpll_d2",	.pll = &plls[MSDCPLL], },
	[msdcpll_d4]		= { .name = "msdcpll_d4",	.pll = &plls[MSDCPLL], },
	[syspll1_d16]		= { .name = "syspll1_d16",	.pll = &plls[MAINPLL], },
	[syspll1_d2]		= { .name = "syspll1_d2",	.pll = &plls[MAINPLL], },
	[syspll1_d4]		= { .name = "syspll1_d4",	.pll = &plls[MAINPLL], },
	[syspll1_d8]		= { .name = "syspll1_d8",	.pll = &plls[MAINPLL], },
	[syspll2_d2]		= { .name = "syspll2_d2",	.pll = &plls[MAINPLL], },
	[syspll2_d4]		= { .name = "syspll2_d4",	.pll = &plls[MAINPLL], },
	[syspll2_d8]		= { .name = "syspll2_d8",	.pll = &plls[MAINPLL], },
	[syspll3_d2]		= { .name = "syspll3_d2",	.pll = &plls[MAINPLL], },
	[syspll3_d4]		= { .name = "syspll3_d4",	.pll = &plls[MAINPLL], },
	[syspll4_d2]		= { .name = "syspll4_d2",	.pll = &plls[MAINPLL], },
	[syspll4_d4]		= { .name = "syspll4_d4",	.pll = &plls[MAINPLL], },
	[syspll_d2]		= { .name = "syspll_d2",	.pll = &plls[MAINPLL], },
	[syspll_d2p5]		= { .name = "syspll_d2p5",	.pll = &plls[MAINPLL], },
	[syspll_d3]		= { .name = "syspll_d3",	.pll = &plls[MAINPLL], },
	[syspll_d5]		= { .name = "syspll_d5",	.pll = &plls[MAINPLL], },
	[syspll_d7]		= { .name = "syspll_d7",	.pll = &plls[MAINPLL], },
	[tvdpll_d16]		= { .name = "tvdpll_d16",	.pll = &plls[TVDPLL], },
	[tvdpll_d2]		= { .name = "tvdpll_d2",	.pll = &plls[TVDPLL], },
	[tvdpll_d4]		= { .name = "tvdpll_d4",	.pll = &plls[TVDPLL], },
	[tvdpll_d8]		= { .name = "tvdpll_d8",	.pll = &plls[TVDPLL], },
	[univpll1_d2]		= { .name = "univpll1_d2",	.pll = &plls[UNIVPLL], },
	[univpll1_d4]		= { .name = "univpll1_d4",	.pll = &plls[UNIVPLL], },
	[univpll2_d2]		= { .name = "univpll2_d2",	.pll = &plls[UNIVPLL], },
	[univpll2_d4]		= { .name = "univpll2_d4",	.pll = &plls[UNIVPLL], },
	[univpll2_d8]		= { .name = "univpll2_d8",	.pll = &plls[UNIVPLL], },
	[univpll3_d2]		= { .name = "univpll3_d2",	.pll = &plls[UNIVPLL], },
	[univpll3_d4]		= { .name = "univpll3_d4",	.pll = &plls[UNIVPLL], },
	[univpll3_d8]		= { .name = "univpll3_d8",	.pll = &plls[UNIVPLL], },
	[univpll_d2]		= { .name = "univpll_d2",	.pll = &plls[UNIVPLL], },
	[univpll_d26]		= { .name = "univpll_d26",	.pll = &plls[UNIVPLL], },
	[univpll_d2p5]		= { .name = "univpll_d5",	.pll = &plls[UNIVPLL], },
	[univpll_d3]		= { .name = "univpll_d3",	.pll = &plls[UNIVPLL], },
	[univpll_d5]		= { .name = "univpll_d5",	.pll = &plls[UNIVPLL], },
	[univpll_d7]		= { .name = "univpll_d7",	.pll = &plls[UNIVPLL], },
	[vencpll_ck]		= { .name = "vencpll_ck",	.pll = &plls[VENCPLL], },
};

static struct mux_t *mux_from_id(enum mux_id_t mux_id)
{
	struct mux_t *mux = &g_mux[mux_id];

	if (mux_id <= MUX_NULL || mux_id >= MUX_END || !mux->name || !mux->reg)
		return NULL;

	return mux;
}

static struct mux_clksrc_t *mux_clksrc_from_id(enum mux_clksrc_id id)
{
	struct mux_clksrc_t *clksrc = &g_mux_clksrc[id];

	if (id <= mux_clksrc_null || id >= mux_clksrc_end || !clksrc->name)
		return NULL;

	return clksrc;
}

static u32 get_mux_sel(enum mux_id_t mux_id)
{
	u8 h, l;
	u32 mask;
	u32 r = (u32)(-1);
	struct mux_t *mux = mux_from_id(mux_id);

	if (!mux)
		return r;

	h = mux->sel_bits.h;
	l = mux->sel_bits.l;
	mask = GENMASK(h, l);
	r = (clk_readl(clk_cksys_base + mux->reg) & mask) >> l;

	return r;
}

static BOOL update_mux_setting(struct mux_t *mux, u8 h, u8 l, u32 v)
{
	u32 old_reg, new_reg;

	old_reg = clk_readl(clk_cksys_base + mux->reg);
	new_reg = ALT_BITS(old_reg, h, l, v);

	if (old_reg == new_reg)
		return TRUE;

	clk_writel(clk_cksys_base + mux->reg, new_reg);

	if (mux->upd_bit < 32)
		clk_writel(CLK_CFG_UPDATE, BIT(mux->upd_bit));
	else
		clk_writel(CLK_CFG_UPDATE_1, BIT(mux->upd_bit - 32));

	return TRUE;
}

static BOOL set_mux_sel(enum mux_id_t mux_id, u32 value)
{
	struct mux_t *mux = mux_from_id(mux_id);

	if (!mux)
		return FALSE;

	return update_mux_setting(mux, mux->sel_bits.h, mux->sel_bits.l, value);
}

static BOOL enable_mux_op(enum mux_id_t mux_id)
{
	struct mux_t *mux = mux_from_id(mux_id);

	if (!mux)
		return FALSE;

	return update_mux_setting(mux, mux->pdn_bit, mux->pdn_bit, 0);
}

static BOOL disable_mux_op(enum mux_id_t mux_id)
{
	struct mux_t *mux = mux_from_id(mux_id);

	if (!mux)
		return FALSE;

	return update_mux_setting(mux, mux->pdn_bit, mux->pdn_bit, 1);
}

static BOOL mux_is_on(enum mux_id_t mux_id)
{
	struct mux_t *mux = mux_from_id(mux_id);

	if (!mux)
		return FALSE;

	return !(clk_readl(clk_cksys_base + mux->reg) & BIT(mux->pdn_bit));
}

static struct pll *get_pll_from_mux_sel(enum mux_id_t mux_id, u32 sel)
{
	enum mux_clksrc_id clksrc_id = mux_from_id(mux_id)->clksrc[sel];

	return mux_clksrc_from_id(clksrc_id)->pll;
}

static struct clkmux muxs[NR_MUXS] = {
	[MT_MUX_AXI] = {
		.mux_id = MUX_AXI,
		.name = "MUX_AXI",
	},
	[MT_MUX_MEM] = {
		.mux_id = MUX_MEM,
		.name = "MUX_MEM",
	},
	[MT_MUX_DDRPHY] = {
		.mux_id = MUX_DDRPHY,
		.name = "MUX_DDRPHY",
	},
	[MT_MUX_MM] = {
		.mux_id = MUX_MM,
		.name = "MUX_MM",
	},
	[MT_MUX_PWM] = {
		.mux_id = MUX_PWM,
		.name = "MUX_PWM",
	},
	[MT_MUX_VDEC] = {
		.mux_id = MUX_VDEC,
		.name = "MUX_VDEC",
	},
	[MT_MUX_MFG] = {
		.mux_id = MUX_MFG,
		.name = "MUX_MFG",
	},
	[MT_MUX_CAMTG] = {
		.mux_id = MUX_CAMTG,
		.name = "MUX_CAMTG",
	},
	[MT_MUX_UART] = {
		.mux_id = MUX_UART,
		.name = "MUX_UART",
	},
	[MT_MUX_SPI] = {
		.mux_id = MUX_SPI,
		.name = "MUX_SPI",
	},
	[MT_MUX_MSDC30_0] = {
		.mux_id = MUX_MSDC30_0,
		.name = "MUX_MSDC30_0",
	},
	[MT_MUX_MSDC30_1] = {
		.mux_id = MUX_MSDC30_1,
		.name = "MUX_MSDC30_1",
	},
	[MT_MUX_MSDC30_2] = {
		.mux_id = MUX_MSDC30_2,
		.name = "MUX_MSDC30_2",
	},
	[MT_MUX_MSDC50_3_HCLK] = {
		.mux_id = MUX_MSDC50_3_HCLK,
		.name = "MUX_MSDC50_3_HCLK",
	},
	[MT_MUX_MSDC50_3] = {
		.mux_id = MUX_MSDC50_3,
		.name = "MUX_MSDC50_3",
		.siblings = &muxs[MT_MUX_MSDC50_3_HCLK],
	},
	[MT_MUX_AUDIO] = {
		.mux_id = MUX_AUDIO,
		.name = "MUX_AUDIO",
	},
	[MT_MUX_AUDINTBUS] = {
		.mux_id = MUX_AUDINTBUS,
		.name = "MUX_AUDINTBUS",
		.siblings = &muxs[MT_MUX_AUDIO],
	},
	[MT_MUX_PMICSPI] = {
		.mux_id = MUX_PMICSPI,
		.name = "MUX_PMICSPI",
	},
	[MT_MUX_SCP] = {
		.mux_id = MUX_SCP,
		.name = "MUX_SCP",
	},
	[MT_MUX_ATB] = {
		.mux_id = MUX_ATB,
		.name = "MUX_ATB",
	},
	[MT_MUX_MJC] = {
		.mux_id = MUX_MJC,
		.name = "MUX_MJC",
	},
	[MT_MUX_DPI0] = {
		.mux_id = MUX_DPI0,
		.name = "MUX_DPI0",
	},
	[MT_MUX_SCAM] = {
		.mux_id = MUX_SCAM,
		.name = "MUX_SCAM",
		},
	[MT_MUX_AUD1] = {
		.mux_id = MUX_AUD1,
		.name = "MUX_AUD1",
	},
	[MT_MUX_AUD2] = {
		.mux_id = MUX_AUD2,
		.name = "MUX_AUD2",
	},
	[MT_MUX_DPI1] = {
		.mux_id = MUX_DPI1,
		.name = "MUX_DPI1",
	},
	[MT_MUX_UFOENC] = {
		.mux_id = MUX_UFOENC,
		.name = "MUX_UFOENC",
	},
	[MT_MUX_UFODEC] = {
		.mux_id = MUX_UFODEC,
		.name = "MUX_UFODEC",
		.siblings = &muxs[MT_MUX_UFOENC],
	},
	[MT_MUX_ETH] = {
		.mux_id = MUX_ETH,
		.name = "MUX_ETH",
	},
	[MT_MUX_ONFI] = {
		.mux_id = MUX_ONFI,
		.name = "MUX_ONFI",
	},
	[MT_MUX_SNFI] = {
		.mux_id = MUX_SNFI,
		.name = "MUX_SNFI",
	},
	[MT_MUX_HDMI] = {
		.mux_id = MUX_HDMI,
		.name = "MUX_HDMI",
	},
	[MT_MUX_RTC] = {
		.mux_id = MUX_RTC,
		.name = "MUX_RTC",
	},
};


#else /* !NEW_MUX */

static struct clkmux_ops clkmux_ops;
static struct clkmux_ops audio_clkmux_ops;

static struct clkmux muxs[NR_MUXS] = {
	[MT_MUX_AXI] = {
		.name = __stringify(MUX_AXI),
		.sel_mask = 0x00000001,
		.pdn_mask = 0x00000080,
		.upd_bit = 0,
		.offset = 0,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MEM] = {
		.name = __stringify(MUX_MEM),
		.sel_mask = 0x00000300,
		.pdn_mask = 0x00008000,
		.upd_bit = 1,
		.offset = 8,
		.nr_inputs = 3,
		.ops = &clkmux_ops,
	},
	[MT_MUX_DDRPHY] = {
		.name = __stringify(MUX_DDRPHY),
		.sel_mask = 0x00010000,
		.pdn_mask = 0x00800000,
		.upd_bit = 2,
		.offset = 16,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MM] = {
		.name = __stringify(MUX_MM),
		.sel_mask = 0x03000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 3,
		.offset = 24,
		.nr_inputs = 4,
		.ops = &clkmux_ops,
	},
	[MT_MUX_PWM] = {
		.name = __stringify(MUX_PWM),
		.sel_mask = 0x00000003,
		.pdn_mask = 0x00000080,
		.upd_bit = 4,
		.offset = 0,
		.nr_inputs = 3,
		.ops = &clkmux_ops,
	},
	[MT_MUX_VDEC] = {
		.name = __stringify(MUX_VDEC),
		.sel_mask = 0x00000700,
		.pdn_mask = 0x00008000,
		.upd_bit = 5,
		.offset = 8,
		.nr_inputs = 5,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MFG] = {
		.name = __stringify(MUX_MFG),
		.sel_mask = 0x03000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 7,
		.offset = 24,
		.nr_inputs = 4,
		.ops = &clkmux_ops,
		.pll = &plls[MMPLL],
	},
	[MT_MUX_CAMTG] = {
		.name = __stringify(MUX_CAMTG),
		.sel_mask = 0x00000003,
		.pdn_mask = 0x00000080,
		.upd_bit = 8,
		.offset = 0,
		.nr_inputs = 3,
		.ops = &clkmux_ops,
		.pll = &plls[UNIVPLL],
	},
	[MT_MUX_UART] = {
		.name = __stringify(MUX_UART),
		.sel_mask = 0x00000100,
		.pdn_mask = 0x00008000,
		.upd_bit = 9,
		.offset = 8,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_SPI] = {
		.name = __stringify(MUX_SPI),
		.sel_mask = 0x00010000,
		.pdn_mask = 0x00800000,
		.upd_bit = 10,
		.offset = 16,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MSDC30_0] = {
		.name = __stringify(MUX_MSDC30_0),
		.sel_mask = 0x00000007,
		.pdn_mask = 0x00000080,
		.upd_bit = 12,
		.offset = 0,
		.nr_inputs = 7,
		.ops = &clkmux_ops,
		.pll = &plls[MSDCPLL],
	},
	[MT_MUX_MSDC30_1] = {
		.name = __stringify(MUX_MSD30_1),
		.sel_mask = 0x00000700,
		.pdn_mask = 0x00008000,
		.upd_bit = 13,
		.offset = 8,
		.nr_inputs = 7,
		.ops = &clkmux_ops,
		.pll = &plls[MSDCPLL],
	},
	[MT_MUX_MSDC30_2] = {
		.name = __stringify(MUX_MSDC30_2),
		.sel_mask = 0x00070000,
		.pdn_mask = 0x00800000,
		.upd_bit = 14,
		.offset = 16,
		.nr_inputs = 7,
		.ops = &clkmux_ops,
		.pll = &plls[MSDCPLL],
	},
	[MT_MUX_MSDC50_3_HCLK] = {
		.name = __stringify(MUX_MSDC50_3_HCLK),
		.sel_mask = 0x00000003,
		.pdn_mask = 0x00000080,
		.upd_bit = 15,
		.offset = 0,
		.nr_inputs = 4,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MSDC50_3] = {
		.name = __stringify(MUX_MSDC30_3),
		.sel_mask = 0x00000F00,
		.pdn_mask = 0x00008000,
		.upd_bit = 16,
		.offset = 8,
		.nr_inputs = 9,
		.ops = &clkmux_ops,
		.pll = &plls[MSDCPLL],
		.siblings = &muxs[MT_MUX_MSDC50_3_HCLK],
	},
	[MT_MUX_AUDIO] = {
		.name = __stringify(MUX_AUDIO),
		.sel_mask = 0x00030000,
		.pdn_mask = 0x00800000,
		.upd_bit = 17,
		.offset = 16,
		.nr_inputs = 4,
		.ops = &audio_clkmux_ops,
	},
	[MT_MUX_AUDINTBUS] = {
		.name = __stringify(MUX_AUDINTBUS),
		.sel_mask = 0x03000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 18,
		.offset = 24,
		.nr_inputs = 3,
		.ops = &audio_clkmux_ops,
		.siblings = &muxs[MT_MUX_AUDIO],
	},
	[MT_MUX_PMICSPI] = {
		.name = __stringify(MUX_PMICSPI),
		.sel_mask = 0x00000001,
		.pdn_mask = 0x00000080,
		.upd_bit = 19,
		.offset = 0,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_SCP] = {
		.name = __stringify(MUX_SCP),
		.sel_mask = 0x00000300,
		.pdn_mask = 0x00008000,
		.upd_bit = 20,
		.offset = 8,
		.nr_inputs = 3,
		.ops = &clkmux_ops,
		.pll = &plls[MPLL],
	},
	[MT_MUX_ATB] = {
		.name = __stringify(MUX_ATB),
		.sel_mask = 0x00010000,
		.pdn_mask = 0x00800000,
		.upd_bit = 21,
		.offset = 16,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
	},
	[MT_MUX_MJC] = {
		.name = __stringify(MUX_MJC),
		.sel_mask = 0x03000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 22,
		.offset = 24,
		.nr_inputs = 3,
		.ops = &clkmux_ops,
	},
	[MT_MUX_DPI0] = {
		.name = __stringify(MUX_DPI0),
		.sel_mask = 0x00000007,
		.pdn_mask = 0x00000080,
		.upd_bit = 23,
		.offset = 0,
		.nr_inputs = 5,
		.ops = &clkmux_ops,
		.pll = &plls[LVDSPLL],
	},
	[MT_MUX_SCAM] = {
		.name = __stringify(MUX_SCAM),
		.sel_mask = 0x00000100,
		.pdn_mask = 0x00008000,
		.upd_bit = 24,
		.offset = 8,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
		},
	[MT_MUX_AUD1] = {
		.name = __stringify(MUX_AUD1),
		.sel_mask = 0x00010000,
		.pdn_mask = 0x00800000,
		.upd_bit = 25,
		.offset = 16,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
		.pll = &plls[AUD1PLL],
	},
	[MT_MUX_AUD2] = {
		.name = __stringify(MUX_AUD2),
		.sel_mask = 0x01000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 26,
		.offset = 24,
		.nr_inputs = 2,
		.ops = &clkmux_ops,
		.pll = &plls[AUD2PLL],
	},
	[MT_MUX_DPI1] = {
		.name = __stringify(MUX_DPI1),
		.sel_mask = 0x00000007,
		.pdn_mask = 0x00000080,
		.upd_bit = 6,
		.offset = 0,
		.nr_inputs = 5,
		.ops = &clkmux_ops,
		.pll = &plls[TVDPLL],
	},
	[MT_MUX_UFOENC] = {
		.name = __stringify(MUX_UFOENC),
		.sel_mask = 0x00000700,
		.pdn_mask = 0x00008000,
		.upd_bit = 27,
		.offset = 8,
		.nr_inputs = 8,
		.ops = &clkmux_ops,
		.pll = &plls[UNIVPLL],
	},
	[MT_MUX_UFODEC] = {
		.name = __stringify(MUX_UFODEC),
		.sel_mask = 0x00070000,
		.pdn_mask = 0x00800000,
		.upd_bit = 28,
		.offset = 16,
		.nr_inputs = 7,
		.ops = &clkmux_ops,
		.pll = &plls[UNIVPLL],
		.siblings = &muxs[MT_MUX_UFOENC],
	},
	[MT_MUX_ETH] = {
		.name = __stringify(MUX_ETH),
		.sel_mask = 0x0F000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 29,
		.offset = 24,
		.nr_inputs = 9,
		.ops = &clkmux_ops,
	},
	[MT_MUX_ONFI] = {
		.name = __stringify(MUX_ONFI),
		.sel_mask = 0x00000007,
		.pdn_mask = 0x00000080,
		.upd_bit = 30,
		.offset = 0,
		.nr_inputs = 7,
		.ops = &clkmux_ops,
	},
	[MT_MUX_SNFI] = {
		.name = __stringify(MUX_SNFI),
		.sel_mask = 0x00000F00,
		.pdn_mask = 0x00008000,
		.upd_bit = 31,
		.offset = 8,
		.nr_inputs = 9,
		.ops = &clkmux_ops,
	},
	[MT_MUX_HDMI] = {
		.name = __stringify(MUX_HDMI),
		.sel_mask = 0x00030000,
		.pdn_mask = 0x00800000,
		.upd_bit = 11,
		.offset = 16,
		.nr_inputs = 4,
		.ops = &clkmux_ops,
	},
	[MT_MUX_RTC] = {
		.name = __stringify(MUX_RTC),
		.sel_mask = 0x03000000,
		.pdn_mask = 0x80000000,
		.upd_bit = 32,
		.offset = 24,
		.nr_inputs = 4,
		.ops = &clkmux_ops,
	},
};

#endif /* NEW_MUX */

static struct clkmux *id_to_mux(unsigned int id)
{
	return id < NR_MUXS ? muxs + id : NULL;
}

#define mux_to_id(mux) (mux - muxs)

extern void aee_rr_rec_clk(int id, u32 val);

#if NEW_MUX
#else /* !NEW_MUX */

static void clkmux_sel_op(struct clkmux *mux, unsigned clksrc)
{
#ifdef MUX_LOG_TOP
	clk_dbg("[%s]: mux->name=%s, clksrc=%d\n", __func__, mux->name, clksrc);
#endif

	clk_writel(mux->base_addr + 8, mux->sel_mask);           /* clr */
	clk_writel(mux->base_addr + 4, (clksrc << mux->offset)); /* set */

	if (mux->upd_bit < 32)
		clk_writel(CLK_CFG_UPDATE, BIT(mux->upd_bit));
	else
		clk_writel(CLK_CFG_UPDATE_1, BIT(mux->upd_bit - 32));

#ifdef CONFIG_MTK_RAM_CONSOLE
	do {
		unsigned int id = mux_to_id(mux);

		if (id < 4)
			aee_rr_rec_clk(0, clk_readl(mux->base_addr));
		else if (id < 7)
			aee_rr_rec_clk(1, clk_readl(mux->base_addr));
		else if (id < 10)
			aee_rr_rec_clk(2, clk_readl(mux->base_addr));
		else if (id < 13)
			aee_rr_rec_clk(3, clk_readl(mux->base_addr));
		else if (id < 17)
			aee_rr_rec_clk(4, clk_readl(mux->base_addr));
		else if (id < 20)
			aee_rr_rec_clk(5, clk_readl(mux->base_addr));
		else if (id < 24)
			aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	} while (0);
#endif /* CONFIG_MTK_RAM_CONSOLE */
}

static void clkmux_enable_op(struct clkmux *mux)
{
#ifdef MUX_LOG
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif

	clk_writel(mux->base_addr + 8, mux->pdn_mask);	/* write clr reg */

	if (mux->upd_bit < 32)
		clk_writel(CLK_CFG_UPDATE, BIT(mux->upd_bit));
	else
		clk_writel(CLK_CFG_UPDATE_1, BIT(mux->upd_bit - 32));

#ifdef CONFIG_MTK_RAM_CONSOLE
	do {
		unsigned int id = mux_to_id(mux);

		if (id < 4)
			aee_rr_rec_clk(0, clk_readl(mux->base_addr));
		else if (id < 7)
			aee_rr_rec_clk(1, clk_readl(mux->base_addr));
		else if (id < 10)
			aee_rr_rec_clk(2, clk_readl(mux->base_addr));
		else if (id < 13)
			aee_rr_rec_clk(3, clk_readl(mux->base_addr));
		else if (id < 17)
			aee_rr_rec_clk(4, clk_readl(mux->base_addr));
		else if (id < 20)
			aee_rr_rec_clk(5, clk_readl(mux->base_addr));
		else if (id < 24)
			aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	} while (0);
#endif
}

static void clkmux_disable_op(struct clkmux *mux)
{
#ifdef MUX_LOG
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif

	clk_writel(mux->base_addr + 4, mux->pdn_mask);	/* write set reg */

	if (mux->upd_bit < 32)
		clk_writel(CLK_CFG_UPDATE, BIT(mux->upd_bit));
	else
		clk_writel(CLK_CFG_UPDATE_1, BIT(mux->upd_bit - 32));

#ifdef CONFIG_MTK_RAM_CONSOLE
	do {
		unsigned int id = mux_to_id(mux);

		if (id < 4)
			aee_rr_rec_clk(0, clk_readl(mux->base_addr));
		else if (id < 7)
			aee_rr_rec_clk(1, clk_readl(mux->base_addr));
		else if (id < 10)
			aee_rr_rec_clk(2, clk_readl(mux->base_addr));
		else if (id < 13)
			aee_rr_rec_clk(3, clk_readl(mux->base_addr));
		else if (id < 17)
			aee_rr_rec_clk(4, clk_readl(mux->base_addr));
		else if (id < 20)
			aee_rr_rec_clk(5, clk_readl(mux->base_addr));
		else if (id < 24)
			aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	} while (0);
#endif
}

static struct clkmux_ops clkmux_ops = {
	.sel = clkmux_sel_op,
	.enable = clkmux_enable_op,
	.disable = clkmux_disable_op,
};

static void audio_clkmux_enable_op(struct clkmux *mux)
{
#ifdef MUX_LOG
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif

	clk_clrl(mux->base_addr, mux->pdn_mask);

	if (mux->upd_bit < 32)
		clk_writel(CLK_CFG_UPDATE, BIT(mux->upd_bit));
	else
		clk_writel(CLK_CFG_UPDATE_1, BIT(mux->upd_bit - 32));
};

static struct clkmux_ops audio_clkmux_ops = {
	.sel = clkmux_sel_op,
	.enable = audio_clkmux_enable_op,
	.disable = clkmux_disable_op,
};

static void clkmux_sel_locked(struct clkmux *mux, unsigned int clksrc)
{
	mux->ops->sel(mux, clksrc);
}

#endif /* NEW_MUX */

static void mux_enable_locked(struct clkmux *mux)
{
	mux->cnt++;

#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	if (mux->cnt > 1)
		return;

	if (mux->pll)
		pll_enable_internal(mux->pll, "mux");

#if NEW_MUX
	enable_mux_op(mux->mux_id);
#else /* !NEW_MUX */
	if (mux->ops)
		mux->ops->enable(mux);
#endif /* NEW_MUX */

	if (mux->siblings)
		mux_enable_internal(mux->siblings, "mux_s");

#ifdef MUX_LOG_TOP
	clk_info("[%s]: End. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif
}

static void mux_disable_locked(struct clkmux *mux)
{
#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	BUG_ON(!mux->cnt);

	mux->cnt--;

#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	if (mux->cnt > 0)
		return;

#if NEW_MUX
	disable_mux_op(mux->mux_id);
#else /* !NEW_MUX */
	if (mux->ops)
		mux->ops->disable(mux);
#endif /* NEW_MUX */

	if (mux->siblings)
		mux_disable_internal(mux->siblings, "mux_s");

	if (mux->pll)
		pll_disable_internal(mux->pll, "mux");

#ifdef MUX_LOG_TOP
	clk_info("[%s]: End. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif
}

int clkmux_sel(int id, unsigned int clksrc, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#if NEW_MUX
	struct pll *new_pll;
	struct pll *old_pll;
#endif /* !NEW_MUX */

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);

#if NEW_MUX

	clkmgr_lock(flags);

	new_pll = get_pll_from_mux_sel(mux->mux_id, clksrc);
	old_pll = mux->pll;

	if (old_pll != new_pll) {
		if (new_pll && mux->cnt > 0)
			pll_enable_internal(new_pll, "mux");
	}

	set_mux_sel(mux->mux_id, clksrc);
	mux->pll = new_pll;

	if (old_pll != new_pll) {
		if (old_pll && mux->cnt > 0)
			pll_disable_internal(old_pll, "mux");
	}

	clkmgr_unlock(flags);

#else /* !NEW_MUX */

	BUG_ON(clksrc >= mux->nr_inputs);

	clkmgr_lock(flags);

	if (id == MT_MUX_PWM) {
		if (clksrc != 0) {
			if (muxs[MT_MUX_PWM].pll == NULL) {
				struct pll *pll = &plls[UNIVPLL];

				muxs[MT_MUX_PWM].pll = pll;
				pll_enable_locked(pll);
			}
		} else {
			muxs[MT_MUX_PWM].pll = NULL;
		}
	}

	clkmux_sel_locked(mux, clksrc);
	clkmgr_unlock(flags);

#endif /* !NEW_MUX */

	return 0;
}
EXPORT_SYMBOL(clkmux_sel);

void enable_mux(int id, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);
	BUG_ON(!name);

#ifdef MUX_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	mux_enable_internal(mux, name);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(enable_mux);

void disable_mux(int id, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);
	BUG_ON(!name);

#ifdef MUX_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	mux_disable_internal(mux, name);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(disable_mux);

/************************************************
 **********         cg_grp part        **********
 ************************************************/

static struct cg_grp_ops general_cg_grp_ops;
static struct cg_grp_ops audio_cg_grp_ops;
static struct cg_grp_ops disp0_cg_grp_ops;
static struct cg_grp_ops vdec_cg_grp_ops;
static struct cg_grp_ops venc_cg_grp_ops;

static struct cg_grp grps[NR_GRPS] = {
	[CG_INFRA0] = {
		.name = __stringify(CG_INFRA0),
		.mask = 0x9FE7BFFF,
		.ops = &general_cg_grp_ops,
	},
	[CG_INFRA1] = {
		.name = __stringify(CG_INFRA1),
		.mask = 0xFFF7FD76,
		.ops = &general_cg_grp_ops,
	},
	[CG_DISP0] = {
		.name = __stringify(CG_DISP0),
		.mask = 0xFFFFFFFF,
		.ops = &disp0_cg_grp_ops,
		.sys = &syss[SYS_DIS],
	},
	[CG_DISP1] = {
		.name = __stringify(CG_DISP1),
		.mask = 0x0000FFFF,
		.ops = &general_cg_grp_ops,
		.sys = &syss[SYS_DIS],
	},
	[CG_IMAGE] = {
		.name = __stringify(CG_IMAGE),
		.mask = 0x000003F1,
		.ops = &general_cg_grp_ops,
		.sys = &syss[SYS_ISP],
	},
	[CG_MFG] = {
		.name = __stringify(CG_MFG),
		.mask = 0x0000000F,
		.ops = &general_cg_grp_ops,
		.sys = &syss[SYS_MFG],
	},
	[CG_AUDIO] = {
		.name = __stringify(CG_AUDIO),
		.mask = 0x6F3C0B44,
		.ops = &audio_cg_grp_ops,
		.sys = &syss[SYS_AUD],
	},
	[CG_VDEC0] = {
		.name = __stringify(CG_VDEC0),
		.mask = 0x00000001,
		.ops = &vdec_cg_grp_ops,
		.sys = &syss[SYS_VDE],
	},
	[CG_VDEC1] = {
		.name = __stringify(CG_VDEC1),
		.mask = 0x00000001,
		.ops = &vdec_cg_grp_ops,
		.sys = &syss[SYS_VDE],
	},
	[CG_VENC] = {
		.name = __stringify(CG_VENC),
		.mask = 0x00001111,
		.ops = &venc_cg_grp_ops,
		.sys = &syss[SYS_VEN],
	},
};

static struct cg_grp *id_to_grp(unsigned int id)
{
	return id < NR_GRPS ? grps + id : NULL;
}

static unsigned int general_grp_get_state_op(struct cg_grp *grp)
{
	unsigned int val;
	struct subsys *sys = grp->sys;

	if (sys && !sys->state)
		return 0;

	val = clk_readl(grp->sta_addr);
	val = (~val) & (grp->mask);
	return val;
}

static int general_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);

	return 1;
}

static struct cg_grp_ops general_cg_grp_ops = {
	.get_state = general_grp_get_state_op,
	.dump_regs = general_grp_dump_regs_op,
};

static unsigned int audio_grp_get_state_op(struct cg_grp *grp)
{
	unsigned int val;
	struct subsys *sys = grp->sys;
	struct cg_clk *clk = &clks[MT_CG_INFRA_AUDIO];
	u32 en_mask = BIT(29) | BIT(30);
	u32 cg_mask = grp->mask & ~en_mask;

	if (sys && !sys->state)
		return 0;

	if (!clk->ops->get_state(clk))
		return 0;

	val = clk_readl(grp->sta_addr);
	val = (~val & cg_mask) | (val & en_mask);
	return val;
}

static struct cg_grp_ops audio_cg_grp_ops = {
	.get_state = audio_grp_get_state_op,
	.dump_regs = general_grp_dump_regs_op,
};

static unsigned int disp0_grp_get_state_op(struct cg_grp *grp)
{
	unsigned int val;
	struct subsys *sys = grp->sys;

	if (sys && !sys->state)
		return 0;

	val = clk_readl(grp->dummy_addr);
	val = (~val) & (grp->mask);
	return val;
}

static int disp0_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);
	*(++ptr) = clk_readl(grp->dummy_addr);

	return 2;
}

static struct cg_grp_ops disp0_cg_grp_ops = {
	.get_state = disp0_grp_get_state_op,
	.dump_regs = disp0_grp_dump_regs_op,
};

static unsigned int vdec_grp_get_state_op(struct cg_grp *grp)
{
	unsigned int val = clk_readl(grp->set_addr);

	val &= grp->mask;
	return val;
}

static int vdec_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->set_addr);
	*(++ptr) = clk_readl(grp->clr_addr);

	return 2;
}

static struct cg_grp_ops vdec_cg_grp_ops = {
	.get_state = vdec_grp_get_state_op,
	.dump_regs = vdec_grp_dump_regs_op,
};

static unsigned int venc_grp_get_state_op(struct cg_grp *grp)
{
	unsigned int val = clk_readl(grp->sta_addr);

	val &= grp->mask;
	return val;
}

static int venc_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);

	return 1;
}

static struct cg_grp_ops venc_cg_grp_ops = {
	.get_state = venc_grp_get_state_op,
	.dump_regs = venc_grp_dump_regs_op,
};

/************************************************
 **********         cg_clk part        **********
 ************************************************/

static struct cg_clk_ops general_cg_clk_ops;
static struct cg_clk_ops infra0_cg_clk_ops;
static struct cg_clk_ops infra1_cg_clk_ops;

static struct cg_clk_ops audsys_cg_clk_ops;
static struct cg_clk_ops disp0_cg_clk_ops;
static struct cg_clk_ops vdec_cg_clk_ops;
static struct cg_clk_ops venc_cg_clk_ops;

static struct cg_clk clks[NR_CLKS] = {
	[CG_INFRA0_FROM ... CG_INFRA0_TO] = {
		.cnt = 0,
		.ops = &infra0_cg_clk_ops,
		.grp = &grps[CG_INFRA0],
	},
	[CG_INFRA1_FROM ... CG_INFRA1_TO] = {
		.cnt = 0,
		.ops = &infra1_cg_clk_ops,
		.grp = &grps[CG_INFRA1],
	},
	[CG_DISP0_FROM ... CG_DISP0_TO] = {
		.cnt = 0,
		.ops = &disp0_cg_clk_ops,
		.grp = &grps[CG_DISP0],
	},
	[CG_DISP1_FROM ... CG_DISP1_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_DISP1],
	},
	[CG_IMAGE_FROM ... CG_IMAGE_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_IMAGE],
	},
	[CG_MFG_FROM ... CG_MFG_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_MFG],
	},
	[CG_AUDIO_FROM ... CG_AUDIO_TO] = {
		.cnt = 0,
		.ops = &audsys_cg_clk_ops,
		.grp = &grps[CG_AUDIO],
	},
	[CG_VDEC0_FROM ... CG_VDEC0_TO] = {
		.cnt = 0,
		.ops = &vdec_cg_clk_ops,
		.grp = &grps[CG_VDEC0],
	},
	[CG_VDEC1_FROM ... CG_VDEC1_TO] = {
		.cnt = 0,
		.ops = &vdec_cg_clk_ops,
		.grp = &grps[CG_VDEC1],
	},
	[CG_VENC_FROM ... CG_VENC_TO] = {
		.cnt = 0,
		.ops = &venc_cg_clk_ops,
		.grp = &grps[CG_VENC],
	},
};

static struct cg_clk *id_to_clk(unsigned int id)
{
	return id < NR_CLKS ? clks + id : NULL;
}

static int general_clk_get_state_op(struct cg_clk *clk)
{
	struct subsys *sys = clk->grp->sys;

	if (sys && !sys->state)
		return PWR_DOWN;

	return (clk_readl(clk->grp->sta_addr) & (clk->mask)) ? PWR_DOWN : PWR_ON;
}

static int general_clk_check_validity_op(struct cg_clk *clk)
{
	int valid = 0;

	if (clk->mask & clk->grp->mask)
		valid = 1;

	return valid;
}

static int general_clk_enable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_writel(clk->grp->clr_addr, clk->mask);
	return 0;
}

static int general_clk_disable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_writel(clk->grp->set_addr, clk->mask);
	return 0;
}

static int infra0_clk_enable_op(struct cg_clk *clk)
{
	struct cg_clk *clk_usb_mcu = id_to_clk(MT_CG_INFRA_USB_MCU);
	struct cg_clk *clk_icusb = id_to_clk(MT_CG_INFRA_ICUSB);
	struct cg_clk *clk_usb = id_to_clk(MT_CG_INFRA_USB);
	u32 mask;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	if (clk == clk_icusb || clk == clk_usb)
		return 0;

	if (clk == clk_usb_mcu)
		mask = clk_usb_mcu->mask | clk_icusb->mask | clk_usb->mask;
	else
		mask = clk->mask;

	clk_writel(clk->grp->clr_addr, mask);

	return 0;
}

static int infra0_clk_disable_op(struct cg_clk *clk)
{
	struct cg_clk *clk_usb_mcu = id_to_clk(MT_CG_INFRA_USB_MCU);
	struct cg_clk *clk_icusb = id_to_clk(MT_CG_INFRA_ICUSB);
	struct cg_clk *clk_usb = id_to_clk(MT_CG_INFRA_USB);
	u32 mask;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	if (clk == clk_icusb || clk == clk_usb)
		return 0;

	if (clk == clk_usb_mcu)
		mask = clk_usb_mcu->mask | clk_icusb->mask | clk_usb->mask;
	else
		mask = clk->mask;

	clk_writel(clk->grp->set_addr, mask);

	return 0;
}

static int infra1_clk_enable_op(struct cg_clk *clk)
{
	unsigned int count = 1000, cnt;
	unsigned long apdma_flags;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	if (clk->mask == 0x40000) {
		apdma_spin_lock_irqsave(&apdma_flags);

		if (flag_md32_addr != 0) {
			do {
				if (get_md32_semaphore(SEMAPHORE_APDMA) == 1)
					break;
			} while (count--);

			if (count == 0)
				BUG();

			cnt = readl((void __iomem *)(MD32_DTCM + flag_apmcu_addr));
			cnt++;
			mt_reg_sync_writel(cnt, (MD32_DTCM + flag_apmcu_addr));

			clk_writel(clk->grp->clr_addr, clk->mask);
			release_md32_semaphore(SEMAPHORE_APDMA);
		} else {
			apmcu_clk_init_count++;
			clk_writel(clk->grp->clr_addr, clk->mask);
		}

		apdma_spin_unlock_irqrestore(&apdma_flags);
		return 0;
	}

	clk_writel(clk->grp->clr_addr, clk->mask);
	return 0;
}

static int infra1_clk_disable_op(struct cg_clk *clk)
{
	unsigned int count = 1000, cnt;
	unsigned long apdma_flags;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk))
		clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	if (clk->mask == 0x40000) {
		apdma_spin_lock_irqsave(&apdma_flags);

		if (flag_md32_addr != 0) {
			do {
				if (get_md32_semaphore(SEMAPHORE_APDMA) == 1)
					break;
			} while (count--);

			if (count == 0)
				BUG();

			cnt = readl((void __iomem *)(MD32_DTCM + flag_apmcu_addr));
			cnt--;
			mt_reg_sync_writel(cnt, (MD32_DTCM + flag_apmcu_addr));

			cnt = readl((void __iomem *)(MD32_DTCM + flag_md32_addr));

			if (cnt == 0)
				clk_writel(clk->grp->set_addr, clk->mask);

			release_md32_semaphore(SEMAPHORE_APDMA);
		} else {
			apmcu_clk_init_count--;
			clk_writel(clk->grp->set_addr, clk->mask);
		}

		apdma_spin_unlock_irqrestore(&apdma_flags);
		return 0;
	}

	clk_writel(clk->grp->set_addr, clk->mask);
	return 0;
}

static struct cg_clk_ops general_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};

static struct cg_clk_ops infra0_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = infra0_clk_enable_op,
	.disable = infra0_clk_disable_op,
};

static struct cg_clk_ops infra1_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = infra1_clk_enable_op,
	.disable = infra1_clk_disable_op,
};

static int disp0_clk_get_state_op(struct cg_clk *clk)
{
	struct subsys *sys = clk->grp->sys;

	if (sys && !sys->state)
		return PWR_DOWN;

	return (clk_readl(clk->grp->dummy_addr) & (clk->mask)) ? PWR_DOWN : PWR_ON;
}

static int disp0_clk_enable_op(struct cg_clk *clk)
{
#ifdef DISP_CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_clrl(clk->grp->dummy_addr, clk->mask);

	if (clk->mask & 0x00600003)
		clk_writel(clk->grp->clr_addr, clk->mask);

	return 0;
}

static int disp0_clk_disable_op(struct cg_clk *clk)
{
#ifdef DISP_CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_setl(clk->grp->dummy_addr, clk->mask);

	if (clk->mask & 0x00600003)
		clk_writel(clk->grp->set_addr, clk->mask);

	return 0;
}

static struct cg_clk_ops disp0_cg_clk_ops = {
	.get_state = disp0_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = disp0_clk_enable_op,
	.disable = disp0_clk_disable_op,
};

static int audsys_clk_get_state_op(struct cg_clk *clk)
{
	u32 val;
	struct subsys *sys = clk->grp->sys;

	if (sys && !sys->state)
		return PWR_DOWN;

	val = clk_readl(clk->grp->sta_addr) & clk->mask;

	if (clk->mask & (BIT(29) | BIT(30)))
		return val ? PWR_ON : PWR_DOWN;

	return val ? PWR_DOWN : PWR_ON;
}

static int audsys_clk_enable_op(struct cg_clk *clk)
{
	clk_clrl(clk->grp->sta_addr, clk->mask);
	return 0;
}

static int audsys_clk_disable_op(struct cg_clk *clk)
{
	clk_setl(clk->grp->sta_addr, clk->mask);
	return 0;
}

static struct cg_clk_ops audsys_cg_clk_ops = {
	.get_state = audsys_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = audsys_clk_enable_op,
	.disable = audsys_clk_disable_op,
};

static int vdec_clk_get_state_op(struct cg_clk *clk)
{
	return (clk_readl(clk->grp->set_addr) & (clk->mask)) ? PWR_ON : PWR_DOWN;
}

static struct cg_clk_ops vdec_cg_clk_ops = {
	.get_state = vdec_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};

static int venc_clk_get_state_op(struct cg_clk *clk)
{
	return (clk_readl(clk->grp->sta_addr) & (clk->mask)) ? PWR_ON : PWR_DOWN;
}

static struct cg_clk_ops venc_cg_clk_ops = {
	.get_state = venc_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};

#ifdef PLL_CLK_LINK
static int power_prepare_locked(struct cg_grp *grp)
{
	int err = 0;

	if (grp->sys)
		err = subsys_enable_internal(grp->sys, "clk");

	return err;
}

static int power_finish_locked(struct cg_grp *grp)
{
	int err = 0;

	if (grp->sys)
		err = subsys_disable_internal(grp->sys, 0, "clk");

	return err;
}
#endif

static int clk_enable_locked(struct cg_clk *clk)
{
	struct cg_grp *grp = clk->grp;
#ifdef STATE_CHECK_DEBUG
	unsigned int reg_state;
#endif

#ifdef PLL_CLK_LINK
	int err;
#endif

	clk->cnt++;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk)) {
		clk_info
		    ("[%s]: Start. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
		     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
	}
#endif

	if (clk->cnt > 1)
		return 0;

#ifdef STATE_CHECK_DEBUG
	reg_state = grp->ops->get_state(grp, clk);
#endif

#ifdef PLL_CLK_LINK
	if (clk->pll)
		pll_enable_internal(clk->pll, "clk");

	if (clk->mux)
		mux_enable_internal(clk->mux, "clk");

	err = power_prepare_locked(grp);
	BUG_ON(err);
#endif

	clk->ops->enable(clk);

	clk->state = PWR_ON;
	grp->state |= clk->mask;

	if (clk->siblings)
		clk_enable_internal(clk->siblings, "clk_s");

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk)) {
		clk_info(
			"[%s]: End. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
			__func__, grp->name, grp->state, clk->mask, clk->cnt,
			clk->state);
	}
#endif

	return 0;
}

static void clk_stat_bug(void);
static int clk_disable_locked(struct cg_clk *clk)
{
	struct cg_grp *grp = clk->grp;
#ifdef STATE_CHECK_DEBUG
	unsigned int reg_state;
#endif

#ifdef PLL_CLK_LINK
	int err;
#endif

	if (!clk->cnt) {
		clk_info(
			"[%s]: grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
			__func__, grp->name, grp->state, clk->mask, clk->cnt,
			clk->state);

		clk_stat_bug();
	}

	BUG_ON(!clk->cnt);
	clk->cnt--;

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk)) {
		clk_info(
			"[%s]: Start. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
			__func__, grp->name, grp->state, clk->mask, clk->cnt,
			clk->state);
	}
#endif

	if (clk->cnt > 0)
		return 0;

#ifdef STATE_CHECK_DEBUG
	reg_state = grp->ops->get_state(grp, clk);
#endif

	if (clk->force_on)
		return 0;

	if (clk->siblings)
		clk_disable_internal(clk->siblings, "clk_s");

	clk->ops->disable(clk);

	clk->state = PWR_DOWN;
	grp->state &= ~(clk->mask);

#ifdef PLL_CLK_LINK
	err = power_finish_locked(grp);
	BUG_ON(err);

	if (clk->mux)
		mux_disable_internal(clk->mux, "clk");

	if (clk->pll)
		pll_disable_internal(clk->pll, "clk");
#endif

#ifdef CLK_LOG
	if (!ignore_clk_log(0, clk)) {
		clk_info(
			"[%s]: End. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
			__func__, grp->name, grp->state, clk->mask, clk->cnt,
			clk->state);
	}
#endif

	return 0;
}

static int get_clk_state_locked(struct cg_clk *clk)
{
	if (likely(initialized))
		return clk->state;
	else
		return clk->ops->get_state(clk);
}

int mt_enable_clock(enum cg_clk_id id, char *name)
{
	int err;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));
	BUG_ON(!name);

#ifdef CLK_LOG_TOP
	if (!ignore_clk_log(id, clk))
		clk_info("[%s]: id=%d, names=%s\n", __func__, id, name);
#else
	if (id == MT_CG_DISP0_SMI_COMMON)
		clk_dbg("[%s]: id=%d, names=%s\n", __func__, id, name);
#endif

	clkmgr_lock(flags);
	err = clk_enable_internal(clk, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(mt_enable_clock);

int mt_disable_clock(enum cg_clk_id id, char *name)
{
	int err;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));
	BUG_ON(!name);

#ifdef CLK_LOG_TOP
	if (!ignore_clk_log(id, clk))
		clk_info("[%s]: id=%d, names=%s\n", __func__, id, name);
#else
	if (id == MT_CG_DISP0_SMI_COMMON)
		clk_dbg("[%s]: id=%d, names=%s\n", __func__, id, name);
#endif

	clkmgr_lock(flags);
	err = clk_disable_internal(clk, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(mt_disable_clock);

int enable_clock_ext_locked(int id, char *name)
{
	int err;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	BUG_ON(!clkmgr_locked());
	err = clk_enable_internal(clk, name);

	return err;
}
EXPORT_SYMBOL(enable_clock_ext_locked);

int disable_clock_ext_locked(int id, char *name)
{
	int err;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	BUG_ON(!clkmgr_locked());
	err = clk_disable_internal(clk, name);

	return err;
}
EXPORT_SYMBOL(disable_clock_ext_locked);

int clock_is_on(int id)
{
	int state;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 1;
#endif

	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	state = get_clk_state_locked(clk);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(clock_is_on);

static void clk_set_force_on_locked(struct cg_clk *clk)
{
	clk->force_on = 1;
}

static void clk_clr_force_on_locked(struct cg_clk *clk)
{
	clk->force_on = 0;
}

void clk_set_force_on(int id)
{
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	clk_set_force_on_locked(clk);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(clk_set_force_on);

void clk_clr_force_on(int id)
{
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	clk_clr_force_on_locked(clk);

	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(clk_clr_force_on);

int clk_is_force_on(int id)
{
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	return clk->force_on;
}

int grp_dump_regs(int id, unsigned int *ptr)
{
	struct cg_grp *grp = id_to_grp(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!grp);

	return grp->ops->dump_regs(grp, ptr);
}
EXPORT_SYMBOL(grp_dump_regs);

const char *grp_get_name(int id)
{
	struct cg_grp *grp = id_to_grp(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!grp);

	return grp->name;
}

void print_grp_regs(void)
{
	int i;
	int cnt;
	unsigned int value[3];
	const char *name;

	for (i = 0; i < NR_GRPS; i++) {
		name = grp_get_name(i);
		cnt = grp_dump_regs(i, value);

		if (cnt == 1)
			clk_info("[%02d][%-8s]=[0x%08x]\n", i, name, value[0]);
		else if (cnt == 2)
			clk_info("[%02d][%-8s]=[0x%08x][0x%08x]\n",
				i, name, value[0], value[1]);
		else
			clk_info("[%02d][%-8s]=[0x%08x][0x%08x][0x%08x]\n",
				i, name, value[0], value[1], value[2]);
	}
}

/************************************************
 **********       initialization       **********
 ************************************************/

static void cg_bootup_pdn(void)
{
#if ALL_CLK_ON
	return;
#endif

	/* AUDIO */
	clk_writel(AUDIO_TOP_CON0, 0x0f3c0bc4);

	/* INFRA CG */
	clk_writel(INFRA_PDN_SET0, 0x87a7b980);
	clk_writel(INFRA_PDN_SET1, 0x7cec39f6);

	/* MFG */
	clk_writel(MFG_CG_SET, 0x0000000f);

	/* DISP */
	/*clk_writel(DISP_CG_SET0, 0xfffffffc);*/
	/*clk_writel(DISP_CG_SET1, 0x0000ffff);*/

	/* ISP */
	clk_writel(IMG_CG_SET, 0x00000bf1);

	/* VDE */
	clk_writel(VDEC_CKEN_CLR, 0x00000001);
	clk_writel(LARB_CKEN_CLR, 0x00000001);

	/* VENC */
	clk_clrl(VENC_CG_CON, 0x00001111);
}

static void mt_subsys_init(void)
{
	int i;
	struct subsys *sys;

	syss[SYS_CONN].ctl_addr = SPM_CONN_PWR_CON;
	syss[SYS_DIS].ctl_addr = SPM_DIS_PWR_CON;
	syss[SYS_MFG].ctl_addr = SPM_MFG_PWR_CON;
	syss[SYS_ISP].ctl_addr = SPM_ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = SPM_VDE_PWR_CON;
	syss[SYS_VEN].ctl_addr = SPM_VEN_PWR_CON;
	syss[SYS_AUD].ctl_addr = SPM_AUDIO_PWR_CON;

#ifndef Bring_Up
	for (i = 0; i < NR_SYSS; i++) {
		sys = &syss[i];
		sys->state = sys->ops->get_state(sys);

		if (sys->state == PWR_ON && sys->mux)
			mux_enable_internal(sys->mux, "sys");

#if !ALL_CLK_ON
		if (sys->state != sys->default_sta) {
			clk_info("[%s]%s, change state: (%u->%u)\n", __func__,
				 sys->name, sys->state, sys->default_sta);
			if (sys->default_sta == PWR_DOWN)
				sys_disable_locked(sys, 1);
			else
				sys_enable_locked(sys);
		}
#endif /* ALL_CLK_ON */

#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&sys->head);
#endif
	}
#endif /* Bring_Up */
}

static void mt_plls_init(void)
{
	int i;
	struct pll *pll;

	plls[ARMPLL].base_addr = ARMPLL_CON0;
	plls[ARMPLL].pwr_addr = ARMPLL_PWR_CON0;
	plls[MAINPLL].base_addr = MAINPLL_CON0;
	plls[MAINPLL].pwr_addr = MAINPLL_PWR_CON0;
	plls[MSDCPLL].base_addr = MSDCPLL_CON0;
	plls[MSDCPLL].pwr_addr = MSDCPLL_PWR_CON0;
	plls[UNIVPLL].base_addr = UNIVPLL_CON0;
	plls[UNIVPLL].pwr_addr = UNIVPLL_PWR_CON0;
	plls[MMPLL].base_addr = MMPLL_CON0;
	plls[MMPLL].pwr_addr = MMPLL_PWR_CON0;
	plls[VENCPLL].base_addr = VENCPLL_CON0;
	plls[VENCPLL].pwr_addr = VENCPLL_PWR_CON0;
	plls[TVDPLL].base_addr = TVDPLL_CON0;
	plls[TVDPLL].pwr_addr = TVDPLL_PWR_CON0;
	plls[MPLL].base_addr = MPLL_CON0;
	plls[MPLL].pwr_addr = MPLL_PWR_CON0;
	plls[AUD1PLL].base_addr = AUD1PLL_CON0;
	plls[AUD1PLL].pwr_addr = AUD1PLL_PWR_CON0;
	plls[AUD2PLL].base_addr = AUD2PLL_CON0;
	plls[AUD2PLL].pwr_addr = AUD2PLL_PWR_CON0;
	plls[LVDSPLL].base_addr = LVDSPLL_CON0;
	plls[LVDSPLL].pwr_addr = LVDSPLL_PWR_CON0;

	for (i = 0; i < NR_PLLS; i++) {
		pll = &plls[i];
		pll->state = pll->ops->get_state(pll);

#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&pll->head);
#endif
	}
}

static void mt_muxs_init(void)
{
	int i;
	struct clkmux *mux;

#if NEW_MUX

	enum mux_id_t mux_id;

	for (i = 0; i < NR_MUXS; i++) {
		mux = &muxs[i];
		mux_id = mux->mux_id;
		mux->pll = get_pll_from_mux_sel(mux_id, get_mux_sel(mux_id));
	}

#else /* !NEW_MUX */

	muxs[MT_MUX_AXI].base_addr = CLK_CFG_0;
	muxs[MT_MUX_MEM].base_addr = CLK_CFG_0;
	muxs[MT_MUX_DDRPHY].base_addr = CLK_CFG_0;
	muxs[MT_MUX_MM].base_addr = CLK_CFG_0;

	muxs[MT_MUX_PWM].base_addr = CLK_CFG_1;
	muxs[MT_MUX_VDEC].base_addr = CLK_CFG_1;
	muxs[MT_MUX_MFG].base_addr = CLK_CFG_1;

	muxs[MT_MUX_CAMTG].base_addr = CLK_CFG_2;
	muxs[MT_MUX_UART].base_addr = CLK_CFG_2;
	muxs[MT_MUX_SPI].base_addr = CLK_CFG_2;

	muxs[MT_MUX_MSDC30_0].base_addr = CLK_CFG_3;
	muxs[MT_MUX_MSDC30_1].base_addr = CLK_CFG_3;
	muxs[MT_MUX_MSDC30_2].base_addr = CLK_CFG_3;

	muxs[MT_MUX_MSDC50_3_HCLK].base_addr = CLK_CFG_4;
	muxs[MT_MUX_MSDC50_3].base_addr = CLK_CFG_4;
	muxs[MT_MUX_AUDIO].base_addr = CLK_CFG_4;
	muxs[MT_MUX_AUDINTBUS].base_addr = CLK_CFG_4;

	muxs[MT_MUX_PMICSPI].base_addr = CLK_CFG_5;
	muxs[MT_MUX_SCP].base_addr = CLK_CFG_5;
	muxs[MT_MUX_ATB].base_addr = CLK_CFG_5;
	muxs[MT_MUX_MJC].base_addr = CLK_CFG_5;

	muxs[MT_MUX_DPI0].base_addr = CLK_CFG_6;
	muxs[MT_MUX_SCAM].base_addr = CLK_CFG_6;
	muxs[MT_MUX_AUD1].base_addr = CLK_CFG_6;
	muxs[MT_MUX_AUD2].base_addr = CLK_CFG_6;

	muxs[MT_MUX_DPI1].base_addr = CLK_CFG_7;
	muxs[MT_MUX_UFOENC].base_addr = CLK_CFG_7;
	muxs[MT_MUX_UFODEC].base_addr = CLK_CFG_7;
	muxs[MT_MUX_ETH].base_addr = CLK_CFG_7;

	muxs[MT_MUX_ONFI].base_addr = CLK_CFG_8;
	muxs[MT_MUX_SNFI].base_addr = CLK_CFG_8;
	muxs[MT_MUX_HDMI].base_addr = CLK_CFG_8;
	muxs[MT_MUX_RTC].base_addr = CLK_CFG_8;

#endif /* NEW_MUX */

	for (i = 0; i < NR_MUXS; i++) {
		mux = &muxs[i];
#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&mux->head);
#endif
	}
}

static void mt_clks_init(void)
{
	int i, j;
	struct cg_grp *grp;
	struct cg_clk *clk;

	grps[CG_INFRA0].set_addr = INFRA_PDN_SET0;
	grps[CG_INFRA0].clr_addr = INFRA_PDN_CLR0;
	grps[CG_INFRA0].sta_addr = INFRA_PDN_STA0;
	grps[CG_INFRA1].set_addr = INFRA_PDN_SET1;
	grps[CG_INFRA1].clr_addr = INFRA_PDN_CLR1;
	grps[CG_INFRA1].sta_addr = INFRA_PDN_STA1;
	grps[CG_DISP0].set_addr = DISP_CG_SET0;
	grps[CG_DISP0].clr_addr = DISP_CG_CLR0;
	grps[CG_DISP0].sta_addr = DISP_CG_CON0;
	grps[CG_DISP0].dummy_addr = MMSYS_DUMMY;
	grps[CG_DISP1].set_addr = DISP_CG_SET1;
	grps[CG_DISP1].clr_addr = DISP_CG_CLR1;
	grps[CG_DISP1].sta_addr = DISP_CG_CON1;
	grps[CG_IMAGE].set_addr = IMG_CG_SET;
	grps[CG_IMAGE].clr_addr = IMG_CG_CLR;
	grps[CG_IMAGE].sta_addr = IMG_CG_CON;
	grps[CG_MFG].set_addr = MFG_CG_SET;
	grps[CG_MFG].clr_addr = MFG_CG_CLR;
	grps[CG_MFG].sta_addr = MFG_CG_CON;
	grps[CG_AUDIO].sta_addr = AUDIO_TOP_CON0;
	grps[CG_VDEC0].clr_addr = VDEC_CKEN_SET;
	grps[CG_VDEC0].set_addr = VDEC_CKEN_CLR;
	grps[CG_VDEC1].clr_addr = LARB_CKEN_SET;
	grps[CG_VDEC1].set_addr = LARB_CKEN_CLR;
	grps[CG_VENC].clr_addr = VENC_CG_SET;
	grps[CG_VENC].set_addr = VENC_CG_CLR;
	grps[CG_VENC].sta_addr = VENC_CG_CON;

	for (i = 0; i < NR_GRPS; i++) {
		grp = &grps[i];
		grp->state = grp->ops->get_state(grp);

		for (j = 0; j < 32; j++) {
			if (grp->mask & (1U << j)) {
				clk = &clks[i * 32 + j];
				clk->mask = 1U << j;
#ifndef Bring_Up
				clk->state = clk->ops->get_state(clk);
#endif

#ifdef CONFIG_CLKMGR_STAT
				INIT_LIST_HEAD(&clk->head);
#endif
			}
		}
	}

	clks[MT_CG_INFRA_MSDC_0].mux = &muxs[MT_MUX_MSDC30_0];
	clks[MT_CG_INFRA_MSDC_1].mux = &muxs[MT_MUX_MSDC30_1];
	clks[MT_CG_INFRA_MSDC_2].mux = &muxs[MT_MUX_MSDC30_2];
	clks[MT_CG_INFRA_MSDC_3].mux = &muxs[MT_MUX_MSDC50_3];

	clks[MT_CG_INFRA_UART0].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_INFRA_UART1].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_INFRA_UART2].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_INFRA_UART3].mux = &muxs[MT_MUX_UART];

	clks[MT_CG_INFRA_SPI].mux = &muxs[MT_MUX_SPI];

	clks[MT_CG_INFRA_UFO].mux = &muxs[MT_MUX_UFODEC];

	clks[MT_CG_INFRA_ETH_50M].mux = &muxs[MT_MUX_ETH];
	clks[MT_CG_INFRA_ETH_25M].mux = &muxs[MT_MUX_ETH];

	clks[MT_CG_INFRA_DEBUGSYS].mux = &muxs[MT_MUX_ATB];

	clks[MT_CG_INFRA_ONFI].mux = &muxs[MT_MUX_ONFI];
	clks[MT_CG_INFRA_SNFI].mux = &muxs[MT_MUX_SNFI];

	clks[MT_CG_IMAGE_SEN_TG].mux = &muxs[MT_MUX_CAMTG];

	clks[MT_CG_DISP1_DISP_PWM_26M].mux = &muxs[MT_MUX_PWM];

	clks[MT_CG_DISP1_DPI_PIXEL].mux = &muxs[MT_MUX_DPI0];

	for (i = MT_CG_DISP0_CAM_MDP; i < MT_CG_DISP1_DISP_PWM_MM; i++)
		clks[i].mux = &muxs[MT_MUX_MM];

	clks[MT_CG_DISP1_DISP_PWM_MM].mux = &muxs[MT_MUX_MM];
	clks[MT_CG_DISP1_DSI_ENGINE].mux = &muxs[MT_MUX_MM];
	clks[MT_CG_DISP1_DSI_DIGITAL].mux = &muxs[MT_MUX_MM];
	clks[MT_CG_DISP1_DPI_ENGINE].mux = &muxs[MT_MUX_MM];

	clks[MT_CG_DISP1_DPI1_PIXEL].mux = &muxs[MT_MUX_MM];
	clks[MT_CG_DISP1_DISP_DSC_ENGINE].mux = &muxs[MT_MUX_MM];
	clks[MT_CG_DISP1_DISP_DSC_MEM].mux = &muxs[MT_MUX_MM];

	clks[MT_CG_DISP1_DPI_ENGINE].mux = &muxs[MT_MUX_DPI0];
	clks[MT_CG_DISP1_LVDS_PIXEL].mux = &muxs[MT_MUX_DPI0];

	clks[MT_CG_DISP1_DPI1_ENGINE].mux = &muxs[MT_MUX_DPI1];
	clks[MT_CG_DISP1_HDMI_PIXEL].mux = &muxs[MT_MUX_DPI1];

	clks[MT_CG_DISP1_HDMI_PLLCK].mux = &muxs[MT_MUX_HDMI];

	clks[MT_CG_AUDIO_22M].mux = &muxs[MT_MUX_AUD1];
	clks[MT_CG_AUDIO_24M].mux = &muxs[MT_MUX_AUD2];
	clks[MT_CG_AUDIO_APLL_TUNER].mux = &muxs[MT_MUX_AUD1];
	clks[MT_CG_AUDIO_APLL2_TUNER].mux = &muxs[MT_MUX_AUD2];

	clks[MT_CG_INFRA_USB_MCU].pll = &plls[UNIVPLL];
	clks[MT_CG_INFRA_ICUSB].siblings = &clks[MT_CG_INFRA_USB_MCU];
	clks[MT_CG_INFRA_USB].siblings = &clks[MT_CG_INFRA_USB_MCU];

	/* Don't disable these clock until it's clk_clr_force_on() is called */
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_LARB0]);
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_COMMON]);
}

static void iomap(void);

int mt_clkmgr_init(void)
{
	iomap();
	BUG_ON(initialized);

	cg_bootup_pdn();

	mt_plls_init();
	mt_muxs_init();
	mt_subsys_init();
	mt_clks_init();

	initialized = 1;
	mt_freqhopping_init();

#if !defined(Bring_Up)
	print_grp_regs();
#endif

	return 0;
}

#ifdef CONFIG_MTK_MMC
extern void msdc_clk_status(int *status);
#else
void msdc_clk_status(int *status)
{
	*status = 0;
}
#endif

#define MFG_ASYNC_PWR_STA_MASK	(0x1 << 23)
#define VEN_PWR_STA_MASK	(0x1 << 21)
#define MJC_PWR_STA_MASK	(0x1 << 20)
#define VDE_PWR_STA_MASK	(0x1 << 7)
#define ISP_PWR_STA_MASK	(0x1 << 5)
#define MFG_PWR_STA_MASK	(0x1 << 4)
#define DIS_PWR_STA_MASK	(0x1 << 3)

bool clkmgr_idle_can_enter(unsigned int *condition_mask, unsigned int *block_mask,
			   enum idle_mode mode)
{
	int i, j;
	unsigned int sd_mask = 0;
	unsigned int cg_mask = 0;

	unsigned int sta;

	msdc_clk_status(&sd_mask);
	if (sd_mask) {
		block_mask[CG_INFRA1] |= sd_mask;
		return false;
	}

	for (i = CG_INFRA0; i < NR_GRPS; i++) {
		cg_mask = grps[i].state & condition_mask[i];
		if (cg_mask) {
			for (j = CG_INFRA0; j < NR_GRPS; j++)
				block_mask[j] =
					grps[j].state & condition_mask[j];
			return false;
		}
	}

	sta = clk_readl(SPM_PWR_STATUS);

	if (mode == dpidle) {
		if (sta & (MFG_PWR_STA_MASK | ISP_PWR_STA_MASK |
				VDE_PWR_STA_MASK | MJC_PWR_STA_MASK |
				VEN_PWR_STA_MASK | DIS_PWR_STA_MASK))
			return false;
	} else if (mode == soidle) {
		if (sta & (MFG_PWR_STA_MASK | ISP_PWR_STA_MASK |
				VDE_PWR_STA_MASK | MJC_PWR_STA_MASK |
				VEN_PWR_STA_MASK))
			return false;
	}

	return true;
}

#define pdn_mfg_13m_ck (0x1<<0)

void clk_misc_cfg_ops(bool flag)
{
	if (flag == true)
		clk_clrl(CLK_MISC_CFG_0, pdn_mfg_13m_ck);
	else
		clk_setl(CLK_MISC_CFG_0, pdn_mfg_13m_ck);
}
EXPORT_SYMBOL(clk_misc_cfg_ops);

/************************************************
 **********       function debug       **********
 ************************************************/

static int pll_test_read(struct seq_file *m, void *v)
{
	int i, j;
	int cnt;
	unsigned int value[3];
	const char *name;

	seq_puts(m, "********** pll register dump **********\n");
	for (i = 0; i < NR_PLLS; i++) {
		name = pll_get_name(i);
		cnt = pll_dump_regs(i, value);

		for (j = 0; j < cnt; j++)
			seq_printf(m, "[%2d][%8s reg%d]=[0x%08x]\n", i, name, j, value[j]);
	}

	seq_puts(m, "\n********** pll_test help **********\n");
	seq_puts(m, "enable  pll: echo enable  id [mod_name] > /proc/clkmgr/pll_test\n");
	seq_puts(m, "disable pll: echo disable id [mod_name] > /proc/clkmgr/pll_test\n");

	return 0;
}

static ssize_t pll_test_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	char cmd[10];
	char mod_name[10];
	int id;
	int err = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s %d %s", cmd, &id, mod_name) == 3) {
		if (!strcmp(cmd, "enable"))
			err = enable_pll(id, mod_name);
		else if (!strcmp(cmd, "disable"))
			err = disable_pll(id, mod_name);
	} else if (sscanf(desc, "%s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_pll(id, "pll_test");
		else if (!strcmp(cmd, "disable"))
			err = disable_pll(id, "pll_test");
	}

	clk_info("[%s]%s pll %d: result is %d\n", __func__, cmd, id, err);

	return count;
}

static int pll_fsel_read(struct seq_file *m, void *v)
{
	int i;
	int cnt;
	unsigned int value[3];
	const char *name;

	for (i = 0; i < NR_PLLS; i++) {
		name = pll_get_name(i);
		if (pll_is_on(i)) {
			cnt = pll_dump_regs(i, value);
			if (cnt >= 2)
				seq_printf(m,
					"[%2d][%8s]=[0x%08x%08x]\n",
					i, name, value[0], value[1]);
			else
				seq_printf(m, "[%2d][%8s]=[0x%08x]\n",
					i, name, value[0]);
		} else {
			seq_printf(m, "[%2d][%8s]=[-1]\n", i, name);
		}
	}

	seq_puts(m, "\n********** pll_fsel help **********\n");
	seq_puts(m, "adjust pll frequency:  echo id freq > /proc/clkmgr/pll_fsel\n");

	return 0;
}

static ssize_t pll_fsel_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int id;
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %x", &id, &value) == 2)
		pll_fsel(id, value);

	return count;
}

#ifdef CONFIG_CLKMGR_STAT
static int pll_stat_read(struct seq_file *m, void *v)
{
	struct pll *pll;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "\n********** pll stat dump **********\n");

	for (i = 0; i < NR_PLLS; i++) {
		pll = id_to_pll(i);
		seq_printf(m, "[%2d][%8s] state=%u, cnt=%u",
			i, pll->name, pll->state, pll->cnt);
		list_for_each(pos, &pll->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)",
				node->name, node->cnt_on, node->cnt_off);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** pll_dump help **********\n");

	return 0;
}
#endif

static int subsys_test_read(struct seq_file *m, void *v)
{
	int i;
	int state;
	unsigned int value = 0, sta, sta_s;
	const char *name;

	sta = clk_readl(SPM_PWR_STATUS);
	sta_s = clk_readl(SPM_PWR_STATUS_2ND);

	seq_puts(m, "********** subsys register dump **********\n");

	for (i = 0; i < NR_SYSS; i++) {
		name = subsys_get_name(i);
		state = subsys_is_on(i);
		subsys_dump_regs(i, &value);
		seq_printf(m, "[%d][%-8s]=[0x%08x], state(%u)\n",
			i, name, value, state);
	}

	seq_printf(m, "SPM_PWR_STATUS=0x%08x, SPM_PWR_STATUS_2ND=0x%08x\n",
		sta, sta_s);

	seq_puts(m, "\n********** subsys_test help **********\n");
	seq_puts(m,
		"enable subsys:  echo enable id > /proc/clkmgr/subsys_test\n");
	seq_puts(m,
		"disable subsys: echo disable id [force_off] > /proc/clkmgr/subsys_test\n");

	return 0;
}

static ssize_t subsys_test_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	char cmd[10];
	int id;
	int force_off;
	int err = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s %d %d", cmd, &id, &force_off) == 3) {
		if (!strcmp(cmd, "disable"))
			err = disable_subsys_force(id, "test");
	} else if (sscanf(desc, "%s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_subsys(id, "test");
		else if (!strcmp(cmd, "disable"))
			err = disable_subsys(id, "test");
	}

	clk_info("[%s]%s subsys %d: result is %d\n", __func__, cmd, id, err);

	return count;
}

#ifdef CONFIG_CLKMGR_STAT
static int subsys_stat_read(struct seq_file *m, void *v)
{
	struct subsys *sys;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "\n********** subsys stat dump **********\n");

	for (i = 0; i < NR_SYSS; i++) {
		sys = id_to_sys(i);
		seq_printf(m, "[%d][%-8s]state=%u", i, sys->name, sys->state);
		list_for_each(pos, &sys->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on,
				node->cnt_off);
		}

		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** subsys_dump help **********\n");

	return 0;
}
#endif

static int mux_test_read(struct seq_file *m, void *v)
{
	seq_puts(m, "********** mux register dump *********\n");
	seq_printf(m, "[CLK_CFG_0]=0x%08x\n", clk_readl(CLK_CFG_0));
	seq_printf(m, "[CLK_CFG_1]=0x%08x\n", clk_readl(CLK_CFG_1));
	seq_printf(m, "[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
	seq_printf(m, "[CLK_CFG_3]=0x%08x\n", clk_readl(CLK_CFG_3));
	seq_printf(m, "[CLK_CFG_4]=0x%08x\n", clk_readl(CLK_CFG_4));
	seq_printf(m, "[CLK_CFG_5]=0x%08x\n", clk_readl(CLK_CFG_5));
	seq_printf(m, "[CLK_CFG_6]=0x%08x\n", clk_readl(CLK_CFG_6));
	seq_printf(m, "[CLK_CFG_7]=0x%08x\n", clk_readl(CLK_CFG_7));
	seq_printf(m, "[CLK_CFG_8]=0x%08x\n", clk_readl(CLK_CFG_8));

	seq_puts(m, "\n********** mux_test help *********\n");
	seq_puts(m, "enable  mux: echo enable  id [mod_name] > /proc/clkmgr/mux_test\n");
	seq_puts(m, "disable mux: echo disable id [mod_name] > /proc/clkmgr/mux_test\n");
	seq_puts(m, "mux src sel: echo sel     id val        > /proc/clkmgr/mux_test\n");

	return 0;
}

static ssize_t mux_test_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	char cmd[10];
	char mod_name[10];
	int id, val;
	int err;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s %d %d", cmd, &id, &val) == 3) {
		if (!strcmp(cmd, "sel"))
			err = clkmux_sel(id, val, "mux_test");
	} else if (sscanf(desc, "%s %d %s", cmd, &id, mod_name) == 3) {
		if (!strcmp(cmd, "enable"))
			enable_mux(id, mod_name);
		else if (!strcmp(cmd, "disable"))
			disable_mux(id, mod_name);
	} else if (sscanf(desc, "%s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			enable_mux(id, "mux_test");
		else if (!strcmp(cmd, "disable"))
			disable_mux(id, "mux_test");
	}

	return count;
}

#ifdef CONFIG_CLKMGR_STAT
static int mux_stat_read(struct seq_file *m, void *v)
{
	struct clkmux *mux;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "********** mux stat dump **********\n");

	for (i = 0; i < NR_MUXS; i++) {
#if NEW_MUX
		enum mux_id_t mux_id;
		struct mux_t *imux;
		struct mux_clksrc_t *mux_clksrc;
		u32 sel;

		mux = id_to_mux(i);
		mux_id = mux->mux_id;
		imux = mux_from_id(mux_id);
		sel = get_mux_sel(mux_id);
		mux_clksrc = &g_mux_clksrc[imux->clksrc[sel]];

		seq_printf(m,
			"[%02d][%-17s] state=%d, cnt=%2u, sel [%2u: %-12s (%7s)] %6u KHz",
			i, imux->name, mux_is_on(mux_id), mux->cnt, sel,
			mux_clksrc->name,
			mux_clksrc->pll ? mux_clksrc->pll->name : "",
			measure_ckgen_freq(imux->ckgen_clk));
#else /* !NEW_MUX */
		mux = id_to_mux(i);
		seq_printf(m, "[%02d][%-17s] cnt=%u", i, mux->name, mux->cnt);
#endif /* NEW_MUX */

		list_for_each(pos, &mux->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on,
				node->cnt_off);
		}

		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** mux_dump help **********\n");

	return 0;
}
#endif

static int clk_test_read(struct seq_file *m, void *v)
{
	int i;
	int cnt;
	unsigned int value[3];
	const char *name;

	seq_puts(m, "********** clk register dump **********\n");

	for (i = 0; i < NR_GRPS; i++) {
		name = grp_get_name(i);
		cnt = grp_dump_regs(i, value);

		if (cnt == 1)
			seq_printf(m, "[%02d][%-9s]=[0x%08x]\n",
				i, name, value[0]);
		else if (cnt == 2)
			seq_printf(m, "[%02d][%-9s]=[0x%08x][0x%08x]\n",
				i, name, value[0], value[1]);
		else
			seq_printf(m, "[%02d][%-9s]=[0x%08x][0x%08x][0x%08x]\n",
				i, name, value[0], value[1], value[2]);
	}

	seq_puts(m, "\n********** clk_test help **********\n");
	seq_puts(m, "enable  clk: echo enable  id [mod_name] > /proc/clkmgr/clk_test\n");
	seq_puts(m, "disable clk: echo disable id [mod_name] > /proc/clkmgr/clk_test\n");
	seq_puts(m, "read state:  echo id > /proc/clkmgr/clk_test\n");

	return 0;
}

static ssize_t clk_test_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	char cmd[10];
	char mod_name[10];
	int id;
	int err;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s %d %s", cmd, &id, mod_name) == 3) {
		if (!strcmp(cmd, "enable"))
			err = enable_clock(id, mod_name);
		else if (!strcmp(cmd, "disable"))
			err = disable_clock(id, mod_name);
	} else if (sscanf(desc, "%s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_clock(id, "clk_test");
		else if (!strcmp(cmd, "disable"))
			err = disable_clock(id, "clk_test");
	} else if (sscanf(desc, "%d", &id) == 1) {
		clk_info("clock %d is %s\n",
			id, clock_is_on(id) ? "on" : "off");
	}

	return count;
}

#ifdef CONFIG_CLKMGR_STAT
static int clk_stat_read(struct seq_file *m, void *v)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, grp, offset;
	int skip;

	seq_puts(m, "\n********** clk stat dump **********\n");

	for (i = 0; i < NR_CLKS; i++) {
		grp = i / 32;
		offset = i % 32;

		if (offset == 0)
			seq_printf(m, "\n*****[%02d][%-9s]*****\n",
				grp, grp_get_name(grp));

		clk = id_to_clk(i);
		if (!clk || !clk->grp || !clk->ops->check_validity(clk))
			continue;

		skip = (clk->cnt == 0) && (clk->state == 0) &&
			list_empty(&clk->head);
		if (skip)
			continue;

		seq_printf(m, "[%02d] state=%u, cnt=%u",
			offset, clk->state, clk->cnt);

		list_for_each(pos, &clk->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)",
				node->name, node->cnt_on, node->cnt_off);
		}

		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** clk_dump help **********\n");

	return 0;
}

void clk_stat_check(int id)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, j, grp, offset;
	int skip;

	if (id == SYS_DIS) {
		for (i = CG_DISP0_FROM; i <= CG_DISP0_TO; i++) {
			grp = i / 32;
			offset = i % 32;
			clk = id_to_clk(i);

			if (!clk || !clk->grp || !clk->ops->check_validity(clk))
				continue;

			skip = (clk->cnt == 0) && (clk->state == 0) &&
				list_empty(&clk->head);
			if (skip)
				continue;

			clk_info(" [%02d]state=%u, cnt=%u",
				offset, clk->state, clk->cnt);

			j = 0;
			list_for_each(pos, &clk->head) {
				node = list_entry(pos, struct stat_node, link);
				clk_info(" (%s,%u,%u)", node->name,
					node->cnt_on, node->cnt_off);
				if (++j % 3 == 0)
					clk_info("\n \t\t\t\t ");
			}
			clk_info("\n");
		}
	}
}
EXPORT_SYMBOL(clk_stat_check);

static void clk_stat_bug(void)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, j, grp, offset;
	int skip;

	for (i = 0; i < NR_CLKS; i++) {
		grp = i / 32;
		offset = i % 32;

		if (offset == 0)
			clk_info("\n*****[%02d][%-8s]*****\n",
				grp, grp_get_name(grp));

		clk = id_to_clk(i);

		if (!clk || !clk->grp || !clk->ops->check_validity(clk))
			continue;

		skip = (clk->cnt == 0) && (clk->state == 0) &&
			list_empty(&clk->head);
		if (skip)
			continue;

		clk_info(" [%02d]state=%u, cnt=%u",
			offset, clk->state, clk->cnt);

		j = 0;
		list_for_each(pos, &clk->head) {
			node = list_entry(pos, struct stat_node, link);
			clk_info(" (%s,%u,%u)",
				node->name, node->cnt_on, node->cnt_off);
			if (++j % 3 == 0)
				clk_info("\n \t\t\t\t ");
		}
		clk_info("\n");
	}
}
#endif /* CONFIG_CLKMGR_STAT */

void slp_check_pm_mtcmos_pll(void)
{
	int i;

	slp_chk_mtcmos_pll_stat = 1;

	clk_info("[%s]\n", __func__);
	for (i = 2; i < NR_PLLS; i++) {
		if (i == MPLL)
			continue;

		if (!pll_is_on(i))
			continue;

		slp_chk_mtcmos_pll_stat = -1;
		clk_warn("%s: on\n", plls[i].name);
		clk_warn("suspend warning: %s is on!!!\n", plls[i].name);
		clk_warn("warning! warning! it may cause resume fail\n");
	}

	for (i = 0; i < NR_SYSS; i++) {
		if (!subsys_is_on(i))
			continue;

		clk_info("%s: on\n", syss[i].name);
		slp_chk_mtcmos_pll_stat = -1;
		if (i <= SYS_CONN)
			continue;

		clk_warn("suspend warning: %s is on!!!\n", syss[i].name);
		clk_warn("warning! warning! it may cause resume fail\n");
#ifdef CONFIG_CLKMGR_STAT
		clk_stat_bug();
#endif
	}
}
EXPORT_SYMBOL(slp_check_pm_mtcmos_pll);

static int clk_force_on_read(struct seq_file *m, void *v)
{
	int i;
	struct cg_clk *clk;

	seq_puts(m, "********** clk force on info dump **********\n");
	for (i = 0; i < NR_CLKS; i++) {
		clk = &clks[i];
		if (clk->force_on) {
			seq_printf(m, "clock %d (0x%08x @ %s) is force on\n", i,
				   clk->mask, clk->grp->name);
		}
	}

	seq_puts(m, "\n********** clk_force_on help **********\n");
	seq_puts(m,
		"set clk force on: echo set id > /proc/clkmgr/clk_force_on\n");
	seq_puts(m,
		"clr clk force on: echo clr id > /proc/clkmgr/clk_force_on\n");

	return 0;
}

static ssize_t clk_force_on_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	char cmd[10];
	int id;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);

	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "set"))
			clk_set_force_on(id);
		else if (!strcmp(cmd, "clr"))
			clk_clr_force_on(id);
	}

	return count;
}

static int slp_chk_mtcmos_pll_stat_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", slp_chk_mtcmos_pll_stat);
	return 0;
}

/* for pll_test */
static int proc_pll_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_test_read, NULL);
}

static const struct file_operations pll_test_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_pll_test_open,
	.read = seq_read,
	.write = pll_test_write,
};

/* for pll_fsel */
static int proc_pll_fsel_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_fsel_read, NULL);
}

static const struct file_operations pll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_pll_fsel_open,
	.read = seq_read,
	.write = pll_fsel_write,
};

#ifdef CONFIG_CLKMGR_STAT

static int proc_pll_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_stat_read, NULL);
}

static const struct file_operations pll_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_pll_stat_open,
	.read = seq_read,
};

#endif /* CONFIG_CLKMGR_STAT */

static int proc_subsys_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_test_read, NULL);
}

static const struct file_operations subsys_test_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_subsys_test_open,
	.read = seq_read,
	.write = subsys_test_write
};

#ifdef CONFIG_CLKMGR_STAT

static int proc_subsys_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_stat_read, NULL);
}

static const struct file_operations subsys_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_subsys_stat_open,
	.read = seq_read,
};

#endif /* CONFIG_CLKMGR_STAT */

static int proc_mux_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, mux_test_read, NULL);
}

static const struct file_operations mux_test_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_mux_test_open,
	.read = seq_read,
	.write = mux_test_write,
};

#ifdef CONFIG_CLKMGR_STAT

static int proc_mux_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, mux_stat_read, NULL);
}

static const struct file_operations mux_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_mux_stat_open,
	.read = seq_read,
};

#endif /* CONFIG_CLKMGR_STAT */

static int proc_clk_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_test_read, NULL);
}

static const struct file_operations clk_test_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_clk_test_open,
	.read = seq_read,
	.write = clk_test_write,
};

#ifdef CONFIG_CLKMGR_STAT

static int proc_clk_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_stat_read, NULL);
}

static const struct file_operations clk_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_clk_stat_open,
	.read = seq_read,
};

#endif /* CONFIG_CLKMGR_STAT */

static int proc_clk_force_on_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_force_on_read, NULL);
}

static const struct file_operations clk_force_on_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_clk_force_on_open,
	.read = seq_read,
	.write = clk_force_on_write,
};

static int proc_slp_chk_mtcmos_pll_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, slp_chk_mtcmos_pll_stat_read, NULL);
}

static const struct file_operations slp_chk_mtcmos_pll_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_slp_chk_mtcmos_pll_stat_open,
	.read = seq_read,
};

#if FMETER

static int fmeter_show(struct seq_file *s, void *v)
{
	enum ABIST_CLK abist_clk[] = {
		FM_AD_ARMCA7PLL_754M_CORE_CK,
		FM_AD_OSC_CK,
		FM_AD_MAIN_H546M_CK,
		FM_AD_MAIN_H364M_CK,
		FM_AD_MAIN_H218P4M_CK,
		FM_AD_MAIN_H156M_CK,
		FM_AD_UNIV_178P3M_CK,
		FM_AD_UNIV_48M_CK,
		FM_AD_UNIV_624M_CK,
		FM_AD_UNIV_416M_CK,
		FM_AD_UNIV_249P6M_CK,
		FM_AD_APLL1_180P6336M_CK,
		FM_AD_APLL2_196P608M_CK,
		FM_AD_LTEPLL_FS26M_CK,
		FM_RTC32K_CK_I,
		FM_AD_MMPLL_700M_CK,
		FM_AD_VENCPLL_410M_CK,
		FM_AD_TVDPLL_594M_CK,
		FM_AD_MPLL_208M_CK,
		FM_AD_MSDCPLL_806M_CK,
		FM_AD_USB_48M_CK,
		FM_MIPI_DSI_TST_CK,
		FM_AD_PLLGP_TST_CK,
		FM_AD_LVDSTX_MONCLK,
		FM_AD_DSI0_LNTC_DSICLK,
		FM_AD_DSI0_MPPLL_TST_CK,
		FM_AD_CSI0_DELAY_TSTCLK,
		FM_AD_CSI1_DELAY_TSTCLK,
		FM_AD_HDMITX_MON_CK,
		FM_AD_ARMPLL_1508M_CK,
		FM_AD_MAINPLL_1092M_CK,
		FM_AD_MAINPLL_PRO_CK,
		FM_AD_UNIVPLL_1248M_CK,
		FM_AD_LVDSPLL_150M_CK,
		FM_AD_LVDSPLL_ETH_CK,
		FM_AD_SSUSB_48M_CK,
		FM_AD_MIPI_26M_CK,
		FM_AD_MEM_26M_CK,
		FM_AD_MEMPLL_MONCLK,
		FM_AD_MEMPLL2_MONCLK,
		FM_AD_MEMPLL3_MONCLK,
		FM_AD_MEMPLL4_MONCLK,
		FM_AD_MEMPLL_REFCLK_BUF,
		FM_AD_MEMPLL_FBCLK_BUF,
		FM_AD_MEMPLL2_REFCLK_BUF,
		FM_AD_MEMPLL2_FBCLK_BUF,
		FM_AD_MEMPLL3_REFCLK_BUF,
		FM_AD_MEMPLL3_FBCLK_BUF,
		FM_AD_MEMPLL4_REFCLK_BUF,
		FM_AD_MEMPLL4_FBCLK_BUF,
		FM_AD_USB20_MONCLK,
		FM_AD_USB20_MONCLK_1P,
		FM_AD_MONREF_CK,
		FM_AD_MONFBK_CK,
	};

	enum CKGEN_CLK ckgen_clk[] = {
		FM_HD_FAXI_CK,
		FM_HF_FDDRPHYCFG_CK,
		FM_F_FPWM_CK,
		FM_HF_FVDEC_CK,
		FM_HF_FMM_CK,
		FM_HF_FCAMTG_CK,
		FM_F_FUART_CK,
		FM_HF_FSPI_CK,
		FM_HF_FMSDC_30_0_CK,
		FM_HF_FMSDC_30_1_CK,
		FM_HF_FMSDC_30_2_CK,
		FM_HF_FMSDC_50_3_CK,
		FM_HF_FMSDC_50_3_HCLK_CK,
		FM_HF_FAUDIO_CK,
		FM_HF_FAUD_INTBUS_CK,
		FM_HF_FPMICSPI_CK,
		FM_HF_FSCP_CK,
		FM_HF_FATB_CK,
		FM_HF_FSNFI_CK,
		FM_HF_FDPI0_CK,
		FM_HF_FAUD_1_CK_PRE,
		FM_HF_FAUD_2_CK_PRE,
		FM_HF_FSCAM_CK,
		FM_HF_FMFG_CK,
		FM_ECO_AO12_MEM_CLKMUX_CK,
		FM_ECO_A012_MEM_DCM_CK,
		FM_HF_FDPI1_CK,
		FM_HF_FUFOENC_CK,
		FM_HF_FUFODEC_CK,
		FM_HF_FETH_CK,
		FM_HF_FONFI_CK,
	};

	size_t i;
	struct bak_regs1 bak_regs;

	before_fmeter(&bak_regs);

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		measure_abist_clock(abist_clk[i], s);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		measure_ckgen_clock(ckgen_clk[i], s);

	after_fmeter(&bak_regs);

	return 0;
}

static int fmeter_open(struct inode *inode, struct file *file)
{
	return single_open(file, fmeter_show, NULL);
}

static const struct file_operations fmeter_fops = {
	.owner      = THIS_MODULE,
	.open       = fmeter_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release,
};

#endif /* FMETER */

void mt_clkmgr_debug_init(void)
{
	struct proc_dir_entry *entry;
	struct proc_dir_entry *clkmgr_dir;

	clkmgr_dir = proc_mkdir("clkmgr", NULL);
	if (!clkmgr_dir) {
		clk_err("[%s]: fail to mkdir /proc/clkmgr\n", __func__);
		return;
	}

	entry = proc_create("pll_test", S_IRUGO | S_IWUSR, clkmgr_dir, &pll_test_proc_fops);

	entry = proc_create("pll_fsel", S_IRUGO | S_IWUSR, clkmgr_dir, &pll_fsel_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("pll_stat", S_IRUGO, clkmgr_dir, &pll_stat_proc_fops);
#endif

	entry = proc_create("subsys_test", S_IRUGO | S_IWUSR, clkmgr_dir, &subsys_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("subsys_stat", S_IRUGO, clkmgr_dir, &subsys_stat_proc_fops);
#endif

	entry = proc_create("mux_test", S_IRUGO, clkmgr_dir, &mux_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("mux_stat", S_IRUGO, clkmgr_dir, &mux_stat_proc_fops);
#endif

	entry = proc_create("clk_test", S_IRUGO | S_IWUSR, clkmgr_dir, &clk_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("clk_stat", S_IRUGO, clkmgr_dir, &clk_stat_proc_fops);
#endif

#if FMETER
	entry = proc_create("fmeter", 00640, clkmgr_dir, &fmeter_fops);
#endif /* FMETER */

	entry = proc_create("clk_force_on", S_IRUGO | S_IWUSR, clkmgr_dir,
			&clk_force_on_proc_fops);

	entry = proc_create("slp_chk_mtcmos_pll_stat", S_IRUGO, clkmgr_dir,
			&slp_chk_mtcmos_pll_stat_proc_fops);
}

/***********************************
 * for early suspend
 ************************************/

#ifdef CONFIG_HAS_EARLYSUSPEND

static void clkmgr_early_suspend(struct early_suspend *h)
{
}

static void clkmgr_late_resume(struct early_suspend *h)
{
}

static struct early_suspend mt_clkmgr_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = clkmgr_early_suspend,
	.resume = clkmgr_late_resume,
};
#endif /* #ifdef CONFIG_HAS_EARLYSUSPEND */

struct platform_device clkmgr_device = {
	.name = "CLK",
	.id = -1,
	.dev = {},
};

int clk_pm_restore_noirq(struct device *device)
{
	struct subsys *sys;

	sys = &syss[SYS_DIS];
	sys->state = sys->ops->get_state(sys);

	muxs[MT_MUX_MM].cnt = 1;
	plls[MAINPLL].cnt = 1;
	es_flag = 0;

	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_LARB0]);
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_COMMON]);

	clk_info("clk_pm_restore_noirq\n");

	return 0;
}

#ifdef CONFIG_PM
const struct dev_pm_ops clkmgr_pm_ops = {
	.restore_noirq = clk_pm_restore_noirq,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id mt_clkmgr_of_match[] = {
	{.compatible = "mediatek,APMIXED",},
	{},
};
#endif

static struct platform_driver clkmgr_driver = {
	.driver = {
		.name = "CLK",
#ifdef CONFIG_PM
		.pm = &clkmgr_pm_ops,
#endif
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_clkmgr_of_match,
#endif
	},
};

#ifdef CONFIG_OF
static void __iomem *reg_from_cmp(const char *cmp, int idx)
{
	void __iomem *reg;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);
	if (!node) {
		pr_warn("%s find node failed\n", cmp);
		return NULL;
	}

	reg = of_iomap(node, idx);
	if (!reg)
		pr_warn("%s base failed\n", cmp);

	return reg;
}

static void iomap(void)
{
	clk_apmixed_base = reg_from_cmp("mediatek,APMIXED", 0);
	clk_cksys_base = reg_from_cmp("mediatek,TOPCKGEN", 0);
	clk_infracfg_ao_base = reg_from_cmp("mediatek,INFRACFG_AO", 0);
	clk_audio_base = reg_from_cmp("mediatek,AUDIO", 0);
	clk_mfgcfg_base = reg_from_cmp("mediatek,G3D_CONFIG", 0);
	clk_mmsys_config_base = reg_from_cmp("mediatek,MMSYS_CONFIG", 0);
	clk_imgsys_base = reg_from_cmp("mediatek,IMGSYS", 0);
	clk_vdec_gcon_base = reg_from_cmp("mediatek,VDEC_GCON", 0);
	clk_venc_gcon_base = reg_from_cmp("mediatek,VENC_GCON", 0);

	/* imgsys workaround */
	if (!clk_imgsys_base)
		clk_imgsys_base = reg_from_cmp("mediatek,ISPSYS", 1);

	if (!clk_imgsys_base)
		clk_imgsys_base = reg_from_cmp("mediatek,IMGSYS_CONFIG", 0);

	WARN_ON(!clk_apmixed_base);
	WARN_ON(!clk_cksys_base);
	WARN_ON(!clk_infracfg_ao_base);
	WARN_ON(!clk_audio_base);
	WARN_ON(!clk_mfgcfg_base);
	WARN_ON(!clk_mmsys_config_base);
	WARN_ON(!clk_imgsys_base);
	WARN_ON(!clk_vdec_gcon_base);
	WARN_ON(!clk_venc_gcon_base);
}
#endif

static int mt_clkmgr_debug_module_init(void)
{
	int ret;

	mt_clkmgr_debug_init();

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&mt_clkmgr_early_suspend_handler);
#endif

	ret = platform_device_register(&clkmgr_device);
	if (ret) {
		clk_info("clkmgr_device register fail(%d)\n", ret);
		return ret;
	}

	ret = platform_driver_register(&clkmgr_driver);
	if (ret) {
		clk_info("clkmgr_driver register fail(%d)\n", ret);
		return ret;
	}

	return 0;
}

static int __init mt_clkmgr_late_init(void)
{
	int toggle_plls[] = {
		VENCPLL,
	};

	int toggle_muxes[] = {
		MT_MUX_MJC,
		MT_MUX_SCAM,
		MT_MUX_AUD1,
		MT_MUX_AUD2,
	};

	enum cg_clk_id toggle_clks[] = {
		MT_CG_DISP1_DPI_PIXEL,
		MT_CG_DISP1_DPI1_ENGINE,
		MT_CG_DISP1_HDMI_PLLCK,
		MT_CG_VDEC0_VDEC,
		MT_CG_VENC_LARB,
		MT_CG_IMAGE_LARB2_SMI,
		MT_CG_G3D_FF,
		MT_CG_G3D_DW,
		MT_CG_G3D_TX,
		MT_CG_G3D_MX,
		MT_CG_INFRA_SPI,
		MT_CG_INFRA_MSDC_0,
		MT_CG_INFRA_MSDC_2,
		MT_CG_INFRA_MSDC_3,
		MT_CG_INFRA_UFO,
		MT_CG_INFRA_ETH_50M,
		MT_CG_INFRA_ETH_25M,
		MT_CG_INFRA_ONFI,
		MT_CG_INFRA_SNFI,
	};

	size_t i;

#if ALL_CLK_ON || defined(Bring_Up)
	return 0;
#endif

#if MSDC_WORKAROUND
	/* MSDC clock workaround */
	mt_enable_clock(MT_CG_INFRA_MSDC_0, "clkmgr_msdc");
	mt_enable_clock(MT_CG_INFRA_MSDC_3, "clkmgr_msdc");
#endif /* MSDC_WORKAROUND */

	for (i = 0; i < ARRAY_SIZE(toggle_plls); i++)
		enable_pll(toggle_plls[i], "clkmgr");

	for (i = 0; i < ARRAY_SIZE(toggle_muxes); i++)
		enable_mux(toggle_muxes[i], "clkmgr");

	for (i = 0; i < ARRAY_SIZE(toggle_clks); i++)
		mt_enable_clock(toggle_clks[i], "clkmgr");

	for (i = ARRAY_SIZE(toggle_clks); i > 0; i--)
		mt_disable_clock(toggle_clks[i - 1], "clkmgr");

	for (i = ARRAY_SIZE(toggle_muxes); i > 0; i--)
		disable_mux(toggle_muxes[i - 1], "clkmgr");

	for (i = ARRAY_SIZE(toggle_plls); i > 0; i--)
		disable_pll(toggle_plls[i - 1], "clkmgr");

	return 0;
}

module_init(mt_clkmgr_debug_module_init);
late_initcall(mt_clkmgr_late_init);
