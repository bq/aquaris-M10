#ifndef _GPAD_IPEM_UTS_H
#define _GPAD_IPEM_UTS_H

#define IPEM_UTS_RAND_LOOP	4

/*!
 * IPEM policy unit test entry. 
 * @param dev	Handle of the device.
 * @param param  Test parameter from user space.
 *
 * @return 0 if succeeded, error code otherwise.
 */
long gpad_ipem_utst(struct gpad_device *dev, int param);
#endif /* _GPAD_IPEM_UTS_H */
