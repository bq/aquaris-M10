/*
 * (c) TRUSTONIC LIMITED 2013
 */

 
#ifndef __TLAPIROT13_H__
#define __TLAPIROT13_H__

#include "TlApi/TlApiCommon.h"
#include "TlApi/TlApiError.h"

/* Marshaled function parameters.
 * structs and union of marshaling parameters via TlApi.
 *
 * @note The structs can NEVER be packed !
 * @note The structs can NOT used via sizeof(..) !
 */
 

/** Encode cleartext with rot13.
 *
 * @param encodeData Pointer to the tlApiEncodeRot13_t structure 
 * @return TLAPI_OK if data has successfully been encrypted.
 */

//_TLAPI_EXTERN_C tlApiResult_t tlApiAddRpmb(tlApiRpmb_ptr RpmbData);

#endif // __TLAPIROT13_H__



