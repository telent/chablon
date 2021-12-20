#include <chablon.h>
#include <lua.h>

#include "lua_state.h"

// nrf
#include <hal/nrf_rtc.h>
#include <hal/nrf_wdt.h>
#include <legacy/nrf_drv_clock.h>
#include <libraries/timer/app_timer.h>
#include <softdevice/common/nrf_sdh.h>
#include <nrf_delay.h>
#include <libraries/log/nrf_log.h>
#include <libraries/log/nrf_log_ctrl.h>
#include <libraries/log/nrf_log_default_backends.h>
#include "nrf_gpio.h"


#if 0
// nimble
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <controller/ble_ll.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nimble/npl_freertos.h>
#include <os/os_cputime.h>
#include <services/gap/ble_svc_gap.h>
#include <transport/ram/ble_hci_ram.h>
#undef max
#undef min
#endif

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

extern void lua_hello();

void system_task(void *self) {
     NRF_LOG_INFO("systemtask task started! %s", self);
     nrf_gpio_cfg_output(5);	/* spi flash csn */
     nrf_gpio_pin_set(5);	/* spi flash csn */

     lua_hello();
     NRF_LOG_FLUSH();

     /* lcd_spi_controller_init(); */
     /* lcd_write_junk(); */
     for (;;) {
	  vTaskDelay(3000);
	  NRF_LOG_INFO("systemtask task running");
     }
}


int main(void) {
     TaskHandle_t taskHandle;

     int result = NRF_LOG_INIT(NULL);
     APP_ERROR_CHECK(result);
     NRF_LOG_DEFAULT_BACKENDS_INIT();

     nrf_drv_clock_init();
     NRF_LOG_INFO("start the clock!");

     if (pdPASS != xTaskCreate(system_task, "MAIN", 3500,
			       "main task",configMAX_PRIORITIES - 1,
			       &taskHandle)) {
	  APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
     }
     //     NRF_LOG_INFO("systemtask handle %p", taskHandle);
     vTaskStartScheduler();

     for (;;) {
	  APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
     }
}
