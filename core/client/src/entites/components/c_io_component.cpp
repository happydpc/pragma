/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/entities/components/c_io_component.hpp"

using namespace pragma;

luabind::object CIOComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CIOComponentHandleWrapper>(l);}
