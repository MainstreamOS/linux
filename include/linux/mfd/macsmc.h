/* SPDX-License-Identifier: GPL-2.0-only OR MIT */
/*
 * Apple SMC (System Management Controller) core definitions
 *
 * Copyright (C) The Asahi Linux Contributors
 */

#ifndef _LINUX_MFD_MACSMC_H
#define _LINUX_MFD_MACSMC_H

#include <linux/miscdevice.h>
#include <linux/soc/apple/rtkit.h>
#include <uapi/linux/macsmc.h>

#define SMC_MSG_READ_KEY                0x10
#define SMC_MSG_WRITE_KEY               0x11
#define SMC_MSG_GET_KEY_BY_INDEX        0x12
#define SMC_MSG_GET_KEY_INFO            0x13
#define SMC_MSG_INITIALIZE              0x17
#define SMC_MSG_NOTIFICATION            0x18
#define SMC_MSG_RW_KEY                  0x20

#define SMC_INT_SHUTDOWN                0x40
#define SMC_INT_BRIDGEOS_CRASH          0x41
#define SMC_INT_ALS_CHANGE              0x2A
#define SMC_INT_CMD_DONE                0x4B
#define SMC_INT_THERMAL_LEVEL           0x54
#define SMC_INT_PLATFORM_FUNCTION       0x55
#define SMC_INT_SMS_DROP                0x60
#define SMC_INT_SMS_ORIENTATION 0x69
#define SMC_INT_SMS_SHOCK               0x6F
#define SMC_INT_PLIMIT_CHANGED          0x80

/**
 * enum apple_smc_boot_stage - SMC boot stage
 * @APPLE_SMC_BOOTING: SMC is booting
 * @APPLE_SMC_INITIALIZED: SMC is initialized and ready to use
 * @APPLE_SMC_ERROR_NO_SHMEM: Shared memory could not be initialized during boot
 * @APPLE_SMC_ERROR_CRASHED: SMC has crashed
 */
enum apple_smc_boot_stage {
	APPLE_SMC_BOOTING,
	APPLE_SMC_INITIALIZED,
	APPLE_SMC_ERROR_NO_SHMEM,
	APPLE_SMC_ERROR_CRASHED
};

/**
 * struct apple_smc_backend_ops - SMC backend interface
 * @read: Read SMC key data
 * @write: Write SMC key data
 * @enter_atomic: Enter atomic access mode
 * @write_atomic: Atomic write to SMC key
 * @rw: Combined write/read transaction
 * @get_key_by_index: Resolve key by index
 * @get_key_info: Get metadata for key
 * @disable_notifications: Disable notfications on removal
 */
struct apple_smc_backend_ops {
	int (*read)(void *cookie, smc_key key, void *buf, size_t size);
	int (*write)(void *cookie, smc_key key, const void *buf, size_t size);
	int (*enter_atomic)(void *cookie);
	int (*write_atomic)(void *cookie, smc_key key, const void *buf, size_t size);
	int (*rw)(void *cookie, smc_key key, const void *wbuf, size_t wsize,
		  void *rbuf, size_t rsize);
	int (*get_key_by_index)(void *cookie, int index, smc_key *key);
	int (*get_key_info)(void *cookie, smc_key key,
			    struct apple_smc_key_info *info);
	void (*disable_notifications)(void *cookie);
};


/**
 * struct apple_smc
 * @dev: Underlying device struct for the physical backend device
 * @key_count: Number of available SMC keys
 * @first_key: First valid SMC key
 * @last_key: Last valid SMC key
 * @event_handlers: Pointer to Notifier call chain from the backend
 * @be: Backend operations for SMC access
 * @be_cookie: Backend context passed to ops callbacks
 * @is_acpi: Is the backend ACPI based
 * @mutex: Mutex for non-atomic mode
 * @lock: Spinlock for atomic mode
 * @miscdev: Misc Device struct for ioctl ops
 */
struct apple_smc {
	struct device *dev;

	u32 key_count;
	smc_key first_key;
	smc_key last_key;

	struct blocking_notifier_head *event_handlers;
	const struct apple_smc_backend_ops *be;
	void *be_cookie;

	bool is_acpi;

	struct mutex mutex;
	spinlock_t lock;

	struct miscdevice miscdev;
};

/**
 * apple_smc_read - Read size bytes from given SMC key into buf
 * @smc: Pointer to apple_smc struct
 * @key: smc_key to be read
 * @buf: Buffer into which size bytes of data will be read from SMC
 * @size: Number of bytes to be read into buf
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_read(struct apple_smc *smc, smc_key key, void *buf, size_t size);

/**
 * apple_smc_write - Write size bytes into given SMC key from buf
 * @smc: Pointer to apple_smc struct
 * @key: smc_key data will be written to
 * @buf: Buffer from which size bytes of data will be written to SMC
 * @size: Number of bytes to be written
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_write(struct apple_smc *smc, smc_key key, const void *buf, size_t size);

/**
 * apple_smc_enter_atomic - Enter atomic mode to be able to use apple_smc_write_atomic
 * @smc: Pointer to apple_smc struct
 *
 * This function switches the SMC backend to atomic mode which allows the
 * use of apple_smc_write_atomic while disabling *all* other functions.
 * This is only used for shutdown/reboot which requires writing to a SMC
 * key from atomic context.
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_enter_atomic(struct apple_smc *smc);

/**
 * apple_smc_write_atomic - Write size bytes into given SMC key from buf without sleeping
 * @smc: Pointer to apple_smc struct
 * @key: smc_key data will be written to
 * @buf: Buffer from which size bytes of data will be written to SMC
 * @size: Number of bytes to be written
 *
 * Note that this function will fail if apple_smc_enter_atomic hasn't been
 * called before.
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_write_atomic(struct apple_smc *smc, smc_key key, const void *buf, size_t size);

/**
 * apple_smc_rw - Write and then read using the given SMC key
 * @smc: Pointer to apple_smc struct
 * @key: smc_key data will be written to
 * @wbuf: Buffer from which size bytes of data will be written to SMC
 * @wsize: Number of bytes to be written
 * @rbuf: Buffer to which size bytes of data will be read from SMC
 * @rsize: Number of bytes to be read
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_rw(struct apple_smc *smc, smc_key key, const void *wbuf, size_t wsize,
		 void *rbuf, size_t rsize);

/**
 * apple_smc_get_key_by_index - Given an index return the corresponding SMC key
 * @smc: Pointer to apple_smc struct
 * @index: Index to be resolved
 * @key: Buffer for SMC key to be returned
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_get_key_by_index(struct apple_smc *smc, int index, smc_key *key);

/**
 * apple_smc_get_key_info - Get key information from SMC
 * @smc: Pointer to apple_smc struct
 * @key: Key to acquire information for
 * @info: Pointer to struct apple_smc_key_info which will be filled
 *
 * Return: Zero on success, negative errno on error
 */
int apple_smc_get_key_info(struct apple_smc *smc, smc_key key, struct apple_smc_key_info *info);

/**
 * apple_smc_key_exists - Check if the given SMC key exists
 * @smc: Pointer to apple_smc struct
 * @key: smc_key to be checked
 *
 * Return: True if the key exists, false otherwise
 */
static inline bool apple_smc_key_exists(struct apple_smc *smc, smc_key key)
{
	return apple_smc_get_key_info(smc, key, NULL) >= 0;
}

/**
 * apple_smc_probe - Initialize Apple SMC device
 * @dev: Device for the SMC instance
 * @ops: Backend operations for SMC access
 * @cookie: Opaque backend context
 * @event_handlers: Pointer to Notifier call chain from backend
 * @is_acpi: Is the backend ACPI based.
 * Return: Zero on success, negative errno on error
 */
int apple_smc_probe(struct device *dev, const struct apple_smc_backend_ops *ops,
		void *cookie, struct blocking_notifier_head *event_handlers, bool is_acpi);

#define APPLE_SMC_TYPE_OPS(type) \
	static inline int apple_smc_read_##type(struct apple_smc *smc, smc_key key, type *p) \
	{ \
		int ret = apple_smc_read(smc, key, p, sizeof(*p)); \
		return (ret < 0) ? ret : ((ret != sizeof(*p)) ? -EINVAL : 0); \
	} \
	static inline int apple_smc_write_##type(struct apple_smc *smc, smc_key key, type p) \
	{ \
		return apple_smc_write(smc, key, &p, sizeof(p)); \
	} \
	static inline int apple_smc_write_##type##_atomic(struct apple_smc *smc, smc_key key, type p) \
	{ \
		return apple_smc_write_atomic(smc, key, &p, sizeof(p)); \
	} \
	static inline int apple_smc_rw_##type(struct apple_smc *smc, smc_key key, \
					      type w, type *r) \
	{ \
		int ret = apple_smc_rw(smc, key, &w, sizeof(w), r, sizeof(*r)); \
		return (ret < 0) ? ret : ((ret != sizeof(*r)) ? -EINVAL : 0); \
	}

APPLE_SMC_TYPE_OPS(u64)
APPLE_SMC_TYPE_OPS(u32)
APPLE_SMC_TYPE_OPS(u16)
APPLE_SMC_TYPE_OPS(u8)
APPLE_SMC_TYPE_OPS(s64)
APPLE_SMC_TYPE_OPS(s32)
APPLE_SMC_TYPE_OPS(s16)
APPLE_SMC_TYPE_OPS(s8)

static inline int apple_smc_read_flag(struct apple_smc *smc, smc_key key, bool *flag)
{
	u8 val;
	int ret = apple_smc_read_u8(smc, key, &val);

	if (ret < 0)
		return ret;

	*flag = val ? true : false;
	return ret;
}

static inline int apple_smc_write_flag(struct apple_smc *smc, smc_key key, bool state)
{
	return apple_smc_write_u8(smc, key, state ? 1 : 0);
}

static inline int apple_smc_write_flag_atomic(struct apple_smc *smc, smc_key key, bool state)
{
	return apple_smc_write_u8_atomic(smc, key, state ? 1 : 0);
}

#endif
