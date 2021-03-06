#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"

#define LUA_SOURCE(c) \
    extern uint8_t c##_luac[]; \
    extern int c##_luac_len

LUA_SOURCE(backlight);
LUA_SOURCE(spi_controller);
LUA_SOURCE(lcd);
LUA_SOURCE(hello);
LUA_SOURCE(byte_buffer);

#define CHUNK_NAME(c) ("@" #c ".lua")
#define RUN_FILE(c) luaC_dobytes_or_log(L, c##_luac, c##_luac_len, CHUNK_NAME(c))

void * sbrk(intptr_t);

#define LUA_ERROR_CHECK(code) \
do							    \
    {                                                       \
        const uint32_t local_err_code = (code);             \
        if (local_err_code != NRF_SUCCESS)                  \
        {                                                   \
            luaL_error(L, "nRF lib failure %d at %s:%d", local_err_code, __FILE__ , __LINE__); \
        }                                                   \
    } while (0)

static void memused(const char *loc) {
    NRF_LOG_INFO("heap at %s: %x", loc, sbrk(0));
}

static void opensomelibs (lua_State *L) {
    static const luaL_Reg libs[] = {
	{"_G", luaopen_base},
	{LUA_LOADLIBNAME, luaopen_package},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{NULL, NULL}
    };
    const luaL_Reg *lib;

    for (lib = libs; lib->func; lib++) {
	luaL_requiref(L, lib->name, lib->func, 1);
	lua_pop(L, 1);
    }
}

int luaC_dobytes_or_log(lua_State *L, const uint8_t *bytes, int len, char * chunk_name) {
    int ret = luaL_loadbuffer(L, (const char *)bytes, len, chunk_name);
    if(ret == LUA_OK)
	ret = lua_pcall(L, 0, LUA_MULTRET, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);
    if(ret != LUA_OK) {
	const char * err = lua_tostring(L, -1);
	NRF_LOG_INFO("lua script error %s", err);
	lua_pop(L, 1);
    }
    return ret;
}

int luaC_dostring_or_log(lua_State *L, const char *string) {
    return luaC_dobytes_or_log(L, (const uint8_t *)string, strlen(string), "string");
}

static int trace(lua_State* L) {
    if(lua_isinteger(L, 1)) {
	NRF_LOG_INFO("brk %x trace int = %d",
		     sbrk(0), lua_tointeger(L, 1));
    } else if(lua_isstring(L, 1)) {
	NRF_LOG_INFO("brk %x trace string = %s",
		     sbrk(0), lua_tostring(L, 1));
    }
    else {
	NRF_LOG_INFO("brk %x trace %s = ???",
		     sbrk(0),
		     lua_typename(L, lua_type(L, 1)));
    }
    lua_pushvalue(L, 1);
    return 1;
}

/* expose byte buffer accessors to lua, for spi etc */

static int byte_buffer_index (lua_State* L) {
    uint8_t *buf = luaL_checkudata(L, 1, "byte_buffer");
    int index = luaL_checkinteger(L, 2);
    lua_pushnumber(L, buf[index-1]);
    return 1;
}

static int byte_buffer_newindex (lua_State* L) {
    uint8_t *buf  = luaL_checkudata(L, 1, "byte_buffer");
    int index = luaL_checkinteger(L, 2);
    int value = luaL_checkinteger(L, 3);
    if((value < 0) || (value >= 256)) {
	// XXX untested
	luaL_argerror(L, value, "byte value not in range  0 <= v < 256");
    }
    buf[index-1] = value;
    return 0;
}

static void create_byte_buffer_metatable(lua_State* L) {
    static const struct luaL_Reg funcs[] = {
	{ "__index",  byte_buffer_index  },
	{ "__newindex",  byte_buffer_newindex  },
	{ NULL, NULL }
    };
    luaL_newmetatable(L, "byte_buffer");
    luaL_setfuncs(L, funcs, 0);
}


static int byte_buffer_new(lua_State* L) {
    int size = luaL_checkinteger(L, 1);
    void * bytes = lua_newuserdata(L, size);
    memset(bytes, 0, size);
    luaL_getmetatable(L, "byte_buffer");
    lua_setmetatable(L, -2);
    return 1;
}


/* spi stuffz
 */


/* NRFX_SPIM_INSTANCE is a macro that uses its unevaluated
 * parameter in token concatenation, therefore impossible to
 * use except with a literal value. Hence this array
 */
static nrfx_spim_t spi_instances[] = {
#if NRFX_CHECK(NRFX_SPIM0_ENABLED)
    NRFX_SPIM_INSTANCE(0),
#endif
#if NRFX_CHECK(NRFX_SPIM1_ENABLED)
    NRFX_SPIM_INSTANCE(1),
#endif
#if NRFX_CHECK(NRFX_SPIM2_ENABLED)
    NRFX_SPIM_INSTANCE(2),
#endif
#if NRFX_CHECK(NRFX_SPIM3_ENABLED)
    NRFX_SPIM_INSTANCE(3),
#endif

};

/* this may be premature optimization and strcmp would have
 * been perfectly fine (it only gets called at setup time,
 * anyway)
 */

static unsigned long nrf_frequency(const char *id) {
#define freqhash(a,b,c) ((a<<16) | (b<<8) | (c))
    if(id[0] == 'm') {
	switch(id[1]) {
	case '1': return NRF_SPIM_FREQ_1M;
	case '2': return NRF_SPIM_FREQ_2M;
	case '4': return NRF_SPIM_FREQ_4M;
	case '8': return NRF_SPIM_FREQ_8M;
	}
    } else if(id[0]=='k') {
	switch(freqhash(id[1], id[2], id[3])) {
	case freqhash('1','2','5') : return NRF_SPIM_FREQ_125K;
	case freqhash('2','5','0') : return NRF_SPIM_FREQ_250K;
	case freqhash('5','0','0') : return NRF_SPIM_FREQ_500K;
	}
    }
    return 0;
#undef freqhash
}


static int spictl_new(lua_State* L) {
    int instance = luaL_checkinteger(L, 1);
    /* XXX check instance is in spi_instances */

    if(! lua_istable(L, 2))
	luaL_error(L, "params must be a table");

    nrfx_spim_t * spi = &(spi_instances[instance]);
    lua_pushlightuserdata(L, spi);

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;

    lua_pushnil(L);
    while(lua_next(L, 2) != 0) {
	const char * key = lua_tostring(L, -2);
	if(!strcmp(key, "frequency"))
	    spi_config.frequency =
		nrf_frequency(lua_tostring(L, -1));
	else if(!strcmp(key, "mode"))
	    spi_config.mode = lua_tointeger(L, -1);
	else if(!strcmp(key, "cs-pin"))
	    spi_config.ss_pin = lua_tointeger(L, -1);
	else if(!strcmp(key, "cipo-pin"))
	    spi_config.miso_pin = lua_tointeger(L, -1);
	else if(!strcmp(key, "copi-pin"))
	    spi_config.mosi_pin = lua_tointeger(L, -1);
	else if(!strcmp(key, "sck-pin"))
	    spi_config.sck_pin = lua_tointeger(L, -1);
	else if(!strcmp(key, "cs-active-high"))
	    spi_config.ss_active_high = lua_toboolean(L, -1);
	lua_pop(L, 1);
    }

    LUA_ERROR_CHECK(nrfx_spim_init(spi, &spi_config, NULL,  NULL));

    return 1;
}

static int spictl_transfer(lua_State* L) {
    nrfx_spim_t * spi = (nrfx_spim_t *) lua_touserdata(L, 1);
    uint8_t * buf = (uint8_t *) lua_touserdata(L, 2);
	/* luaL_checkudata(L, 2, "byte_buffer");*/
    int count = luaL_checkinteger(L, 3);

    /* NRF_LOG_INFO("transfer %p %p %d", spi, buf, count); */

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(buf, count);

    LUA_ERROR_CHECK(nrfx_spim_xfer(spi, &xfer_desc, NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER));

   volatile int * finished = (int *)nrfx_spim_end_event_get(spi);
    while (!*finished) {
      __WFE();
    }
    return 0;
}


static int gpio_set_direction(lua_State *L) {
    /* 0 = output, 1 = input.
     * Mnemomic: 1 looks like "i", 0 looks like "o"
     */
    int pin = lua_tonumber(L, 1);
    int direction = lua_tonumber(L, 2);
    NRF_LOG_INFO("gpio %d as %d", pin, direction);

    if(direction==1) {
	/* nrf_gpio_cfg_input(pin); */
    } else {
	nrf_gpio_cfg_output(pin);
    }
    return 0;
}

static int gpio_write(lua_State *L) {
    int pin = lua_tonumber(L, 1);
    int value = lua_tonumber(L, 2);
    /* NRF_LOG_INFO("gpio write %d => %d", value, pin); */

    nrf_gpio_pin_write(pin, value);
    return 0;
}

static int task_delay(lua_State *L){
    int ms = lua_tonumber(L, 1);
    nrf_delay_ms(ms);
    return 0;
}

static int unsafe_peek(lua_State *L){
    int8_t * userdata = (void *) lua_touserdata(L, 1);
    int offset = lua_tonumber(L, 2);
    int size = lua_tonumber(L, 3);
    /* FIXME check offset is in bounds */
    switch(size) {
    case 1:
	lua_pushinteger(L, *(userdata+offset));
	return 1;
    case 2:
	lua_pushinteger(L, *(int16_t *)(userdata+offset));
	return 1;
    case 4:
	lua_pushinteger(L, *(int32_t *)(userdata+offset));
	return 1;
    }
    return 0;
}

static int unsafe_poke(lua_State *L){
    int8_t  * userdata = (void *) lua_touserdata(L, 1);
    int offset = lua_tonumber(L, 2);
    int size = lua_tonumber(L, 3);
    int value = lua_tointeger(L, 4);
    /* FIXME check offset is in bounds */
    switch(size) {
    case 1:
	*(userdata+offset) = value;
	break;
    case 2:
	*(int16_t *)(userdata+offset) = value;
	break;
    case 4:
	*(int32_t *)(userdata+offset) = value;
	break;
    }
    return 0;
}

static int unsafe_alloc(lua_State* L) {
    int size = luaL_checkinteger(L, 1);
    void * bytes = lua_newuserdata(L, size);
    memset(bytes, 0, size);
    return 1;
}


static void create_libs(lua_State* L) {
    static const struct luaL_Reg gpio_funcs[] = {
	{ "set_direction",  gpio_set_direction  },
	{ "write",  gpio_write  },
	{ NULL, NULL }
    };
    luaL_newlib(L, gpio_funcs);
    lua_setglobal(L, "gpio");

    static const struct luaL_Reg task_funcs[] = {
	{ "delay",  task_delay  },
	{ NULL, NULL }
    };
    luaL_newlib(L, task_funcs);
    lua_setglobal(L, "task");

    static const struct luaL_Reg glue_funcs[] = {
	{ "byte_buffer_new",  byte_buffer_new },
	{ NULL, NULL }
    };
    luaL_newlib(L, glue_funcs);
    lua_setglobal(L, "glue");

    static const struct luaL_Reg spictl_funcs[] = {
	{ "new",  spictl_new },
	{ "transfer",  spictl_transfer },
	{ NULL, NULL }
    };
    luaL_newlib(L, spictl_funcs);
    lua_setglobal(L, "spictl_ffi");
    memused("spictl");

    static const struct luaL_Reg unsafe_funcs[] = {
	{ "peek",  unsafe_peek },
	{ "poke",  unsafe_poke },
	{ "alloc",  unsafe_alloc },
	{ NULL, NULL }
    };
    luaL_newlib(L, unsafe_funcs);
    lua_setglobal(L, "unsafe");


    lua_pushcfunction(L, trace);
    lua_setglobal(L, "trace");

    (void) RUN_FILE(byte_buffer);
    lua_setglobal(L, "byte_buffer");
    memused("byte_buffer");

    (void) RUN_FILE(backlight);
    lua_setglobal(L, "backlight");
    memused("backlight");

    (void) RUN_FILE(spi_controller);
    lua_setglobal(L, "spi_controller");
    memused("spi_controller");

    (void) RUN_FILE(lcd);
    lua_setglobal(L, "lcd");
    memused("lcd");

}


/* the lua_State is a private item which should be acessed through
 * lua_state()
 */

static lua_State *L = NULL;

lua_State * lua_state() {
    memused("before");
    if(L != NULL)
	return L;

    L = luaL_newstate();

    lua_gc(L, LUA_GCGEN, 10, 100);

    opensomelibs(L);
    create_byte_buffer_metatable(L);
    create_libs(L);
    memused("open");

    return L;
}

void lua_hello() {
    lua_State *L = lua_state();

    if(! RUN_FILE(hello)) {
	NRF_LOG_INFO("answer is %s", lua_tostring(L, -1));
	lua_pop(L, 1);
     }
}
