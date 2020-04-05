/*
 * Copyright (c) 2018 Workaround GmbH
 * Copyright (c) 2018 Allterco Robotics
 * Copyright (c) 2018 Linaro Limited
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


struct oc_stm32_config {
	LL_TIM_InitTypeDef timer_config;
	TIM_TypeDef *tim;
	u16_t prescaler;
	struct stm32_pclken pclken;
};

struct oc_stm32_data {
	struct device *clock;
	ovf_callback_t callback;
	void *user_data;
};


#define DEV_DATA(dev) ((struct oc_stm32_data *)(dev)->driver_data)
#define DEV_CFG(dev)	\
((const struct oc_stm32_config * const)(dev)->config->config_info)


//static void timer_stm32_irq_config(struct device *dev);


static int oc_stm32_start(struct device *dev)
{
	// ARG_UNUSED(dev);
	// LL_TIM_InitTypeDef *cfg = &(DEV_CFG(dev)->timer_config);
	// LL_TIM_StructInit(cfg);
	// cfg->ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	// cfg->Prescaler = pre;
	// cfg->Autoreload = max;
	// cfg->CounterMode = LL_TIM_COUNTERMODE_UP;
	// if (LL_TIM_Init(DEV_CFG(dev)->TIMx, cfg)) != SUCCESS) {
	// 	return -1;
	// }

	return 0;
}


static int oc_stm32_stop(struct device *dev)
{
	// ARG_UNUSED(dev);
	// LL_TIM_DeInit();
	// LL_RCC_DisableRTC();

	return 0;
}

// static int rtc_stm32_set_alarm(struct device *dev,
// 				const struct counter_alarm_cfg *alarm_cfg)
// {
// 	return 0;
// }

void oc_stm32_isr(void *arg)
{
	// struct device *const dev = (struct device *)arg;
	// struct rtc_stm32_data *data = DEV_DATA(dev);
	// counter_alarm_callback_t alarm_callback = data->callback;

	// u32_t now = rtc_stm32_read(dev);

	// if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {

	// 	LL_RTC_DisableWriteProtection(RTC);
	// 	LL_RTC_ClearFlag_ALRA(RTC);
	// 	LL_RTC_DisableIT_ALRA(RTC);
	// 	LL_RTC_ALMA_Disable(RTC);
	// 	LL_RTC_EnableWriteProtection(RTC);

	// 	if (alarm_callback != NULL) {
	// 		data->callback = NULL;
	// 		alarm_callback(dev, 0, now, data->user_data);
	// 	}
	// }

	// LL_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
}


static int oc_stm32_init(struct device *dev)
{
	const struct oc_stm32_config *cfg = DEV_CFG(dev);
	struct oc_stm32_data *data = DEV_DATA(dev);
	LL_TIM_InitTypeDef timer_config;

	data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(data->clock);

	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&cfg->pclken) != 0) {
		return -EIO;
	}

	LL_TIM_StructInit(&timer_config);
	timer_config.Prescaler = cfg->prescaler;

	if (LL_TIM_Init(cfg->tim, &timer_config) != SUCCESS) {
		return -EACCES;
	}


// 	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

// 	__ASSERT_NO_MSG(clk);

// 	DEV_DATA(dev)->callback = NULL;

// 	clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken);

// 	LL_PWR_EnableBkUpAccess();
// 	LL_RCC_ForceBackupDomainReset();
// 	LL_RCC_ReleaseBackupDomainReset();

// #if defined(CONFIG_COUNTER_RTC_STM32_CLOCK_LSI)

// #if defined(CONFIG_SOC_SERIES_STM32WBX)
// 	LL_RCC_LSI1_Enable();
// 	while (LL_RCC_LSI1_IsReady() != 1) {
// 	}
// #else
// 	LL_RCC_LSI_Enable();
// 	while (LL_RCC_LSI_IsReady() != 1) {
// 	}
// #endif /* CONFIG_SOC_SERIES_STM32WBX */

// 	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

// #else /* CONFIG_COUNTER_RTC_STM32_CLOCK_LSE */

 #if !defined(CONFIG_SOC_SERIES_STM32F4X) &&	\
 	!defined(CONFIG_SOC_SERIES_STM32F2X) && \
 	!defined(CONFIG_SOC_SERIES_STM32L1X)

// 	LL_RCC_LSE_SetDriveCapability(
// 		CONFIG_COUNTER_RTC_STM32_LSE_DRIVE_STRENGTH);

 #endif /*
	* !CONFIG_SOC_SERIES_STM32F4X
	* && !CONFIG_SOC_SERIES_STM32F2X
	* && !CONFIG_SOC_SERIES_STM32L1X
	*/

// #if defined(CONFIG_COUNTER_RTC_STM32_LSE_BYPASS)
// 	LL_RCC_LSE_EnableBypass();
// #endif /* CONFIG_COUNTER_RTC_STM32_LSE_BYPASS */

// 	LL_RCC_LSE_Enable();

// 	/* Wait until LSE is ready */
// 	while (LL_RCC_LSE_IsReady() != 1) {
// 	}

// 	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

// #endif /* CONFIG_COUNTER_RTC_STM32_CLOCK_SRC */

// 	LL_RCC_EnableRTC();

// 	if (LL_RTC_DeInit(RTC) != SUCCESS) {
// 		return -EIO;
// 	}

// 	if (LL_RTC_Init(RTC, ((LL_RTC_InitTypeDef *)
// 			      &cfg->ll_rtc_config)) != SUCCESS) {
// 		return -EIO;
// 	}

// #ifdef RTC_CR_BYPSHAD
// 	LL_RTC_DisableWriteProtection(RTC);
// 	LL_RTC_EnableShadowRegBypass(RTC);
// 	LL_RTC_EnableWriteProtection(RTC);
// #endif /* RTC_CR_BYPSHAD */

// 	LL_EXTI_EnableIT_0_31(RTC_EXTI_LINE);
// 	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);

// 	rtc_stm32_irq_config(dev);

	return 0;
}

static const struct oc_driver_api api = {
		.start = oc_stm32_start,
		.stop = oc_stm32_stop,
		.set_ovf = NULL
};


// static void stm32_prec_irq_config(struct device *dev)
// {
// 	//IRQ_CONNECT(DT_INST_IRQN(0),
// 	//	    DT_INST_IRQ(0, priority),
// 	//	    rtc_stm32_isr, DEVICE_GET(rtc_stm32), 0);
// 	//irq_enable(DT_INST_IRQN(0));
// }
DT_INST_BUS_LABEL(inst)

#define CREATE_OC_DEV(inst)						\
static struct oc_stm32_data timer_data_##inst = {			\
};									\
									\
static const struct oc_stm32_config timer_config_##inst = {		\
	.tim = DT_INST_##index##_ST_STM32_TIMERS_BASE_ADDRESS,		\
	.pclken = {							\
		.bus = DT_INST_##index##_ST_STM32_TIMERS_CLOCK_BUS,	\
		.enr = DT_INST_##index##_ST_STM32_TIMERS_CLOCK_BITS	\
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
