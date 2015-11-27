#pragma once

#if defined(_WIN32)
#include <stdio.h>
#else
//#include <linux/types.h>
#if defined(linux_user_mode)
#include <stdio.h>
#endif
#endif

#include "yl_mmu_utility_common.h"

#include "../yl_mmu_mapper.h"




#ifdef __cplusplus
extern "C" {
#endif


#if defined(_WIN32)
void Statistics_Trunk_Usage_Map( Mapper m, FILE *fp );
#else
#if defined(linux_user_mode) // linux user space 
void Statistics_Trunk_Usage_Map( Mapper m, FILE *fp );
#else
void Statistics_Trunk_Usage_Map( Mapper m );
#endif
#endif

#ifdef __cplusplus
}
#endif
