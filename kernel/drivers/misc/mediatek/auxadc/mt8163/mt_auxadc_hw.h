#ifndef _MTK_ADC_HW_H
#define _MTK_ADC_HW_H

#include <mach/mt_reg_base.h>

//unsigned long auxadc_base;

void __iomem * auxadc_base;

//extern unsigned long apmixedsys_base;

#define AUXADC_CON0                     (auxadc_base + 0x000)
#define AUXADC_CON1                     (auxadc_base + 0x004)
#define AUXADC_CON2                     (auxadc_base + 0x010)
#define AUXADC_DAT0                     (auxadc_base + 0x014)
#define AUXADC_TP_CMD            (auxadc_base + 0x005c)
#define AUXADC_TP_ADDR           (auxadc_base + 0x0060)
#define AUXADC_TP_CON0           (auxadc_base + 0x0064)
#define AUXADC_TP_DATA0          (auxadc_base + 0x0074)
#define AUXADC_MISC              (auxadc_base + 0x0094)
#define AUXADC_DET_VOLT                 (auxadc_base + 0x084)
#define AUXADC_DET_SEL                  (auxadc_base + 0x088)
#define AUXADC_DET_PERIOD               (auxadc_base + 0x08C)
#define AUXADC_DET_DEBT                 (auxadc_base + 0x090)

#define PAD_AUX_XP				13
#define TP_CMD_ADDR_X			0x0005

//#define AUXADC_CON_RTP		(apmixedsys_base + 0x0404)

#endif	 /*_MTK_ADC_HW_H*/
