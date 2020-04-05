# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OC_STM32     oc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE            oc_handlers.c)
