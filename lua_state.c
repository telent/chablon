#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"

extern char backlight_lua[];
extern int backlight_lua_len;

void * sbrk(intptr_t );

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

int luaC_dostring_or_log(lua_State *L, char *string) {
    return luaC_dobytes_or_log(L, (uint8_t *)string, strlen(string));
}

int luaC_dobytes_or_log(lua_State *L, uint8_t *bytes, int len) {

    int ret = (luaL_loadbuffer(L, bytes, len, "chunk") ||
	       (lua_pcall(L, 0, LUA_MULTRET, 0)));
    char * err = lua_tostring(L, -1);

    if(ret != LUA_OK) {
	NRF_LOG_INFO("luaL_dostring error %s", err);
	lua_pop(L, 1);
    }
    return ret;
}

/* expose byte buffer accessors to lua, for spi etc */

static int byte_buffer_index (lua_State* L) {
    uint8_t ** parray = luaL_checkudata(L, 1, "byte_buffer");
    int index = luaL_checkinteger(L, 2);
    lua_pushnumber(L, (*parray)[index-1]);
    return 1;
}

static int byte_buffer_newindex (lua_State* L) {
    uint16_t ** parray = luaL_checkudata(L, 1, "byte_buffer");
    int index = luaL_checkinteger(L, 2);
    int value = luaL_checkinteger(L, 3);
    if((value < 0) || (value >= 256)) {
	// XXX untested
	luaL_argerror(L, value, "byte value not in range  0 <= v < 256");
    }
    (*parray)[index-1] = value;
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

/* expose an area of C memory to Lua, by storing it in a userdata with
 * the byte_buffer metatable
 */

static int expose_byte_buffer(lua_State* L, uint8_t array[]) {
    uint8_t ** parray = lua_newuserdata(L, sizeof(uint8_t **));
    *parray = array;
    luaL_getmetatable(L, "byte_buffer");
    lua_setmetatable(L, -2);
    return 1;
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
    NRF_LOG_INFO("gpio write %d => %d", value, pin);

    nrf_gpio_pin_write(pin, value);
    return 0;
}

static int task_delay(lua_State *L){
    int ms = lua_tonumber(L, 1);
    nrf_delay_ms(ms);
    return 0;
}

uint8_t lcd_buffer[10];

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

    luaC_dobytes_or_log(L, backlight_lua, backlight_lua_len);
    lua_setglobal(L, "backlight");

}




/* the lua_State is a private item which should be acessed through
 * lua_state()
 */

static lua_State *L = NULL;

lua_State * lua_state() {
    memused("before");
    if(L == NULL) {
	L = luaL_newstate();
	opensomelibs(L);
    }
    create_libs(L);
    memused("open");
    create_byte_buffer_metatable(L);
    return L;
}
