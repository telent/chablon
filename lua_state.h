#include <lua.h>

lua_State * lua_state(void);
int luaC_dostring_or_log(lua_State *L, char *string);
