/* SPDX-License-Identifier: GPL-2.0-only OR MIT WITH Linux-syscall-note */
/*
 * Apple SMC Public Interface
 *
 * Copyright (C) 2026 Atharva Tiwari <atharvatiwarilinuxdev@gmail.com>
 */

#ifndef _UAPI_MACSMC_H
#define _UAPI_MACSMC_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * typedef smc_key - Alias for u32 to be used for SMC keys
 *
 * SMC keys are 32bit integers containing packed ASCII characters in natural
 * integer order, i.e. 0xAABBCCDD, which represent the FourCC ABCD.
 * The SMC driver is designed with this assumption and ensures the right
 * endianness is used when these are stored to memory and sent to or received
 * from the actual SMC firmware (which can be done in either shared memory or
 * as 64bit mailbox message on Apple Silicon).
 * Internally, SMC stores these keys in a table sorted lexicographically and
 * allows resolving an index into this table to the corresponding SMC key.
 * Thus, storing keys as u32 is very convenient as it allows to e.g. use
 * normal comparison operators which directly map to the natural order used
 * by SMC firmware.
 *
 * This simple type alias is introduced to allow easy recognition of SMC key
 * variables and arguments.
 */
typedef __u32 smc_key;

/**
 * SMC_KEY - Convert FourCC SMC keys in source code to smc_key
 *
 * This macro can be used to easily define FourCC SMC keys in source code
 * and convert these to u32 / smc_key, e.g. SMC_KEY(NTAP) will expand to
 * 0x4e544150.
 *
 * @s: FourCC SMC key to be converted
 */
#define SMC_KEY(s) ((smc_key)(_SMC_KEY(#s)))
#define _SMC_KEY(s) (((s)[0] << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])
#define __SMC_KEY(a, b, c, d) (((__u32)(a) << 24) | ((__u32)(b) << 16) | ((__u32)(c) << 8) | ((__u32)(d)))

#define APPLE_SMC_READABLE		BIT(7)
#define APPLE_SMC_WRITABLE		BIT(6)
#define APPLE_SMC_FUNCTION		BIT(4)
#define APPLE_SMC_CONST		BIT(3)
#define APPLE_SMC_ATOMIC		BIT(2)
#define APPLE_SMC_PRIVATE_READ		BIT(1)
#define APPLE_SMC_PRIVATE_WRITE		BIT(0)

/**
 * struct apple_smc_key_info - Information for a SMC key as returned by SMC
 * @type_code: FourCC code indicating the type for this key.
 *      Known types:
 *       ch8*: ASCII string
 *       flag: Boolean, 1 or 0
 *       flt: 32-bit single-precision IEEE 754 float
 *       hex: Binary data
 *       ioft: 64bit Unsigned fixed-point intger (48.16)
 *       {si,ui}{8,16,32,64}: Signed/Unsigned 8-/16-/32-/64-bit integer
 * @size: Size of the buffer associated with this key
 * @flags: Bitfield encoding flags (APPLE_SMC_{READABLE,WRITABLE,FUNCTION,
 *       CONST,ATOMIC,PRIVATE_READ,PRIVATE_WRITE})
 */
struct apple_smc_key_info {
	__u32 type_code;
	__u8 size;
	__u8 flags;
};

/**
 * struct ioctl_smc_rw_payload - argument for IOCTL_SMC_READ and IOCTL_SMC_WRITE
 * @key: SMC key to access. Input.
 * @size: Size of the key's data in bytes. Input. It must match the size the
 *	SMC reports for @key (see IOCTL_SMC_GET_KEY_INFO) and cannot exceed
 *	the size of @buf.
 * @buf: Key data. For IOCTL_SMC_READ this is filled in by the kernel with the
 *	value read from @key. For IOCTL_SMC_WRITE it holds the value to write to
 *	@key.
 */
struct ioctl_smc_rw_payload {
	smc_key key;
	__u32 size;
	__u8 buf[32];
};

/**
 * struct ioctl_smc_index_payload - argument for IOCTL_SMC_GET_KEY_INDEX
 * @index: Position in the SMC's key table. Input.
 * @key: Key stored at @index. Filled in by the kernel.
 *
 * Calling this with increasing @index values lists every key the SMC exposes.
 */
struct ioctl_smc_index_payload {
	__u16 index;
	smc_key key;
};

/**
 * struct ioctl_smc_info_payload - argument for IOCTL_SMC_GET_KEY_INFO
 * @key: SMC key to look up. Input.
 * @info: Size, type code and flags of @key. Filled in by the kernel.
 */
struct ioctl_smc_info_payload {
	smc_key key;
	struct apple_smc_key_info info;
};

#define APPLE_SMC_MAGIC 'S'
#define IOCTL_SMC_READ    _IOWR(APPLE_SMC_MAGIC, 1, struct ioctl_smc_rw_payload)
#define IOCTL_SMC_WRITE   _IOW(APPLE_SMC_MAGIC, 2, struct ioctl_smc_rw_payload)
#define IOCTL_SMC_GET_KEY_INDEX  _IOWR(APPLE_SMC_MAGIC, 3, struct ioctl_smc_index_payload)
#define IOCTL_SMC_GET_KEY_INFO   _IOWR(APPLE_SMC_MAGIC, 4, struct ioctl_smc_info_payload)

#endif /* _UAPI_MACSMC_H */
