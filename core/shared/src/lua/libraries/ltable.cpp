#include "stdafx_shared.h"
#include <pragma/definitions.h>
#include "pragma/lua/libraries/ltable.h"
#include <luasystem.h>

int Lua::table::has_value(lua_State *l)
{
	Lua::CheckTable(l,1);

	Lua::PushValue(l,1); /* 1 */
	auto t = Lua::GetStackTop(l);

	Lua::PushNil(l); /* 2 */
	while(Lua::GetNextPair(l,t) != 0) /* 3 */
	{
		// TODO: Compare values
		Lua::Pop(l,1); /* 2 */
	} /* 1 */

	Lua::Pop(l,1); /* 0 */

	Lua::PushBool(l,false);
	return 1;
}

int Lua::table::random(lua_State *l)
{
	Lua::CheckTable(l,1);
	auto n = Lua::GetObjectLength(l,1);
	if(n == 0)
		return 0;
	auto r = umath::random(1,static_cast<int32_t>(n));
	Lua::PushInt(l,r);
	Lua::GetTableValue(l,1);
	return 1;
}