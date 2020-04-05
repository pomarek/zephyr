/*
 * Copyright (c) 2020 Marek Porwisz
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for timers output compare drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OUTPUT_CMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OUTPUT_CMP_H_

/**
 * @brief Output Compare Interface
 * @defgroup oc_interface Output Compare Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ovf_callback_t)(struct device *dev,
			       u32_t ticks,
			       void *user_data);


struct ovf_cfg {
	ovf_callback_t callback;
	uint16_t max_val;
	void *user_data;
};


typedef int (*oc_api_start)(struct device *dev);
typedef int (*oc_api_stop)(struct device *dev);
typedef int (*oc_api_set_ovf)(struct device *dev,
				const struct ovf_cfg *alarm_cfg);

__subsystem struct output_cmp_driver_api {
	oc_api_start start;
	oc_api_stop stop;
	oc_api_set_ovf set_ovf;
};


__syscall int output_cmp_start(struct device *dev);

static inline int z_impl_output_cmp_start(struct device *dev)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->start(dev);
}

__syscall int output_cmp_stop(struct device *dev);

static inline int z_impl_output_cmp_stop(struct device *dev)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->stop(dev);
}

__syscall int output_cmp_set_ovf(struct device *dev,
				    const struct ovf_cfg *cfg);

static inline int z_impl_output_cmp_set_ovf(struct device *dev,
					    const struct ovf_cfg *ovf_cfg)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->set_ovf(dev, ovf_cfg);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <syscalls/precise_timer.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OUTPUT_CMP_H_ */
