/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/entities/components/c_bsp_leaf_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"

extern DLLCENGINE CEngine *c_engine;

using namespace pragma;

luabind::object CBSPLeafComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CBSPLeafComponentHandleWrapper>(l);}
void CBSPLeafComponent::SetLeaves(const std::vector<uint16_t> &leaves)
{
	for(auto idx : leaves)
		m_leaves.insert(idx);
}
bool CBSPLeafComponent::GetLeafVisibility(uint16_t leafIdx) const
{
	return m_leaves.find(leafIdx) != m_leaves.end();
}
