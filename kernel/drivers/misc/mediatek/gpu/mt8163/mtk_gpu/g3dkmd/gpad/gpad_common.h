#ifndef _GPAD_COMMON_H
#define _GPAD_COMMON_H

#include "gpad_def.h"
#include "gpad_log.h"
#include "gpad_dev.h"

#define GPAD_UNUSED(x) (void)(x)

#define gpad_find_dev(_id) \
            gpad_get_default_dev()

struct gpad_device *gpad_get_default_dev(void);

#endif /* _GPAD_COMMON_H */
