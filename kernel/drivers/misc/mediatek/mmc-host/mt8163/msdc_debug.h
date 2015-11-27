#ifndef __MSDC_DEBUG_H__
#define __MSDC_DEBUG_H__
   
#include "mt_sd.h"
#include <linux/printk.h>
#include <linux/xlog.h>

#ifdef MSDC_MAX_NUM
#undef MSDC_MAX_NUM
#endif
#define MSDC_MAX_NUM               HOST_MAX_NUM


//#define KEEP_SLIENT_BUILD
#define ___MSDC_DEBUG___

#define WHEREDOESMSDCRUN           "KN@MSDC" 

typedef struct MSDCDBGZNHELPER {
        char  szZnFiler[32][10];
unsigned int  u4MsdcDbgZoneMask;
}MSDCDBGZNHELPER;

#define MSDCDBGZONE(znbval)        (g_MsdcZnHelper.u4MsdcDbgZoneMask&(znbval))

#define MSDC_DBGZONE_INIT          (1 << 0)   
#define MSDC_DBGZONE_WARNING       (1 << 1) 
#define MSDC_DBGZONE_ERROR         (1 << 2) 
#define MSDC_DBGZONE_INFO          (1 << 3) 
#define MSDC_DBGZONE_DEBUG         (1 << 4) 
#define MSDC_DBGZONE_FUNC          (1 << 5)
#define MSDC_DBGZONE_CONFIG        (1 << 6) 
#define MSDC_DBGZONE_HREG          (1 << 7) 
#define MSDC_DBGZONE_DREG          (1 << 8)
#define MSDC_DBGZONE_TUNING        (1 << 9)
#define MSDC_DBGZONE_DMA           (1 << 10)
#define MSDC_DBGZONE_PIO           (1 << 11)
#define MSDC_DBGZONE_IRQ           (1 << 12)
#define MSDC_DBGZONE_STACK         (1 << 13)
#define MSDC_DBGZONE_HSTATE        (1 << 14)
#define MSDC_DBGZONE_DSTATUS       (1 << 15)

#define MSDC_DBGZONE_GPIO          (1 << 16)

#define STR_MSDC_INIT               "INIT" 
#define STR_MSDC_WARNING            "WARN"  
#define STR_MSDC_ERROR              "ERROR" 
#define STR_MSDC_INFO               "INFO" 
#define STR_MSDC_DEBUG              "DEBUG" 
#define STR_MSDC_FUNC               "FUNC" 
#define STR_MSDC_CONFIG             "CONFIG" 
#define STR_MSDC_HREG               "HREG"
#define STR_MSDC_DREG               "DREG"
#define STR_MSDC_TUNING             "TUNING"
#define STR_MSDC_DMA                "DMA"
#define STR_MSDC_PIO                "PIO"
#define STR_MSDC_IRQ                "IRQ"
#define STR_MSDC_STACK              "STACK"
#define STR_MSDC_HSTATE             "HSTATE"
#define STR_MSDC_DSTATUS            "DSTATUS"

#define STR_MSDC_UNUSED             "UNUSED"
//#define STR_MSDC_PREINIT            "PREINIT"
#define STR_MSDC_GPIO               "GPIO"

#define MSDC_FILTER_NAME(cond)      g_MsdcZnHelper.szZnFiler[POWER2ROOT(cond)]

#define MSDC_INIT                  MSDCDBGZONE(MSDC_DBGZONE_INIT) 
#define MSDC_WARN                  MSDCDBGZONE(MSDC_DBGZONE_WARNING)   
#define MSDC_ERROR                 MSDCDBGZONE(MSDC_DBGZONE_ERROR) 
#define MSDC_INFO                  MSDCDBGZONE(MSDC_DBGZONE_INFO) 
#define MSDC_DEBUG                 MSDCDBGZONE(MSDC_DBGZONE_DEBUG) 
#define MSDC_FUNC                  MSDCDBGZONE(MSDC_DBGZONE_FUNC) 
#define MSDC_CONFIG                MSDCDBGZONE(MSDC_DBGZONE_CONFIG) 
#define MSDC_HREG                  MSDCDBGZONE(MSDC_DBGZONE_HREG)
#define MSDC_DREG                  MSDCDBGZONE(MSDC_DBGZONE_DREG)
#define MSDC_TUNING                MSDCDBGZONE(MSDC_DBGZONE_TUNING)
#define MSDC_DMA                   MSDCDBGZONE(MSDC_DBGZONE_DMA)
#define MSDC_PIO                   MSDCDBGZONE(MSDC_DBGZONE_PIO)
#define MSDC_IRQ                   MSDCDBGZONE(MSDC_DBGZONE_IRQ)
#define MSDC_STACK                 MSDCDBGZONE(MSDC_DBGZONE_STACK)
#define MSDC_HSTATE                MSDCDBGZONE(MSDC_DBGZONE_HSTATE)
#define MSDC_DSTATUS               MSDCDBGZONE(MSDC_DBGZONE_DSTATUS)

#define MSDC_GPIO                  MSDCDBGZONE(MSDC_DBGZONE_GPIO)

#define MSDC_DBGZONE_RELEASE       MSDC_DBGZONE_INIT|MSDC_DBGZONE_ERROR|MSDC_DBGZONE_INFO


#ifdef KEEP_SLIENT_BUILD
#define MSDC_ERR_PRINT(cond, fmt, ...) ((void)0)
#define MSDC_TRC_PRINT(cond, fmt, ...) ((void)0)
#define MSDC_DBG_PRINT(cond, fmt, ...) ((void)0)
#define MSDC_DBGCHK(friendname, exp)   ((void)0)
#define MSDC_BUGON(exp)                ((void)0)
#define MSDC_ASSERT(exp)                   ((void)0)
#define MSDC_RAW_PRINT(cond, fmt, ...)     ((void)0)
#define MSDC_PRINT(cond, fmt, ...)         ((void)0)
#define MSDC_TRACE(cond, cb, exp)     cb exp
#else 
#ifdef ___MSDC_DEBUG___

#define MSDC_DBG_PRINT(cond, fmt, ...)   \
   ((cond) ? (printk(KERN_ERR "%s: KN@MSDC "fmt" <- L<%d> PID<%s><0x%x>\n", MSDC_FILTER_NAME(cond),\
   				       ##__VA_ARGS__, __LINE__, current->comm, current->pid), 1) : 0)
   				  
#define MSDC_DBGCHK(friendname, exp) \
   ((exp)?(printk(KERN_ERR friendname": DBGCHK failed in file %s at line %d \r\n", \
                 					__FILE__ ,__LINE__ ),  \
	   																1)  \
   :0)

#define MSDC_TRACE(cond, cb, exp) \
   ((cond)?(printk(KERN_ERR #cb ": invoked in file %s at line %d \r\n", \
                 					__FILE__ ,__LINE__ ),  cb exp,\
	   																1)  \
   :0)

#define MSDC_BUGON(exp) MSDC_DBGCHK(WHEREDOESMSDCRUN, exp)
#else  /* ___MSDC_DEBUG___*/
#define MSDC_DBG_PRINT(cond, fmt, ...) ((void)0)
#define MSDC_DBGCHK(friendname, exp)   ((void)0)
#define MSDC_BUGON(exp)                    ((void)0)
#define MSDC_TRACE(cond, cb, exp)     cb exp
#endif /* ___MSDC_DEBUG___*/

static inline int MUSTBE(void)
{
       dump_stack();
       while(1);
	   return 0;
}
static inline unsigned int POWER2ROOT(unsigned int u4Power)
{
    unsigned int u4Exponent = 0;
	
	while(u4Power > 1){
       u4Power >>= 1;
	   u4Exponent ++;
	}

	return u4Exponent;
}
#ifndef MSDC_DEBUG_KICKOFF
extern  MSDCDBGZNHELPER  g_MsdcZnHelper;

#else /* MSDC_DEBUG_KICKOFF */
MSDCDBGZNHELPER  g_MsdcZnHelper = {
		{  STR_MSDC_INIT,    STR_MSDC_WARNING,   STR_MSDC_ERROR,   STR_MSDC_INFO,
		   STR_MSDC_DEBUG,   STR_MSDC_FUNC,      STR_MSDC_CONFIG,  STR_MSDC_HREG,        
	       STR_MSDC_DREG,    STR_MSDC_TUNING,    STR_MSDC_DMA,     STR_MSDC_PIO,    
	       STR_MSDC_IRQ,     STR_MSDC_STACK,     STR_MSDC_HSTATE,  STR_MSDC_DSTATUS,
	       
	       STR_MSDC_GPIO,    STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED,
	       STR_MSDC_UNUSED,  STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED,
		   STR_MSDC_UNUSED,  STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED,
		   STR_MSDC_UNUSED,  STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED	
		},
		MSDC_DBGZONE_RELEASE

};
#endif /* MSDC_DEBUG_KICKOFF */

#define MSDC_ASSERT(exp) \
   ((exp)?1:(          \
       printk(KERN_ERR " KN@MSDC : MSDC_ASSERT MUSTBE: failed in file %s at line %d \r\n", \
                 __FILE__ ,__LINE__ ),    \
       MUSTBE(), \
       0  \
   ))


#define MSDC_TRC_PRINT(cond, fmt, ...)   \
  ((cond)? (printk(KERN_ERR "%s: KN@MSDC "fmt" <- L<%d> PID<%s><0x%x>\n", MSDC_FILTER_NAME(cond),\
   				     ##__VA_ARGS__, __LINE__, current->comm, current->pid), 1):0)

#define MSDC_ERR_PRINT(cond, fmt, ...)	 \
						((cond)?(printk(KERN_ERR "***ERROR:%s: KN@MSDC : %s line %d: "fmt" <- PID<%s><0x%x>\n",\
								 MSDC_FILTER_NAME(cond),__FILE__,__LINE__, ##__VA_ARGS__, current->comm, current->pid), 1) : 0)
#define MSDC_RAW_PRINT(cond, fmt, ...)   \
   ((cond)?(printk(KERN_ERR fmt,##__VA_ARGS__),1):0)

#define MSDC_PRINT(cond, fmt, ...)   \
   ((cond)?(printk(KERN_ERR fmt,##__VA_ARGS__),1):0)

#endif  /* KEEP_SLIENT_BUILD*/    
#endif /* __MSDC_DEBUG_H__*/

