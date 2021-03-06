/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __LUA_ERROR_HANDLING_HPP__
#define __LUA_ERROR_HANDLING_HPP__

#include "pragma/definitions.h"

namespace Lua
{
	DLLENGINE void initialize_error_handler();
	DLLENGINE int HandleTracebackError(lua_State *l);
	// Note: This function will attempt to retrieve the file name from the error message.
	// If the file name is truncated, this will not work! To be sure, define the third parameter as the actual file name.
	DLLENGINE void HandleSyntaxError(lua_State *l,Lua::StatusCode r);
	DLLENGINE void HandleSyntaxError(lua_State *l,Lua::StatusCode r,const std::string &fileName);

	DLLENGINE void OpenFileInZeroBrane(const std::string &fname,uint32_t lineId);
	DLLENGINE std::optional<std::string> GetLuaFilePath(const std::string &fname);
};

#endif
