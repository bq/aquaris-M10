#include "yl_mmu_mapper_helper.h"

//#include <assert.h>

#include "yl_mmu_mapper_public_class.h"
#include "yl_mmu_hw_table.h"



#ifdef __cplusplus
extern "C" {
#endif



//size_t
//Mapper_get_required_size_all()
//{
//    return Mapper_get_required_size_mapper() 
//        + Mapper_get_required_size_page_table() 
//        + Mapper_get_required_size_meta_table();
//}

size_t 
Mapper_get_required_size_mapper()
{
    return sizeof(MapperRec);
}

size_t
Mapper_get_required_size_page_table()
{
    return PageTable_get_required_size() ;
}

size_t
Mapper_get_required_size_meta_table()
{
    return MetaTable_get_required_size() ;
}

#ifdef __cplusplus
}
#endif
