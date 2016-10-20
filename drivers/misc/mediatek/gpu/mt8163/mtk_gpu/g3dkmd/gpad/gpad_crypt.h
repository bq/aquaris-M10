/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _GPAD_CRYPT_H
#define _GPAD_CRYPT_H

#define GPAD_CRYPT_KEY_SIZE   (32)
#define GPAD_PRIVATE_KEY_SIZE (4)
#define GPAD_AES_BLOCK_SIZE   (16)

int gpad_crypt_update_key(unsigned char *key, int key_len);

int gpad_crypt_decrypt(unsigned char *enc_data, unsigned int enc_size, unsigned char *dec_data, unsigned int dec_size);

#endif /* _GPAD_CRYPT_H */

