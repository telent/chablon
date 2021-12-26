/* nimble includes */
#include <controller/ble_ll.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nimble/npl_freertos.h>
#include <os/os_cputime.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <transport/ram/ble_hci_ram.h>

#include "nrf_log.h"
#include "nrf_delay.h"


/* interrupt handlers required for NimBLE radio driver */

static void (*radio_isr_addr)(void);
static void (*rng_isr_addr)(void);
static void (*rtc0_isr_addr)(void);

void RADIO_IRQHandler(void) {
  ((void (*)(void)) radio_isr_addr)();
}

void RNG_IRQHandler(void) {
  ((void (*)(void)) rng_isr_addr)();
}

void RTC0_IRQHandler(void) {
  ((void (*)(void)) rtc0_isr_addr)();
}


void npl_freertos_hw_set_isr(int irqn, void (*addr)(void)) {
    NRF_LOG_INFO("set_isr %d of %d, %d, %d",
		 irqn, RADIO_IRQn, RNG_IRQn, RTC0_IRQn);
    switch (irqn) {
    case RADIO_IRQn:
	radio_isr_addr = addr;
	break;
    case RNG_IRQn:
	rng_isr_addr = addr;
	break;
    case RTC0_IRQn:
	rtc0_isr_addr = addr;
	break;
    }
}

uint32_t npl_freertos_hw_enter_critical(void) {
  uint32_t ctx = __get_PRIMASK();
  __disable_irq();
  return (ctx & 0x01);
}

void npl_freertos_hw_exit_critical(uint32_t ctx) {
  if (!ctx) {
    __enable_irq();
  }
}

static struct ble_npl_eventq g_eventq_dflt;

struct ble_npl_eventq* nimble_port_get_dflt_eventq(void) {
  return &g_eventq_dflt;
}

void nimble_port_run(void) {
    struct ble_npl_event* ev;

    while (1) {
	ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
	NRF_LOG_INFO("ble event");
	ble_npl_event_run(ev);
    }
}

void BleHost(void* _unused) {
  nimble_port_run();
}

void static_assert(int thing) {
    ASSERT(thing);
}

void nimble_port_init(void) {
  void os_msys_init(void);
  void ble_store_ram_init(void);

  ble_npl_eventq_init(&g_eventq_dflt);
  os_msys_init();
  ble_hs_init();
  ble_store_ram_init();

  int res;

  res = hal_timer_init(5, NULL);
  ASSERT(res == 0);
  res = os_cputime_init(32768);
  ASSERT(res == 0);

  ble_ll_init();
  ble_hci_ram_init();

  nimble_port_freertos_init(BleHost);
}

void nimble_port_ll_task_func(void* args) {
  extern void ble_ll_task(void*);
  ble_ll_task(args);
}


/*********************************************/

/* everything above this was basically copied from infinitime */

#define NEED(c) configASSERT((c)==0)

void ble_init() {
    NRF_LOG_INFO("waiting ble hs sync");
    while(!ble_hs_synced()) {
	vTaskDelay(1000);
    }

    NRF_LOG_INFO("done ble hs sync %d",BLE_HS_ENOADDR );

    ble_svc_gap_init();
    ble_svc_gatt_init();

    NEED(ble_hs_util_ensure_addr(1));
    NEED(ble_svc_gap_device_name_set("chablon"));
    NEED(ble_gatts_start());

}
