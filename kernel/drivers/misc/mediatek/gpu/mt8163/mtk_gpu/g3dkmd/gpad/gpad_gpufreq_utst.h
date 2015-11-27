#ifndef _GPAD_GPUFREQ_UTS_H
#define _GPAD_GPUFREQ_UTS_H

#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

struct gpad_device;

/*!
 * GPUFREQ unit test entry.
 * @param dev    Handle of the device.
 * @param param  Test parameter from user space.
 *
 * @return 0 if succeeded, error code otherwise.
 */
long gpad_gpufreq_utst(struct gpad_device *dev, int param);

#endif /* _GPAD_DVFS_UTS_H */







