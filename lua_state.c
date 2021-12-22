#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"

#define LUA_SOURCE(c) \
    extern uint8_t c##_lua[]; \
    extern int c##_lua_len

LUA_SOURCE(backlight);
LUA_SOURCE(spi_controller);
LUA_SOURCE(lcd);
LUA_SOURCE(hello);

#define CHUNK_NAME(c) ("@" #c ".lua")
#define RUN_FILE(c) luaC_dobytes_or_log(L, c##_lua, c##_lua_len, CHUNK_NAME(c))

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
    int ret = (luaL_loadbuffer(L, (const char *)bytes, len, chunk_name) ||
	       (lua_pcall(L, 0, LUA_MULTRET, 0)));

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

/* expose byte buffer accessors to lua, for spi etc */

static int byte_buffer_index (lua_State* L) {
    uint8_t *buf = luaL_checkudata(L, 1, "byte_buffer");
    int index = luaL_checkinteger(L, 2);
    lua_pushnumber(L, buf[index-1]);
    return 1;
}

static int inspect(lua_State* L) {
    NRF_LOG_INFO("sbrk %x", sbrk(0));
    if(lua_isinteger(L, 1))
	NRF_LOG_INFO("inspect int %d", lua_tointeger(L, 1));
    if(lua_isstring(L, 1))
	NRF_LOG_INFO("inspect string %s", lua_tostring(L, 1));
    return 0;
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
		(unsigned)((float)lua_tonumber(L, -1));
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

    static const struct luaL_Reg byte_buffer_funcs[] = {
	{ "new",  byte_buffer_new },
	{ NULL, NULL }
    };
    luaL_newlib(L, byte_buffer_funcs);
    lua_setglobal(L, "byte_buffer");

    static const struct luaL_Reg spictl_funcs[] = {
	{ "new",  spictl_new },
	{ "transfer",  spictl_transfer },
	{ NULL, NULL }
    };
    luaL_newlib(L, spictl_funcs);
    lua_setglobal(L, "spictl_ffi");
    memused("spictl");

    lua_pushcfunction(L, inspect);
    lua_setglobal(L, "inspect");

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
    (void) luaL_dostring(L, "return collectgarbage('collect')");
    (void) luaL_dostring(L, "return collectgarbage('setpause', 100)");
    (void) luaL_dostring(L, "return collectgarbage('setstepmul', 100)");
    lua_pop(L, 2);

    opensomelibs(L);
    create_byte_buffer_metatable(L);
    create_libs(L);
    memused("open");

    return L;
}

void lua_hello() {
    lua_State *L = lua_state();

    if(! luaC_dobytes_or_log(L, hello_lua, hello_lua_len, CHUNK_NAME(hello))) {
	NRF_LOG_INFO("answer is %s", lua_tostring(L, -1));
	lua_pop(L, 1);
     }
}
