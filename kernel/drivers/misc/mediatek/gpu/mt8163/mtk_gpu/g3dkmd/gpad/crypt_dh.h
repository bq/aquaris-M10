/*
 * (c) Ralink Technology, Inc. 2002
 */

#ifndef __CRYPT_DH_H__
#define __CRYPT_DH_H__

#include "crypt_config.h"


/* DH operations */
void DH_PublicKey_Generate (
    IN UINT8 GValue[],
    IN UINT GValueLength,
    IN UINT8 PValue[],
    IN UINT PValueLength,
    IN UINT8 PrivateKey[],
    IN UINT PrivateKeyLength,
    OUT UINT8 PublicKey[],
    INOUT UINT *PublicKeyLength);

void DH_SecretKey_Generate (
    IN UINT8 PublicKey[],
    IN UINT PublicKeyLength,
    IN UINT8 PValue[],
    IN UINT PValueLength,
    IN UINT8 PrivateKey[],
    IN UINT PrivateKeyLength,
    OUT UINT8 SecretKey[],
    INOUT UINT *SecretKeyLength);

#define RT_DH_PublicKey_Generate(GK, GKL, PV, PVL, PriK, PriKL, PubK, PubKL) \
    DH_PublicKey_Generate((GK), (GKL), (PV), (PVL), (PriK), (PriKL), (UINT8 *) (PubK), (UINT *) (PubKL))

#define RT_DH_SecretKey_Generate(PubK, PubKL, PV, PVL, PriK, PriKL, SecK, SecKL) \
    DH_SecretKey_Generate((PubK), (PubKL), (PV), (PVL), (PriK), (PriKL), (UINT8 *) (SecK), (UINT *) (SecKL))

#define RT_DH_FREE_ALL()

    
#endif /* __CRYPT_DH_H__ */

