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

// static u32_t __get_tim_clk(u32_t bus_clk, clock_control_subsys_t *sub_system)
// {
// 	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
// 	u32_t tim_clk;
// 	u32_t apb_psc = 0;

// #if defined(CONFIG_SOC_SERIES_STM32H7X)
// 	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
// 		apb_psc = CONFIG_CLOCK_STM32_D2PPRE1;
// 	} else {
// 		apb_psc = CONFIG_CLOCK_STM32_D2PPRE2;
// 	}

// 	/*
// 	 * Depending on pre-scaler selection (TIMPRE), timer clock frequency
// 	 * is defined as follows:
// 	 *
// 	 * - TIMPRE=0: If the APB prescaler (PPRE1, PPRE2) is configured to a
// 	 *   division factor of 1 then the timer clock equals to APB bus clock.
// 	 *   Otherwise the timer clock is set to twice the frequency of APB bus
// 	 *   clock.
// 	 * - TIMPRE=1: If the APB prescaler (PPRE1, PPRE2) is configured to a
// 	 *   division factor of 1, 2 or 4, then the timer clock equals to HCLK.
// 	 *   Otherwise, the timer clock frequencies are set to four times to
// 	 *   the frequency of the APB domain.
// 	 */
// 	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
// 		if (apb_psc == 1U) {
// 			tim_clk = bus_clk;
// 		} else {
// 			tim_clk = bus_clk * 2U;
// 		}
// 	} else {
// 		if (apb_psc == 1U || apb_psc == 2U || apb_psc == 4U) {
// 			tim_clk = SystemCoreClock;
// 		} else {
// 			tim_clk = bus_clk * 4U;
// 		}
// 	}
// #else
// 	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
// 		apb_psc = CONFIG_CLOCK_STM32_APB1_PRESCALER;
// 	}
// #if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
// 	else {
// 		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
// 	}
// #endif

// 	/*
// 	 * If the APB prescaler equals 1, the timer clock frequencies
// 	 * are set to the same frequency as that of the APB domain.
// 	 * Otherwise, they are set to twice (×2) the frequency of the
// 	 * APB domain.
// 	 */
// 	if (apb_psc == 1U) {
// 		tim_clk = bus_clk;
// 	} else	{
// 		tim_clk = bus_clk * 2U;
// 	}
// #endif

// 	return tim_clk;
// }


//static void timer_stm32_irq_config(struct device *dev);

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
	/* FIXME: IS_TIM_32B_COUNTER_INSTANCE not available on
	 * SMT32F1 Cube HAL since all timer counters are 16 bits
	 */
	counter_32b = 0;
#else
	counter_32b = IS_TIM_32B_COUNTER_INSTANCE(TIM_STRUCT(dev));
#endif

	/*
	 * The timer counts from 0 up to the value in the ARR register (16-bit).
	 * Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!counter_32b && (ovf_cfg->counter_val > 0x10000)) {
		/* 16 bits counter does not support requested period
		 * You might want to adapt PWM output clock to adjust
		 * cycle durations to fit requested period into 16 bits
		 * counter
		 */
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

	if (HAL_TIM_Base_Init(&data->tim_handle) != HAL_OK) {
		return -EACCES;
	}
	if (HAL_TIM_Base_Start_IT(&data->tim_handle) != HAL_OK) {
		return -EACCES;
	}

	return 0;
}

static int oc_stm32_stop(struct device *dev)
{
	if (HAL_TIM_Base_Stop_IT(&DEV_DATA(dev)->tim_handle) != HAL_OK) {
		return -EACCES;
	}
	return 0;
}

static int oc_stm32_set_compare(struct device *dev, u8_t channel,
				const struct counter_cfg *match_cfg)
{
	return 0;
}

static int oc_stm32_update_compare(struct device *dev, u8_t channel,
				   u32_t match)
{
	return 0;
}

static u32_t oc_stm32_get_counter(struct device *dev)
{
	return TIM_STRUCT(dev)->CNT;
}
// static int rtc_stm32_set_alarm(struct device *dev,
// 				const struct counter_alarm_cfg *alarm_cfg)
// {
// 	return 0;
// }
// HAL_StatusTypeDef HAL_TIM_OC_Init(TIM_HandleTypeDef *htim);
// HAL_StatusTypeDef HAL_TIM_OC_DeInit(TIM_HandleTypeDef *htim);
// void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim);
// void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim);
// /* Blocking mode: Polling */
// HAL_StatusTypeDef HAL_TIM_OC_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
// HAL_StatusTypeDef HAL_TIM_OC_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
// /* Non-Blocking mode: Interrupt */
// HAL_StatusTypeDef HAL_TIM_OC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
// HAL_StatusTypeDef HAL_TIM_OC_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);

void oc_stm32_isr(void *arg)
{

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

	return 0;
}

static const struct output_cmp_driver_api api = {
	.start = oc_stm32_start,
	.stop = oc_stm32_stop,
	.set_cmp = oc_stm32_set_compare,
	.update_cmp = oc_stm32_update_compare,
	.get_counter = oc_stm32_get_counter,
};


// static void stm32_prec_irq_config(struct device *dev)
// {
// 	//IRQ_CONNECT(DT_INST_IRQN(0),
// 	//	    DT_INST_IRQ(0, priority),
// 	//	    rtc_stm32_isr, DEVICE_GET(rtc_stm32), 0);
// 	//irq_enable(DT_INST_IRQN(0));
// }


#define CREATE_OC_DEV(inst)						\
static struct oc_stm32_data timer_data_##inst = {			\
};									\
									\
static const struct oc_stm32_config timer_config_##inst = {		\
	.tim = (TIM_TypeDef*)DT_INST_##inst##_ST_STM32_TIMERS_BASE_ADDRESS, \
	.pclken = {							\
		.bus = DT_INST_##inst##_ST_STM32_TIMERS_CLOCK_BUS,	\
		.enr = DT_INST_##inst##_ST_STM32_TIMERS_CLOCK_BITS	\
	},								\
};									\
									\
DEVICE_AND_API_INIT(prec_timer_##inst, DT_INST_LABEL(inst), 		\
		    &oc_stm32_init, &timer_data_##inst, 		\
		    &timer_config_##inst, PRE_KERNEL_1, 		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);

#if DT_HAS_NODE(DT_DRV_INST(0))
CREATE_OC_DEV(0);
#endif

#if DT_HAS_NODE(DT_DRV_INST(1))
CREATE_OC_DEV(1);
#endif

#if DT_HAS_NODE(DT_DRV_INST(2))
CREATE_OC_DEV(2);
#endif

#if DT_HAS_NODE(DT_DRV_INST(3))
CREATE_OC_DEV(3);
#endif

#if DT_HAS_NODE(DT_DRV_INST(4))
CREATE_OC_DEV(4);
#endif

#if DT_HAS_NODE(DT_DRV_INST(5))
CREATE_OC_DEV(5);
#endif

#if DT_HAS_NODE(DT_DRV_INST(6))
CREATE_OC_DEV(6);
#endif

#if DT_HAS_NODE(DT_DRV_INST(7))
CREATE_OC_DEV(7);
#endif
