/*
 * (c) TRUSTONIC LIMITED 2013
 */


#ifndef __DRERROR_H__
#define __DRERROR_H__

#define E_DRAPI_DRV_ROT13 0x40001

/*
 * Driver fatal error codes.
 */
typedef enum {
    E_DR_OK               = 0, /**< Success */
    E_DR_IPC              = 1, /**< IPC error */
    E_DR_INTERNAL         = 2, /**< Internal error */
    /* ... add more error codes when required */
} drError_t;


#endif // __DRERROR_H__



