#ifndef DDR_INFO_H
#define DDR_INFO_H

#define DDR_INFO_TAG                "[DDR INFO]"
#define ddr_info_warn(fmt, arg...)  pr_err(DDR_INFO_TAG fmt, ##arg)
#define ddr_info_dbg(fmt, arg...)   pr_err(DDR_INFO_TAG fmt, ##arg)

#define GET_FIELD(var, mask, pos)     (((var) & (mask)) >> (pos))

 /**************************************************************************
 *  type define
 **************************************************************************/
struct ddr_reg_base {
	void __iomem *ddrphy0_base;
	void __iomem *ddrphy1_base;
	void __iomem *dramc0_base;
	void __iomem *dramc1_base;
	void __iomem *dramc0_nao_base;
	void __iomem *dramc1_nao_base;
	void __iomem *ccif0_base;
	void __iomem *emi_base;
};

struct ddr_info_driver {
	struct device_driver driver;
	struct ddr_reg_base reg_base;
	const struct platform_device_id *id_table;
};

extern struct ddr_info_driver ddr_info_driver;
#endif				/* end of DDR_INFO_H */
