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
OC_HANDLER(start)

#include <syscalls/output_cmp_stop_mrsh.c>
#include <syscalls/output_cmp_start_mrsh.c>

static inline int z_vrfy_precise_timer_set_ovf(struct device *dev,
					       const struct ovf_cfg *cfg)
{
	struct ovf_cfg cfg_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_OUTPUT_CMP(dev, set_ovf));
	Z_OOPS(z_user_from_copy(&cfg_copy, cfg, sizeof(cfg_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cfg_copy.callback == 0,
				    "callbacks may not be set from user mode"));
	return z_impl_output_cmp_set_ovf((struct device *)dev, cfg_copy);

}
#include <syscalls/output_cmp_set_ovf_mrsh.c>
