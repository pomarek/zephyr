/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/output_cmp.h>

/* For those APIs that just take one argument which is a counter driver
 * instance and return an integral value
 */
#define OC_HANDLER(name) \
	static inline int z_vrfy_output_cmp_##name(struct device *dev) \
	{ \
		Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, name)); \
		return z_impl_output_cmp_ ## name((struct device *)dev); \
	}

OC_HANDLER(stop)
#include <syscalls/output_cmp_stop_mrsh.c>

static inline int z_vrfy_output_cmp_start(struct device *dev,
					  const struct counter_cfg * ovf_cfg)
{
	struct counter_cfg cfg_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, start));
	Z_OOPS(z_user_from_copy(&cfg_copy, ovf_cfg, sizeof(cfg_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cfg_copy.ovf_match_callback == 0,
				    "callbacks may not be set from user mode"));
	return z_impl_output_cmp_start((struct device *)dev, cfg_copy);
}
#include <syscalls/output_cmp_start_mrsh.c>

static inline int z_vrfy_output_cmp_set_compare(struct device *dev,
						u8_t channel,
						const struct counter_cfg *cfg)
{
	struct counter_cfg cfg_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, set_compare));
	Z_OOPS(z_user_from_copy(&cfg_copy, cfg, sizeof(cfg_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cfg_copy.ovf_match_callback == 0,
				    "callbacks may not be set from user mode"));
	return z_impl_output_cmp_set_compare((struct device *)dev, channel,
					     cfg_copy);

}
#include <syscalls/output_cmp_set_compare_mrsh.c>

static inline int z_vrfy_output_cmp_update_compare(struct device *dev,
						   u8_t channel,
						   u32_t match)
{
	Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, update_compare));
	return z_impl_output_cmp_update_compare((struct device *)dev, channel,
						match);

}
#include <syscalls/output_cmp_update_compare_mrsh.c>

static inline u32_t output_cmp_get_counter(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, get_counter));
	return z_impl_output_cmp_get_counter((struct device *)dev);

}
#include <syscalls/output_cmp_get_counter_mrsh.c>
