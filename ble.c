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

struct ble_hs_cfg;

static int gatt_svr_init(void);
static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);


#define NEED(c) configASSERT((c)==0)


void nimble_port_init(void) {
  void os_msys_init(void);
  void ble_store_ram_init(void);

  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;

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

  NEED(gatt_svr_init());
  nimble_port_freertos_init(BleHost);
}

void nimble_port_ll_task_func(void* args) {
  extern void ble_ll_task(void*);
  ble_ll_task(args);
}


/*********************************************/

/* everything above this was basically copied from infinitime */

/* a lot of the stuff below was copied from nimble
 * apps/bleprph/src/main.c
 */

#define BLE_APPEARANCE_WATCH_SPORTS_WATCH 193
static uint8_t address_type;

void ble_init() {


    NRF_LOG_INFO("waiting ble hs sync");
    while(!ble_hs_synced()) {
	vTaskDelay(1000);
    }

    NRF_LOG_INFO("done ble hs sync %d",BLE_HS_ENOADDR );

    ble_svc_gap_init();
    ble_svc_gatt_init();

    NEED(ble_hs_util_ensure_addr(1));
    NEED(ble_hs_id_infer_auto(0, &address_type))

    NEED(ble_svc_gap_device_name_set("chablon"));
    NEED(ble_svc_gap_device_appearance_set(BLE_APPEARANCE_WATCH_SPORTS_WATCH));

}

static void
bleprph_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    NRF_LOG_INFO("handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    // print_addr(desc->our_ota_addr.val);
    NRF_LOG_INFO(" our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    // print_addr(desc->our_id_addr.val);
    NRF_LOG_INFO(" peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    // print_addr(desc->peer_ota_addr.val);
    NRF_LOG_INFO(" peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    // print_addr(desc->peer_id_addr.val);
    NRF_LOG_INFO(" conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

void ble_start_advertising();

static int gap_event_cb(struct ble_gap_event* event, void* arg) {
    NRF_LOG_INFO("gap event %p", event);
    struct ble_gap_conn_desc desc;

    NRF_LOG_INFO("event type %d", event->type);

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* new connection established, or connection attempt failed */
        NRF_LOG_INFO("connection %s; status=%d ",
		     event->connect.status == 0 ? "established" : "failed",
		     event->connect.status);
        if (event->connect.status == 0) { /* success */
            NEED(ble_gap_conn_find(event->connect.conn_handle, &desc));
            bleprph_print_conn_desc(&desc);
	} else {
	    ble_start_advertising();
	}
	break;
    case BLE_GAP_EVENT_DISCONNECT:
	bleprph_print_conn_desc(&event->disconnect.conn);
	ble_start_advertising();
	break;
    case BLE_GAP_EVENT_CONN_UPDATE:
	NEED(ble_gap_conn_find(event->conn_update.conn_handle, &desc));
        bleprph_print_conn_desc(&desc);
	break;
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        NRF_LOG_INFO("conn update req");
	break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        NRF_LOG_INFO("advertise complete; reason=%d",
		     event->adv_complete.reason);
        ble_start_advertising();
        break;
    default:
        NRF_LOG_INFO("unhandled event in advertising %d",
		     event->type);
    }
    NRF_LOG_INFO("gap cb returns");
    return 0;
}


#define GATT_SVR_SVC_ALERT_UUID               0x1811

void ble_start_advertising() {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;

    memset(&adv_params, 0, sizeof(adv_params));
    memset(&fields, 0, sizeof(fields));
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    /* undirected connectable */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    /* general discoverable */
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 0;	/* use stack defaults */
    adv_params.itvl_max = 0;	/*  */

    /* } else { */
    /* 	adv_params.itvl_min = 1636; */
    /* 	adv_params.itvl_max = 1651; */
    /* } */

    /* discovery general, ble-only */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    /* advertise "alert notifications" service - whatever that is */
    fields.uuids16 = (ble_uuid16_t[]){
        BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
    };
    fields.num_uuids16 = 1;

    rsp_fields.name = (const uint8_t *) "chablon";
    rsp_fields.name_len = strlen("chablon");
    rsp_fields.name_is_complete = 1;

    NEED(ble_gap_adv_set_fields(&fields));
    NEED(ble_gap_adv_rsp_set_fields(&rsp_fields));
    NEED(ble_gap_adv_start(address_type, NULL, 60*1000, &adv_params, gap_event_cb, NULL));
}

#if 0
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp/bsp.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "bleprph.h"
#endif

/**
 * The vendor specific security test service consists of two characteristics:
 *     o random-number-generator: generates a random 32-bit number each time
 *       it is read.  This characteristic can only be read over an encrypted
 *       connection.
 *     o static-value: a single-byte characteristic that can always be read,
 *       but can only be written over an encrypted connection.
 */

/* 59462f12-9543-9999-12c8-58b459a2712d */
static const ble_uuid128_t gatt_svr_svc_sec_test_uuid =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                     0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

/* 5c3a659e-897e-45e1-b016-007107c96df6 */
static const ble_uuid128_t gatt_svr_chr_sec_test_rand_uuid =
        BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t gatt_svr_chr_sec_test_static_uuid =
        BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

static uint8_t gatt_svr_sec_test_static_val = 42;

static int
gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: Security test. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_sec_test_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /*** Characteristic: Random number generator. */
            .uuid = &gatt_svr_chr_sec_test_rand_uuid.u,
            .access_cb = gatt_svr_chr_access_sec_test,
	    // if I include READ_ENC here then any attempt
	    // to read is rejected, even if READ and READ_ENC
	    // are both present
            .flags = BLE_GATT_CHR_F_READ,// | BLE_GATT_CHR_F_READ_ENC,
        }, {
            /*** Characteristic: Static value. */
            .uuid = &gatt_svr_chr_sec_test_static_uuid.u,
            .access_cb = gatt_svr_chr_access_sec_test,
	    // here similarly, including WRITE_ENC causes
	    // plaintext writes to be rejected
            .flags = BLE_GATT_CHR_F_READ |
	    BLE_GATT_CHR_F_WRITE,// | BLE_GATT_CHR_F_WRITE_ENC,
        }, {
            0, /* No more characteristics in this service. */
        } },
    },

    {
        0, /* No more services. */
    },
};

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

static int
gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg)
{
    const ble_uuid_t *uuid;
    int rand_num;
    int rc;

    uuid = ctxt->chr->uuid;

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_test_rand_uuid.u) == 0) {
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

        /* Respond with a 32-bit random number. */
        rand_num = rand();
        rc = os_mbuf_append(ctxt->om, &rand_num, sizeof rand_num);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_test_static_uuid.u) == 0) {
        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &gatt_svr_sec_test_static_val,
                                sizeof gatt_svr_sec_test_static_val);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = gatt_svr_chr_write(ctxt->om,
                                    sizeof gatt_svr_sec_test_static_val,
                                    sizeof gatt_svr_sec_test_static_val,
                                    &gatt_svr_sec_test_static_val, NULL);
            return rc;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        NRF_LOG_INFO("registered service %s with handle=%d\n",
		     ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
		     ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        NRF_LOG_INFO("registering characteristic %s with "
                           "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        NRF_LOG_INFO("registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

static int
gatt_svr_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
