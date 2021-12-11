// nrf
#include <hal/nrf_rtc.h>
#include <hal/nrf_wdt.h>
#include <legacy/nrf_drv_clock.h>
#include <libraries/gpiote/app_gpiote.h>
#include <libraries/timer/app_timer.h>
#include <softdevice/common/nrf_sdh.h>
#include <nrf_delay.h>
#include <libraries/log/nrf_log.h>
#include <libraries/log/nrf_log_ctrl.h>
#include <libraries/log/nrf_log_default_backends.h>


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
/* #include <drivers/Hrs3300.h> */
/* #include <drivers/Bma421.h> */

/* extern uint32_t __start_noinit_data; */
/* extern uint32_t __stop_noinit_data; */

/* uint32_t NoInit_MagicWord __attribute__((section(".noinit"))); */

void system_task(void *self) {
     NRF_LOG_INFO("systemtask task started! %s", self);
     for (;;) {
	  vTaskDelay(1000);
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

     // systemTasksMsgQueue = xQueueCreate(10, 1);
     if (pdPASS != xTaskCreate(system_task, "MAIN", 350, "main task", 0, &taskHandle)) {
	  APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
     }
     //     NRF_LOG_INFO("systemtask handle %p", taskHandle);
     vTaskStartScheduler();

     for (;;) {
	  APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
     }
}