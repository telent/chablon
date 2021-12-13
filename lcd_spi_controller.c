#include "nrfx_spim.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"

#include <stdlib.h>

static const nrfx_spim_t spi =
    NRFX_SPIM_INSTANCE(ST7735_SPI_INSTANCE);

void hw_reset() {
    nrf_gpio_cfg_output(26);
    nrf_gpio_pin_set(26);
    nrf_gpio_pin_clear(26);
    nrf_delay_ms(200);
    nrf_gpio_pin_set(26);
}

static void spi_init() {
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    spi_config.frequency      = NRF_SPIM_FREQ_8M;
    spi_config.mode           = ST7735_SPI_MODE;
    spi_config.ss_pin         = ST7735_SS_PIN;
    spi_config.miso_pin       = ST7735_MISO_PIN;
    spi_config.mosi_pin       = ST7735_MOSI_PIN;
    spi_config.sck_pin        = ST7735_SCK_PIN;
    spi_config.ss_active_high = false;

    hw_reset() ;

    /* don't know if we need this or if nrfx_spim_init
     * does it for us */

    nrf_gpio_cfg_output(ST7735_SCK_PIN);
    nrf_gpio_pin_set(ST7735_SCK_PIN);

    nrf_gpio_cfg_output(ST7735_MOSI_PIN);
    nrf_gpio_pin_clear(ST7735_MOSI_PIN);

    nrf_gpio_cfg_input(ST7735_MISO_PIN, NRF_GPIO_PIN_NOPULL);

    nrf_gpio_cfg_output(ST7735_DC_PIN);
    nrf_gpio_pin_clear(ST7735_DC_PIN);

    nrf_gpio_cfg_output(ST7735_SS_PIN);
    nrf_gpio_pin_set(ST7735_SS_PIN);

    APP_ERROR_CHECK(nrfx_spim_init(&spi, &spi_config, NULL,  NULL));

    NRF_LOG_INFO("LCD SPI initialized");
}


void write_spi_fn(const uint8_t *tx_bytes,
			 int tx_count,
			 int is_command) {

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(tx_bytes, tx_count);
    nrf_gpio_pin_write(ST7735_DC_PIN, ! is_command);
    APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &xfer_desc, NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER));

   volatile int * finished = (int *)nrfx_spim_end_event_get(&spi);
    while (!*finished) {
      __WFE();
    }

}

#define PANEL_WIDTH 240
#define PANEL_HEIGHT 320 	/* really? */

#define NOP 0x00
#define SWRESET 0x01
#define RDDID 0x04
#define RDDST 0x09
#define SLPIN 0x10
#define SLPOUT 0x11
#define PTLON 0x12
#define NORON 0x13
#define INVOFF 0x20
#define INVON 0x21
#define DISPOFF 0x28
#define DISPON 0x29
#define CASET 0x2A
#define RASET 0x2B
#define RAMWR 0x2C
#define RAMRD 0x2E
#define PTLAR 0x30
#define COLMOD 0x3A
#define WRMEMC 0x3c
#define MADCTL 0x36
#define FRMCTR1 0xB1
#define FRMCTR2 0xB2
#define FRMCTR3 0xB3
#define INVCTR 0xB4
#define DISSET5 0xB6
#define PWCTR1 0xC0
#define PWCTR2 0xC1
#define PWCTR3 0xC2
#define PWCTR4 0xC3
#define PWCTR5 0xC4
#define VMCTR1 0xC5
#define RDID1 0xDA
#define RDID2 0xDB
#define RDID3 0xDC
#define RDID4 0xDD
#define PWCTR6 0xFC
#define GMCTRP1 0xE0
#define GMCTRN1 0xE1

#define UB(...) ((const uint8_t[]) { __VA_ARGS__ })
#define SPI_COMMAND(c)  write_spi_fn(UB(c), 1, 1);
#define SPI_DATA(...)  write_spi_fn(UB(__VA_ARGS__), sizeof UB(__VA_ARGS__), 0);

#define BACKLIGHT_HIGH 23
#define BACKLIGHT_MEDIUM 22
#define BACKLIGHT_LOW 14



static void set_brightness(int level) {
    nrf_gpio_pin_write(BACKLIGHT_HIGH, !(level >= 3));
    nrf_gpio_pin_write(BACKLIGHT_MEDIUM, !(level >= 2));
    nrf_gpio_pin_write(BACKLIGHT_LOW, !(level >= 1));
}


void lcd_spi_controller_init(void)
{
    spi_init();

    nrf_gpio_cfg_output(BACKLIGHT_HIGH);
    nrf_gpio_cfg_output(BACKLIGHT_MEDIUM);
    nrf_gpio_cfg_output(BACKLIGHT_LOW);

    set_brightness(3);


    SPI_COMMAND(SWRESET);
    nrf_delay_ms(150 * 3);

    SPI_COMMAND(SLPOUT);
    nrf_delay_ms(150 * 3);

    SPI_COMMAND(COLMOD);  SPI_DATA(0b01010101);
    nrf_delay_ms(10);

    SPI_COMMAND(MADCTL);  SPI_DATA(0x00);

    SPI_COMMAND(CASET);
    SPI_DATA(0x00, 0x00, (PANEL_WIDTH >> 8u), PANEL_WIDTH & 0xffu);

    SPI_COMMAND(RASET);
    SPI_DATA(0x00, 0x00, (PANEL_HEIGHT >> 8u), PANEL_HEIGHT & 0xffu);

    SPI_COMMAND(INVON);
    nrf_delay_ms(10);

    SPI_COMMAND(NORON);
    nrf_delay_ms(10);

    SPI_COMMAND(DISPON);

}

uint16_t buf[240];

/* this is almost certainly full of off-by-one errors */
void lcd_fill_rect(int x, int y, int w, int h, uint16_t colour) {
    for(int i=0; i < w; i++)
	buf[i]=colour;

    SPI_COMMAND(CASET);
    SPI_DATA(0, x, 0, x + w - 1);

    for(int i = y; i < y+h-1; i++) {
	SPI_COMMAND(RASET);
	SPI_DATA(0, i, 0, i+1);

	SPI_COMMAND(RAMWR);
	/* can't do this in one operation - speculating there's */
	/* a 256 byte limit on polled SPI */
	write_spi_fn((uint8_t *) buf, w, 0);
	write_spi_fn((uint8_t *) buf, w, 0);
    }
}

#define RGB(r,g,b) (b | (g << 5) | (r << (5 + 6)))

void lcd_write_junk() {
    const uint16_t cols[] = {
	RGB( 0,  0,  0),
	RGB(31,  0,  0),
	RGB( 0, 63,  0),
	RGB(31, 63,  0),
	RGB( 0,  0, 31),
	RGB(31,  0, 31),
	RGB( 0, 63, 31),
	RGB(31, 63, 31),
    };
    lcd_fill_rect(0, 0, 240, 240, 0x0);
    for(int i=0; i < 8; i++) {
	lcd_fill_rect(i*20, (9-i)*20, 15, 20, cols[i]);
    }
}
