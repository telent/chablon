#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "nrf_log.h"

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
    int ret = luaL_dostring(L, string);
    NRF_LOG_INFO("luaL_dostring=%d", ret);
    if(ret != LUA_OK) {
	NRF_LOG_INFO("luaL_dostring error %s", lua_tostring(L, -1));
	lua_pop(L, 1);
    }
    return ret;
}

static lua_State *L = NULL;

lua_State * lua_state() {
    memused("before");
    if(L == NULL) {
	L = luaL_newstate();
	opensomelibs(L);
    }
    memused("open");
    return L;
}
