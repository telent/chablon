default: chablon.elf

ifndef ARM_NONE_EABI_TOOLCHAIN_PATH
$(error Missing required parameter ARM_NONE_EABI_TOOLCHAIN_PATH)
endif
ifndef NRF5_SDK_PATH
$(error Missing required parameter NRF5_SDK_PATH)
endif
ifndef LUA_PATH
$(error Missing required parameter LUA_PATH)
endif

# logging adds about 1.3k to the RAM region
LOGGING=-DNRF_LOG_BACKEND_RTT_ENABLED=1 -DNRF_LOG_ENABLED=1 -DNRF_LOG_DEFERRED=0

SYSROOT=--sysroot=$(ARM_NONE_EABI_TOOLCHAIN_PATH)/bin


DEFS=-DBOARD_PCA10040 \
 -DCONFIG_GPIO_AS_PINRESET \
 -DFREERTOS \
 -DNIMBLE_CFG_CONTROLLER \
 -DNRF52 -DNRF52832 -DNRF52832_XXAA \
 -DNRF52_PAN_12 -DNRF52_PAN_15 -DNRF52_PAN_20 -DNRF52_PAN_31 -DNRF52_PAN_36 \
 -DNRF52_PAN_51 -DNRF52_PAN_54 -DNRF52_PAN_55 -DNRF52_PAN_58 -DNRF52_PAN_64 \
 -DNRF52_PAN_74 \
 -DOS_CPUTIME_FREQ  -D__HEAP_SIZE=4096 -D__STACK_SIZE=1024 \
 -DNRFX_SPIM_ENABLED=1 -DNRFX_SPIM0_ENABLED=1 \
 -DDEBUG -DDEBUG_NRF_USER $(LOGGING)

# this is to skip reading "apply_old_config.h", which overrides our
# config settings in unpredictable ways
DEFS+=-DAPPLY_OLD_CONFIG_H__=1



NRF5_SDK_SOURCE_FILES= \
        modules/nrfx/mdk/system_nrf52.c \
        components/boards/boards.c \
        integration/nrfx/legacy/nrf_drv_clock.c \
        modules/nrfx/drivers/src/nrfx_clock.c \
        modules/nrfx/drivers/src/nrfx_spim.c \
        modules/nrfx/soc/nrfx_atomic.c \
        modules/nrfx/drivers/src/nrfx_saadc.c \
        external/freertos/source/croutine.c \
        external/freertos/source/event_groups.c \
        external/freertos/source/portable/MemMang/heap_4.c \
        external/freertos/source/list.c \
        external/freertos/source/queue.c \
        external/freertos/source/stream_buffer.c \
        external/freertos/source/tasks.c \
        external/freertos/source/timers.c \
        components/libraries/timer/app_timer_freertos.c \
        components/libraries/atomic/nrf_atomic.c \
        components/libraries/balloc/nrf_balloc.c \
        components/libraries/util/nrf_assert.c \
        components/libraries/util/app_error.c \
        components/libraries/util/app_error_weak.c \
        components/libraries/util/app_error_handler_gcc.c \
        components/libraries/util/app_util_platform.c \
        components/libraries/memobj/nrf_memobj.c \
        components/libraries/ringbuf/nrf_ringbuf.c \
        components/libraries/strerror/nrf_strerror.c \
	external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
	external/segger_rtt/SEGGER_RTT.c \
	external/segger_rtt/SEGGER_RTT_printf.c \
        external/utf_converter/utf.c \
        external/fprintf/nrf_fprintf.c \
        external/fprintf/nrf_fprintf_format.c \
        modules/nrfx/drivers/src/nrfx_twim.c

ifdef LOGGING
NRF5_SDK_SOURCE_FILES+= \
	components/libraries/log/src/nrf_log_backend_rtt.c \
        components/libraries/log/src/nrf_log_backend_serial.c \
        components/libraries/log/src/nrf_log_default_backends.c \
        components/libraries/log/src/nrf_log_frontend.c \
        components/libraries/log/src/nrf_log_str_formatter.c
endif

NRF5_SDK_OBJ_FILES=$(NRF5_SDK_SOURCE_FILES:.c=.o)


INCLUDES=-Ilibs \
    -I FreeRTOS \
    -I$(NRF5_SDK_PATH)/components/drivers_nrf/nrf_soc_nosd \
    -I$(NRF5_SDK_PATH)/components \
    -I$(NRF5_SDK_PATH)/components/boards \
    -I$(NRF5_SDK_PATH)/components/softdevice/common \
    -I$(NRF5_SDK_PATH)/integration/nrfx \
    -I$(NRF5_SDK_PATH)/integration/nrfx/legacy \
    -I$(NRF5_SDK_PATH)/modules/nrfx \
    -I$(NRF5_SDK_PATH)/modules/nrfx/drivers/include \
    -I$(NRF5_SDK_PATH)/modules/nrfx/hal \
    -I$(NRF5_SDK_PATH)/modules/nrfx/mdk \
    -I$(NRF5_SDK_PATH)/external/freertos/source/include \
    -I$(NRF5_SDK_PATH)/components/toolchain/cmsis/include \
    -I$(NRF5_SDK_PATH)/components/libraries/atomic \
    -I$(NRF5_SDK_PATH)/components/libraries/atomic_fifo \
    -I$(NRF5_SDK_PATH)/components/libraries/atomic_flags \
    -I$(NRF5_SDK_PATH)/components/libraries/balloc \
    -I$(NRF5_SDK_PATH)/components/libraries/bootloader/ble_dfu \
    -I$(NRF5_SDK_PATH)/components/libraries/cli \
    -I$(NRF5_SDK_PATH)/components/libraries/crc16 \
    -I$(NRF5_SDK_PATH)/components/libraries/crc32 \
    -I$(NRF5_SDK_PATH)/components/libraries/crypto \
    -I$(NRF5_SDK_PATH)/components/libraries/csense \
    -I$(NRF5_SDK_PATH)/components/libraries/csense_drv \
    -I$(NRF5_SDK_PATH)/components/libraries/delay \
    -I$(NRF5_SDK_PATH)/components/libraries/ecc \
    -I$(NRF5_SDK_PATH)/components/libraries/experimental_section_vars \
    -I$(NRF5_SDK_PATH)/components/libraries/experimental_task_manager \
    -I$(NRF5_SDK_PATH)/components/libraries/fds \
    -I$(NRF5_SDK_PATH)/components/libraries/fstorage \
    -I$(NRF5_SDK_PATH)/components/libraries/gfx \
    -I$(NRF5_SDK_PATH)/components/libraries/gpiote \
    -I$(NRF5_SDK_PATH)/components/libraries/hardfault \
    -I$(NRF5_SDK_PATH)/components/libraries/hci \
    -I$(NRF5_SDK_PATH)/components/libraries/led_softblink \
    -I$(NRF5_SDK_PATH)/components/libraries/log \
    -I$(NRF5_SDK_PATH)/components/libraries/log/src \
    -I$(NRF5_SDK_PATH)/components/libraries/low_power_pwm \
    -I$(NRF5_SDK_PATH)/components/libraries/mem_manager \
    -I$(NRF5_SDK_PATH)/components/libraries/memobj \
    -I$(NRF5_SDK_PATH)/components/libraries/mpu \
    -I$(NRF5_SDK_PATH)/components/libraries/mutex \
    -I$(NRF5_SDK_PATH)/components/libraries/pwm \
    -I$(NRF5_SDK_PATH)/components/libraries/pwr_mgmt \
    -I$(NRF5_SDK_PATH)/components/libraries/queue \
    -I$(NRF5_SDK_PATH)/components/libraries/ringbuf \
    -I$(NRF5_SDK_PATH)/components/libraries/scheduler \
    -I$(NRF5_SDK_PATH)/components/libraries/sdcard \
    -I$(NRF5_SDK_PATH)/components/libraries/slip \
    -I$(NRF5_SDK_PATH)/components/libraries/sortlist \
    -I$(NRF5_SDK_PATH)/components/libraries/spi_mngr \
    -I$(NRF5_SDK_PATH)/components/libraries/stack_guard \
    -I$(NRF5_SDK_PATH)/components/libraries/strerror \
    -I$(NRF5_SDK_PATH)/components/libraries/svc \
    -I$(NRF5_SDK_PATH)/components/libraries/timer \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/audio \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/cdc \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/cdc/acm \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/hid \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/hid/generic \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/hid/kbd \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/hid/mouse \
    -I$(NRF5_SDK_PATH)/components/libraries/usbd/class/msc \
    -I$(NRF5_SDK_PATH)/components/libraries/util \
    -I$(NRF5_SDK_PATH)/external/segger_rtt \
    -I$(NRF5_SDK_PATH)/external/fprintf \
    -I$(LUA_PATH)/include \
    -isystem . \
    -I$(NRF5_SDK_PATH)/external/thedotfactory_fonts

OPTS=-MP -MD -mthumb -mabi=aapcs -g3 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin --short-enums -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fstack-usage -Os
WARNINGS=-Wall -Wextra -Warray-bounds=2 -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-nonliteral -ftree-vrp -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unknown-pragmas -Wno-expansion-to-defined  -Wreturn-type -Werror=return-type
STD=-std=c99

CC=$(ARM_NONE_EABI_TOOLCHAIN_PATH)/bin/arm-none-eabi-gcc
CFLAGS=$(SYSROOT) $(DEFS) $(INCLUDES) $(OPTS) $(WARNINGS) $(STD)

define build_sdk_file
$(patsubst %.c,nrf/%.o,$(1)): $(NRF5_SDK_PATH)/$(1)
	mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) -o $$@ -c $$^
endef

$(foreach f,$(NRF5_SDK_SOURCE_FILES),$(eval $(call build_sdk_file,$(f))))

libnrf.a: $(patsubst %,nrf/%,$(NRF5_SDK_OBJ_FILES))
	ar rs $@ $^


gcc_startup_nrf52.o: $(NRF5_SDK_PATH)/modules/nrfx/mdk/gcc_startup_nrf52.S
	$(CC) $(CFLAGS) -c $< -o $@

OBJECTS=\
	gcc_startup_nrf52.o \
	chablon.o \
	lua_state.o \
	lcd_spi_controller.o \
	FreeRTOS/port.o \
	FreeRTOS/port_cmsis.o \
	FreeRTOS/port_cmsis_systick.o
LOADLIBES=-lnrf -llua -lm

chablon: $(OBJECTS)
chablon: libnrf.a

$(OBJECTS) $(NRF5_SDK_OBJ_FILES): Makefile sdk_config.h
$(OBJECTS): CFLAGS+=-pedantic

chablon.elf: chablon
	mv $^ $@

LDFLAGS=$(SYSROOT) $(OPTS) \
     -T gcc_nrf52.ld  \
     -Wl,--gc-sections \
     -Wl,--print-memory-usage --specs=nano.specs \
     -lc -lnosys -lm -Wl,-Map=chablon.map \
     -L$(LUA_PATH)/lib
