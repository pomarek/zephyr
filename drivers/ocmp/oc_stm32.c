/*
 * Copyright (c) 2020 Marek Porwisz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */

#define DT_DRV_COMPAT st_stm32_oc

#include <time.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <kernel.h>
#include <soc.h>
#include <drivers/output_cmp.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(output_cmp, CONFIG_OUTPUT_CMP_LOG_LEVEL);

#define OC_UNITS 4


struct oc_stm32_config {
	TIM_TypeDef *tim;
	u16_t prescaler;
	struct stm32_pclken pclken;
	void (*timer_stm32_irq_config)(void);
};

struct oc_stm32_data {
	struct device *clock;
	struct counter_cfg timer_cfg;
	struct counter_cfg compare_unit[OC_UNITS];
	TIM_HandleTypeDef tim_handle;
};


#define DEV_DATA(dev) ((struct oc_stm32_data *)(dev)->driver_data)
#define DEV_CFG(dev)	\
	((const struct oc_stm32_config * const)(dev)->config->config_info)
#define TIM_STRUCT(dev) ((TIM_TypeDef *)(DEV_CFG(dev))->tim)



static int oc_stm32_start(struct device *dev,
			  const struct counter_cfg * ovf_cfg)
{
	struct oc_stm32_data *data = DEV_DATA(dev);
	bool counter_32b;

	if (ovf_cfg->counter_val < 2U) {
		return -EINVAL;
	}

	if (!IS_TIM_INSTANCE(TIM_STRUCT(dev))) {
		return -ENOTSUP;
	}

#ifdef CONFIG_SOC_SERIES_STM32F1X
	counter_32b = 0;
#else
	counter_32b = IS_TIM_32B_COUNTER_INSTANCE(TIM_STRUCT(dev));
#endif

	if (!counter_32b && (ovf_cfg->counter_val > 0x10000)) {
		return -ENOTSUP;
	}

	/* Configure Timer IP */
	data->tim_handle.Instance = TIM_STRUCT(dev);
	data->tim_handle.Init.Prescaler = DEV_CFG(dev)->prescaler;
	data->tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	data->tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	data->tim_handle.Init.RepetitionCounter = 0;

	/* Set period value */
	data->tim_handle.Init.Period = ovf_cfg->counter_val - 1;

	if (HAL_TIM_OC_Init(&data->tim_handle) != HAL_OK) {
		return -EACCES;
	}

	if (HAL_TIM_Base_Start_IT(&data->tim_handle) != HAL_OK) {
		return -EACCES;
	}

	data->timer_cfg.ovf_match_callback = ovf_cfg->ovf_match_callback;
	data->timer_cfg.user_data = ovf_cfg->user_data;

	return 0;
}

static int oc_stm32_stop(struct device *dev)
{
	if (HAL_TIM_Base_Stop_IT(&DEV_DATA(dev)->tim_handle) != HAL_OK) {
		return -EACCES;
	}
	return 0;
}
static s32_t num_to_channel(u8_t chan_num)
{
	switch(chan_num) {
	case 0:
		return TIM_CHANNEL_1;
	case 1:
		return TIM_CHANNEL_2;
	case 2:
		return TIM_CHANNEL_3;
	case 3:
		return TIM_CHANNEL_4;
	default:
		return -1;
	}
}

static int oc_stm32_set_compare(struct device *dev, u8_t channel,
				const struct counter_cfg *match_cfg)
{
	struct oc_stm32_data *data = DEV_DATA(dev);

	if (channel >= OC_UNITS || match_cfg == NULL) {
		return -EINVAL;
	}
	if(match_cfg->ovf_match_callback == NULL) {
		HAL_TIM_OC_Stop_IT(&data->tim_handle, num_to_channel(channel));
		data->compare_unit[channel].ovf_match_callback = NULL;
		return 0;
	}

	if (match_cfg->counter_val > data->tim_handle.Init.Period) {
		return -EINVAL;
	}

	if (!IS_TIM_INSTANCE(TIM_STRUCT(dev))) {
		return -ENOTSUP;
	}

	__HAL_TIM_ENABLE_OCxPRELOAD(&data->tim_handle, num_to_channel(channel));
	__HAL_TIM_SET_COMPARE(&data->tim_handle, num_to_channel(channel), match_cfg->counter_val);

	data->compare_unit[channel].ovf_match_callback = match_cfg->ovf_match_callback;
	data->compare_unit[channel].user_data = match_cfg->user_data;


	if (HAL_TIM_OC_Start_IT(&data->tim_handle, num_to_channel(channel)) != HAL_OK) {
		return -EACCES;
	}

	return 0;
}

static int oc_stm32_update_compare(struct device *dev, u8_t channel,
				   u32_t match)
{
	struct oc_stm32_data *data = DEV_DATA(dev);

	if (match > data->tim_handle.Init.Period) {
		return -EINVAL;
	}

	__HAL_TIM_SET_COMPARE(&data->tim_handle, num_to_channel(channel), match);

	return 0;
}

static u32_t oc_stm32_get_counter(struct device *dev)
{
	return TIM_STRUCT(dev)->CNT;
}

void oc_stm32_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct oc_stm32_data *data = DEV_DATA(dev);
	const struct oc_stm32_config *cfg = DEV_CFG(dev);

	if (cfg->tim->SR & TIM_IT_UPDATE) {
		cfg->tim->SR = ~TIM_IT_UPDATE;
		if (data->timer_cfg.ovf_match_callback) {
			data->timer_cfg.ovf_match_callback(dev,
							data->tim_handle.Init.Period+1,
							data->timer_cfg.user_data);
		}
	}

	if (cfg->tim->SR & TIM_IT_CC1) {
		cfg->tim->SR = ~TIM_IT_CC1;
		if (data->compare_unit[0].ovf_match_callback) {
			data->compare_unit[0].ovf_match_callback(dev,
				data->tim_handle.Instance->CCR1,
				data->compare_unit[0].user_data);
		}
	}
	if (cfg->tim->SR & TIM_IT_CC2) {
		cfg->tim->SR = ~TIM_IT_CC2;
		if (data->compare_unit[1].ovf_match_callback) {
			data->compare_unit[1].ovf_match_callback(dev,
				data->tim_handle.Instance->CCR2,
				data->compare_unit[1].user_data);
		}
	}if (cfg->tim->SR & TIM_IT_CC3) {
		cfg->tim->SR = ~TIM_IT_CC3;
		if (data->compare_unit[2].ovf_match_callback) {
			data->compare_unit[2].ovf_match_callback(dev,
				data->tim_handle.Instance->CCR3,
				data->compare_unit[2].user_data);
		}
	}if (cfg->tim->SR & TIM_IT_CC4) {
		cfg->tim->SR = ~TIM_IT_CC4;
		if (data->compare_unit[3].ovf_match_callback) {
			data->compare_unit[3].ovf_match_callback(dev,
				data->tim_handle.Instance->CCR4,
				data->compare_unit[3].user_data);
		}
	}
}

static inline void __oc_stm32_get_clock(struct device *dev)
{
	struct oc_stm32_data *data = DEV_DATA(dev);
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

static int oc_stm32_init(struct device *dev)
{
	const struct oc_stm32_config *config = DEV_CFG(dev);
	struct oc_stm32_data *data = DEV_DATA(dev);

	__oc_stm32_get_clock(dev);

	/* enable clock */
	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	if(config->timer_stm32_irq_config) {
		config->timer_stm32_irq_config();
	}


	return 0;
}

static const struct output_cmp_driver_api api = {
	.start = oc_stm32_start,
	.stop = oc_stm32_stop,
	.set_cmp = oc_stm32_set_compare,
	.update_cmp = oc_stm32_update_compare,
	.get_counter = oc_stm32_get_counter,
};

#define CONNECT_IRQ(inst, id)						\
do {									\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, id, irq), 			\
		    DT_INST_IRQ_BY_IDX(inst, id, priority), 		\
		    oc_stm32_isr, 					\
		    &__device_oc_stm32_##inst,				\
		    0);							\
	irq_enable(DT_INST_IRQ_BY_IDX(inst, id, irq));	    		\
} while(false)


#define IRQ_CFG_FUN1(inst)						\
static void timer_stm32_irq_config_##inst(void)				\
{									\
	CONNECT_IRQ(inst, 0);						\
}

#define IRQ_CFG_FUN2(inst)						\
static void timer_stm32_irq_config_##inst(void)				\
{									\
	CONNECT_IRQ(inst, 0);						\
	CONNECT_IRQ(inst, 1);						\
}


#define CREATE_OC_DEV(inst)						\
									\
static void timer_stm32_irq_config_##inst(void);			\
									\
static struct oc_stm32_data timer_data_##inst = {			\
};									\
									\
static const struct oc_stm32_config timer_config_##inst = {		\
	.tim = (TIM_TypeDef*)DT_REG_ADDR(DT_INST_PHANDLE(inst, timer)), \
	.pclken = {							\
		.bus = DT_CLOCKS_CELL(DT_INST_PHANDLE(inst, timer), bus), \
		.enr = DT_CLOCKS_CELL(DT_INST_PHANDLE(inst, timer), bits), \
	},								\
	.prescaler = DT_INST_PROP(inst, st_prescaler),			\
	.timer_stm32_irq_config = timer_stm32_irq_config_##inst, 	\
};									\
									\
DEVICE_AND_API_INIT(oc_stm32_##inst, DT_INST_LABEL(inst), 		\
		    &oc_stm32_init, &timer_data_##inst, 		\
		    &timer_config_##inst, PRE_KERNEL_1, 		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);		\



#if DT_HAS_NODE(DT_DRV_INST(0))
CREATE_OC_DEV(0);
#if DT_INST_IRQ_HAS_IDX(0, 1)
IRQ_CFG_FUN2(0)
#else
IRQ_CFG_FUN1(0)
#endif
#endif


#if DT_HAS_NODE(DT_DRV_INST(1))
CREATE_OC_DEV(1);
#if DT_INST_IRQ_HAS_IDX(1, 1)
IRQ_CFG_FUN2(1)
#else
IRQ_CFG_FUN1(1)
#endif
#endif

#if DT_HAS_NODE(DT_DRV_INST(2))
CREATE_OC_DEV(2);
#if DT_INST_IRQ_HAS_IDX(2, 1)
IRQ_CFG_FUN2(2)
#else
IRQ_CFG_FUN1(2)
#endif
#endif

#if DT_HAS_NODE(DT_DRV_INST(3))
CREATE_OC_DEV(3);
#if DT_INST_IRQ_HAS_IDX(3, 1)
IRQ_CFG_FUN2(3)
#else
IRQ_CFG_FUN1(3)
#endif
#endif

#if DT_HAS_NODE(DT_DRV_INST(4))
CREATE_OC_DEV(4);
#if DT_INST_IRQ_HAS_IDX(4, 1)
IRQ_CFG_FUN2(4)
#else
IRQ_CFG_FUN1(4)
#endif
#endif

