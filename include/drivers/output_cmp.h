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


typedef void (*ovf_match_callback_t)(struct device *dev,
			       	     u32_t ticks,
			       	     void *user_data);


struct counter_cfg {
	ovf_match_callback_t ovf_match_callback;
	u32_t counter_val;
	void *user_data;
};


typedef int (*oc_api_start)(struct device *dev,
			    const struct counter_cfg * ovf_cfg);
typedef int (*oc_api_stop)(struct device *dev);
typedef int (*oc_api_set_compare)(struct device *dev, u8_t channel,
				  const struct counter_cfg *match_cfg);
typedef int (*oc_api_update_compare)(struct device *dev, u8_t channel,
				     u32_t match);
typedef u32_t (*oc_api_get_counter)(struct device *dev);

__subsystem struct output_cmp_driver_api {
	oc_api_start start;
	oc_api_stop stop;
	oc_api_get_counter get_counter;
	oc_api_set_compare set_cmp;
	oc_api_update_compare update_cmp;
};


__syscall int output_cmp_start(struct device *dev,
			       const struct counter_cfg * ovf_cfg);

static inline int z_impl_output_cmp_start(struct device *dev,
					  const struct counter_cfg * ovf_cfg)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->start(dev, ovf_cfg);
}

__syscall int output_cmp_stop(struct device *dev);

static inline int z_impl_output_cmp_stop(struct device *dev)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->stop(dev);
}

__syscall u32_t output_cmp_get_counter(struct device *dev);

static inline u32_t z_impl_output_cmp_get_counter(struct device *dev)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->get_counter(dev);
}

__syscall int output_cmp_set_compare(struct device *dev, u8_t channel,
				     const struct counter_cfg *match_cfg);

static inline int z_impl_output_cmp_set_compare(struct device *dev,
	u8_t channel,
	const struct counter_cfg *match_cfg)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->set_cmp(dev, channel, match_cfg);
}

__syscall int output_cmp_update_compare(struct device *dev, u8_t channel,
					u32_t match);

static inline int z_impl_output_cmp_update_compare(struct device *dev,
						   u8_t channel,
						   u32_t match)
{
	const struct output_cmp_driver_api *api =
			(struct output_cmp_driver_api *)dev->driver_api;

	return api->update_cmp(dev, channel, match);

}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <syscalls/output_cmp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OUTPUT_CMP_H_ */
