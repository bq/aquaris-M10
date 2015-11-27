#pragma once

#include "../yl_mmu_mapper.h"


#ifdef __cplusplus
extern "C" {
#endif



size_t times( char c, int n, char *buf );

size_t render( Mapper m, Trunk t, char c_section, char c_small, char *buf );




#ifdef __cplusplus
}
#endif

