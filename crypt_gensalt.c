/*
 * Written by Solar Designer <solar at openwall.com> in 2000-2011.
 * No copyright is claimed, and the software is hereby placed in the public
 * domain.  In case this attempt to disclaim copyright and place the software
 * in the public domain is deemed null and void, then the software is
 * Copyright (c) 2000-2011 Solar Designer and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See crypt_blowfish.c for more information.
 *
 * This file contains salt generation functions for the traditional and
 * other common crypt(3) algorithms, except for bcrypt which is defined
 * entirely in crypt_blowfish.c.
 */

#include <string.h>
#include <stdio.h>
#include <sys/param.h>

#include <errno.h>
#ifndef __set_errno
#define __set_errno(val) errno = (val)
#endif

/* Just to make sure the prototypes match the actual definitions */
#include "crypt_gensalt.h"

unsigned char _crypt_itoa64[64 + 1] =
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char *_crypt_gensalt_traditional_rn(const char *prefix, unsigned long count,
	const char *input, int size, char *output, int output_size)
{
	(void) prefix;

	if (size < 2 || output_size < 2 + 1 || (count && count != 25)) {
		if (output_size > 0) output[0] = '\0';
		__set_errno((output_size < 2 + 1) ? ERANGE : EINVAL);
		return NULL;
	}

	output[0] = _crypt_itoa64[(unsigned int)input[0] & 0x3f];
	output[1] = _crypt_itoa64[(unsigned int)input[1] & 0x3f];
	output[2] = '\0';

	return output;
}

char *_crypt_gensalt_extended_rn(const char *prefix, unsigned long count,
	const char *input, int size, char *output, int output_size)
{
	unsigned long value;

	(void) prefix;

/* Even iteration counts make it easier to detect weak DES keys from a look
 * at the hash, so they should be avoided */
	if (size < 3 || output_size < 1 + 4 + 4 + 1 ||
	    (count && (count > 0xffffff || !(count & 1)))) {
		if (output_size > 0) output[0] = '\0';
		__set_errno((output_size < 1 + 4 + 4 + 1) ? ERANGE : EINVAL);
		return NULL;
	}

	if (!count) count = 725;

	output[0] = '_';
	output[1] = _crypt_itoa64[count & 0x3f];
	output[2] = _crypt_itoa64[(count >> 6) & 0x3f];
	output[3] = _crypt_itoa64[(count >> 12) & 0x3f];
	output[4] = _crypt_itoa64[(count >> 18) & 0x3f];
	value = (unsigned long)(unsigned char)input[0] |
		((unsigned long)(unsigned char)input[1] << 8) |
		((unsigned long)(unsigned char)input[2] << 16);
	output[5] = _crypt_itoa64[value & 0x3f];
	output[6] = _crypt_itoa64[(value >> 6) & 0x3f];
	output[7] = _crypt_itoa64[(value >> 12) & 0x3f];
	output[8] = _crypt_itoa64[(value >> 18) & 0x3f];
	output[9] = '\0';

	return output;
}

char *_crypt_gensalt_md5_rn(const char *prefix, unsigned long count,
	const char *input, int size, char *output, int output_size)
{
	unsigned long value;

	(void) prefix;

	if (size < 3 || output_size < 3 + 4 + 1 || (count && count != 1000)) {
		if (output_size > 0) output[0] = '\0';
		__set_errno((output_size < 3 + 4 + 1) ? ERANGE : EINVAL);
		return NULL;
	}

	output[0] = '$';
	output[1] = '1';
	output[2] = '$';
	value = (unsigned long)(unsigned char)input[0] |
		((unsigned long)(unsigned char)input[1] << 8) |
		((unsigned long)(unsigned char)input[2] << 16);
	output[3] = _crypt_itoa64[value & 0x3f];
	output[4] = _crypt_itoa64[(value >> 6) & 0x3f];
	output[5] = _crypt_itoa64[(value >> 12) & 0x3f];
	output[6] = _crypt_itoa64[(value >> 18) & 0x3f];
	output[7] = '\0';

	if (size >= 6 && output_size >= 3 + 4 + 4 + 1) {
		value = (unsigned long)(unsigned char)input[3] |
			((unsigned long)(unsigned char)input[4] << 8) |
			((unsigned long)(unsigned char)input[5] << 16);
		output[7] = _crypt_itoa64[value & 0x3f];
		output[8] = _crypt_itoa64[(value >> 6) & 0x3f];
		output[9] = _crypt_itoa64[(value >> 12) & 0x3f];
		output[10] = _crypt_itoa64[(value >> 18) & 0x3f];
		output[11] = '\0';
	}

	return output;
}

#define SHA2_SALT_LEN_MAX 16
#define SHA2_ROUNDS_MIN   1000
#define SHA2_ROUNDS_MAX   999999999
char *_crypt_gensalt_sha2_rn (const char *prefix, unsigned long count,
	const char *input, int size, char *output, int output_size)

{
	char *o = output;
	const char *i = input;
	unsigned needed = 3 + MIN(size/3*4, SHA2_SALT_LEN_MAX) + 1;

	if (size < 3 || output_size < needed)
		goto error;

	size = MIN(size, SHA2_SALT_LEN_MAX/4*3);

	o[0] = prefix[0];
	o[1] = prefix[1];
	o[2] = prefix[2];
	o += 3;

	if (count) {
		count = MAX(SHA2_ROUNDS_MIN, MIN(count, SHA2_ROUNDS_MAX));
		int n = snprintf (o, output_size-3, "rounds=%ld$", count);
		if (n < 0 || n >= output_size-3)
			goto error;
		needed += n;
		o += n;
	}

	if (output_size < needed)
		goto error;

	while (size >= 3) {
		unsigned long value =
			(unsigned long)(unsigned char)i[0] |
			((unsigned long)(unsigned char)i[1] << 8) |
			((unsigned long)(unsigned char)i[2] << 16);
		o[0] = _crypt_itoa64[value & 0x3f];
		o[1] = _crypt_itoa64[(value >> 6) & 0x3f];
		o[2] = _crypt_itoa64[(value >> 12) & 0x3f];
		o[3] = _crypt_itoa64[(value >> 18) & 0x3f];
		size -= 3;
		i += 3;
		o += 3;
	}
	o[0] = '\0';

	return output;

error:
	if (output_size > 0)
		output[0] = '\0';
	errno = ENOMEM;
	return NULL;
}


typedef unsigned int BF_word;

static const unsigned char BF_itoa64[64 + 1] =
	"./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void BF_encode(char *dst, const BF_word *src, int size)
{
	const unsigned char *sptr = (const unsigned char *)src;
	const unsigned char *end = sptr + size;
	unsigned char *dptr = (unsigned char *)dst;
	unsigned int c1, c2;

	do {
		c1 = *sptr++;
		*dptr++ = BF_itoa64[c1 >> 2];
		c1 = (c1 & 0x03) << 4;
		if (sptr >= end) {
			*dptr++ = BF_itoa64[c1];
			break;
		}

		c2 = *sptr++;
		c1 |= c2 >> 4;
		*dptr++ = BF_itoa64[c1];
		c1 = (c2 & 0x0f) << 2;
		if (sptr >= end) {
			*dptr++ = BF_itoa64[c1];
			break;
		}

		c2 = *sptr++;
		c1 |= c2 >> 6;
		*dptr++ = BF_itoa64[c1];
		*dptr++ = BF_itoa64[c2 & 0x3f];
	} while (sptr < end);
}

char *_crypt_gensalt_blowfish_rn(const char *prefix, unsigned long count,
	const char *input, int size, char *output, int output_size)
{
	if (size < 16 || output_size < 7 + 22 + 1 ||
	    (count && (count < 4 || count > 31)) ||
	    prefix[0] != '$' || prefix[1] != '2' ||
	    (prefix[2] != 'a' && prefix[2] != 'b' && prefix[2] != 'y')) {
		if (output_size > 0) output[0] = '\0';
		__set_errno((output_size < 7 + 22 + 1) ? ERANGE : EINVAL);
		return NULL;
	}

	if (!count) count = 5;

	output[0] = '$';
	output[1] = '2';
	output[2] = prefix[2];
	output[3] = '$';
	output[4] = '0' + count / 10;
	output[5] = '0' + count % 10;
	output[6] = '$';

	BF_encode(&output[7], (const BF_word *)input, 16);
	output[7 + 22] = '\0';

	return output;
}
