#ifndef _GPAD_DVFS_UTS_H
#define _GPAD_DVFS_UTS_H

#define DVFS_UTS_RAND_LOOP     4

/*!
 * DVFS policy unit test entry. 
 * @param dev    Handle of the device.
 * @param param  Test parameter from user space.
 *
 * @return 0 if succeeded, error code otherwise.
 */
long gpad_dvfs_utst(struct gpad_device *dev, int param);
#endif /* _GPAD_DVFS_UTS_H */
