#include "stdafx_shared.h"
#include "pragma/lua/classes/lentity.h"
#include <pragma/math/angle/wvquaternion.h>
#include "luasystem.h"
#include "pragma/lua/classes/ldef_entity.h"
#include "pragma/lua/classes/ldef_physobj.h"
#include "pragma/lua/classes/ldef_model.h"
#include "pragma/lua/classes/ldef_vector.h"
#include "pragma/lua/classes/ldef_angle.h"
#include "pragma/lua/classes/ldef_color.h"
#include "pragma/lua/libraries/lray.h"
#include "pragma/model/brush/brushmesh.h"
#include "pragma/audio/alsound.h"
#include "pragma/lua/classes/lphysics.h"
#include "pragma/physics/raytraces.h"
#include "pragma/audio/alsound_type.h"
#include "pragma/entities/components/base_weapon_component.hpp"
#include "pragma/entities/components/base_character_component.hpp"
#include "pragma/entities/components/base_player_component.hpp"
#include "pragma/entities/components/base_transform_component.hpp"
#include "pragma/entities/components/base_physics_component.hpp"
#include "pragma/entities/components/base_ai_component.hpp"
#include "pragma/entities/components/base_vehicle_component.hpp"
#include "pragma/entities/components/base_model_component.hpp"
#include "pragma/entities/components/base_animated_component.hpp"
#include "pragma/lua/lentity_type.hpp"
#include "pragma/lua/classes/lproperty.hpp"
#include "pragma/lua/l_entity_handles.hpp"
#include "pragma/lua/lua_entity_component.hpp"
#include <sharedutils/datastream.h>
#include <pragma/physics/movetypes.h>

extern DLLENGINE Engine *engine;

void Lua::Entity::register_class(luabind::class_<EntityHandle> &classDef)
{
	classDef.def(luabind::tostring(luabind::self));
	classDef.def("IsValid",&IsValid);
	classDef.def("Spawn",&Spawn);
	classDef.def("Remove",&Remove);
	classDef.def("GetIndex",&GetIndex);
	classDef.def("IsCharacter",&IsCharacter);
	classDef.def("IsPlayer",&IsPlayer);
	classDef.def("IsWorld",&IsWorld);
	classDef.def("IsInert",&IsInert);
	classDef.def("GetClass",&GetClass);
	//classDef.def("AddCallback",&AddCallback); // Obsolete
	classDef.def("IsScripted",&IsScripted);
	classDef.def("IsSpawned",&IsSpawned);
	classDef.def("SetKeyValue",&SetKeyValue);
	classDef.def("IsNPC",&IsNPC);
	classDef.def("IsWeapon",&IsWeapon);
	classDef.def("IsVehicle",&IsVehicle);
	classDef.def("RemoveSafely",&RemoveSafely);
	classDef.def("RemoveEntityOnRemoval",static_cast<void(*)(lua_State*,EntityHandle&,EntityHandle&)>(&RemoveEntityOnRemoval));
	classDef.def("RemoveEntityOnRemoval",static_cast<void(*)(lua_State*,EntityHandle&,EntityHandle&,Bool)>(&RemoveEntityOnRemoval));
	classDef.def("GetSpawnFlags",&GetSpawnFlags);
	// Obsolete
	/*classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	classDef.def("CallCallbacks",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object,luabind::object)>(&CallCallbacks));
	*/
	classDef.def("GetPos",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		if(hEnt->GetTransformComponent().expired())
		{
			Lua::Push<Vector3>(l,Vector3{});
			return;
		}
		Lua::Push<Vector3>(l,hEnt->GetPosition());
	}));
	classDef.def("SetPos",static_cast<void(*)(lua_State*,EntityHandle&,const Vector3&)>([](lua_State *l,EntityHandle &hEnt,const Vector3 &pos) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto *trComponent = static_cast<pragma::BaseTransformComponent*>(hEnt->AddComponent("transform").get());
		if(trComponent == nullptr)
			return;
		hEnt->SetPosition(pos);
	}));
	classDef.def("GetAngles",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		if(hEnt->GetTransformComponent().expired())
		{
			Lua::Push<EulerAngles>(l,EulerAngles{});
			return;
		}
		Lua::Push<EulerAngles>(l,hEnt->GetTransformComponent()->GetAngles());
	}));
	classDef.def("SetAngles",static_cast<void(*)(lua_State*,EntityHandle&,const EulerAngles&)>([](lua_State *l,EntityHandle &hEnt,const EulerAngles &ang) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto *trComponent = static_cast<pragma::BaseTransformComponent*>(hEnt->AddComponent("transform").get());
		if(trComponent == nullptr)
			return;
		trComponent->SetAngles(ang);
	}));
	classDef.def("GetRotation",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		if(hEnt->GetTransformComponent().expired())
		{
			Lua::Push<EulerAngles>(l,EulerAngles{});
			return;
		}
		Lua::Push<EulerAngles>(l,hEnt->GetTransformComponent()->GetAngles());
	}));
	classDef.def("SetRotation",static_cast<void(*)(lua_State*,EntityHandle&,const Quat&)>([](lua_State *l,EntityHandle &hEnt,const Quat &rot) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto *trComponent = static_cast<pragma::BaseTransformComponent*>(hEnt->AddComponent("transform").get());
		if(trComponent == nullptr)
			return;
		trComponent->SetOrientation(rot);
	}));
	classDef.def("GetCenter",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		if(hEnt->GetPhysicsComponent().expired())
		{
			if(hEnt->GetTransformComponent().expired())
			{
				Lua::Push<Vector3>(l,Vector3{});
				return;
			}
			Lua::Push<Vector3>(l,hEnt->GetTransformComponent()->GetPosition());
			return;
		}
		Lua::Push<Vector3>(l,hEnt->GetPhysicsComponent()->GetCenter());
	}));

	classDef.def("AddComponent",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&,bool)>([](lua_State *l,EntityHandle &hEnt,const std::string &name,bool bForceCreateNew) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->AddComponent(name,bForceCreateNew);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("AddComponent",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&)>([](lua_State *l,EntityHandle &hEnt,const std::string &name) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->AddComponent(name);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("AddComponent",static_cast<void(*)(lua_State*,EntityHandle&,uint32_t,bool)>([](lua_State *l,EntityHandle &hEnt,uint32_t componentId,bool bForceCreateNew) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->AddComponent(componentId,bForceCreateNew);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("AddComponent",static_cast<void(*)(lua_State*,EntityHandle&,uint32_t)>([](lua_State *l,EntityHandle &hEnt,uint32_t componentId) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->AddComponent(componentId);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("RemoveComponent",static_cast<void(*)(lua_State*,EntityHandle&,BaseEntityComponentHandle&)>([](lua_State *l,EntityHandle &hEnt,BaseEntityComponentHandle &hComponent) {
		LUA_CHECK_ENTITY(l,hEnt);
		::pragma::Lua::check_component(l,hComponent);
		hEnt->RemoveComponent(*hComponent.get());
	}));
	classDef.def("RemoveComponent",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&)>([](lua_State *l,EntityHandle &hEnt,const std::string &name) {
		LUA_CHECK_ENTITY(l,hEnt);
		hEnt->RemoveComponent(name);
	}));
	classDef.def("RemoveComponent",static_cast<void(*)(lua_State*,EntityHandle&,uint32_t)>([](lua_State *l,EntityHandle &hEnt,uint32_t componentId) {
		LUA_CHECK_ENTITY(l,hEnt);
		hEnt->RemoveComponent(componentId);
	}));
	classDef.def("ClearComponents",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		hEnt->ClearComponents();
	}));
	classDef.def("HasComponent",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&)>([](lua_State *l,EntityHandle &hEnt,const std::string &name) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto *nw = engine->GetNetworkState(l);
		auto *game = nw->GetGameState();
		auto &componentManager = game->GetEntityComponentManager();
		auto componentId = pragma::INVALID_COMPONENT_ID;
		if(componentManager.GetComponentTypeId(name,componentId) == false)
			return;
		Lua::PushBool(l,hEnt->HasComponent(componentId));
	}));
	classDef.def("HasComponent",static_cast<void(*)(lua_State*,EntityHandle&,uint32_t)>([](lua_State *l,EntityHandle &hEnt,uint32_t componentId) {
		LUA_CHECK_ENTITY(l,hEnt);
		Lua::PushBool(l,hEnt->HasComponent(componentId));
	}));
	classDef.def("GetComponent",static_cast<void(*)(lua_State*,EntityHandle&,const std::string&)>([](lua_State *l,EntityHandle &hEnt,const std::string &name) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->FindComponent(name);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("GetComponent",static_cast<void(*)(lua_State*,EntityHandle&,uint32_t)>([](lua_State *l,EntityHandle &hEnt,uint32_t componentId) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pComponent = hEnt->FindComponent(componentId);
		if(pComponent.expired())
			return;
		pComponent->PushLuaObject(l);
	}));
	classDef.def("GetComponents",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto &components = hEnt->GetComponents();
		auto t = Lua::CreateTable(l);
		auto idx = 1;
		for(auto &pComponent : components)
		{
			Lua::PushInt(l,idx++);
			pComponent->PushLuaObject(l);
			Lua::SetTableValue(l,t);
		}
	}));
	classDef.def("GetTransformComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pTrComponent = hEnt->GetTransformComponent();
		if(pTrComponent.expired())
			return;
		pTrComponent->PushLuaObject(l);
	}));
	classDef.def("GetPhysicsComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pPhysComponent = hEnt->GetPhysicsComponent();
		if(pPhysComponent.expired())
			return;
		pPhysComponent->PushLuaObject(l);
	}));
	classDef.def("GetCharacterComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pCharComponent = hEnt->GetCharacterComponent();
		if(pCharComponent.expired())
			return;
		pCharComponent->PushLuaObject(l);
	}));
	classDef.def("GetWeaponComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pWepComponent = hEnt->GetWeaponComponent();
		if(pWepComponent.expired())
			return;
		pWepComponent->PushLuaObject(l);
	}));
	classDef.def("GetVehicleComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pVhcComponent = hEnt->GetVehicleComponent();
		if(pVhcComponent.expired())
			return;
		pVhcComponent->PushLuaObject(l);
	}));
	classDef.def("GetPlayerComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pPlComponent = hEnt->GetPlayerComponent();
		if(pPlComponent.expired())
			return;
		pPlComponent->PushLuaObject(l);
	}));
	classDef.def("GetAIComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pAIComponent = hEnt->GetAIComponent();
		if(pAIComponent.expired())
			return;
		pAIComponent->PushLuaObject(l);
	}));
	classDef.def("GetModelComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pMdlComponent = hEnt->GetModelComponent();
		if(pMdlComponent.expired())
			return;
		pMdlComponent->PushLuaObject(l);
	}));
	classDef.def("GetAnimatedComponent",static_cast<void(*)(lua_State*,EntityHandle&)>([](lua_State *l,EntityHandle &hEnt) {
		LUA_CHECK_ENTITY(l,hEnt);
		auto pAnimComponent = hEnt->GetAnimatedComponent();
		if(pAnimComponent.expired())
			return;
		pAnimComponent->PushLuaObject(l);
	}));

	classDef.def("Save",&Save);
	classDef.def("Load",&Load);
	classDef.def("Copy",&Copy);

	classDef.def("GetAirDensity",&GetAirDensity);

	classDef.def("IsStatic",&IsStatic);
	classDef.def("IsDynamic",&IsDynamic);

	// Enums
	classDef.add_static_constant("TYPE_DEFAULT",umath::to_integral(LuaEntityType::Default));
	classDef.add_static_constant("TYPE_LOCAL",umath::to_integral(LuaEntityType::Default));
	classDef.add_static_constant("TYPE_SHARED",umath::to_integral(LuaEntityType::Shared));

	classDef.add_static_constant("EVENT_HANDLE_KEY_VALUE",BaseEntity::EVENT_HANDLE_KEY_VALUE);
	classDef.add_static_constant("EVENT_ON_SPAWN",BaseEntity::EVENT_ON_SPAWN);
	classDef.add_static_constant("EVENT_ON_POST_SPAWN",BaseEntity::EVENT_ON_POST_SPAWN);

	classDef.add_static_constant("EVENT_ON_COMPONENT_ADDED",pragma::BaseEntityComponent::EVENT_ON_ENTITY_COMPONENT_ADDED);
	classDef.add_static_constant("EVENT_ON_COMPONENT_REMOVED",pragma::BaseEntityComponent::EVENT_ON_ENTITY_COMPONENT_REMOVED);

	classDef.def(luabind::const_self ==luabind::other<EntityHandle>());
}

void Lua::Entity::IsValid(lua_State *l,EntityHandle &hEnt)
{
	lua_pushboolean(l,hEnt.IsValid() ? true : false);
}

void Lua::Entity::Remove(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	hEnt->Remove();
}

void Lua::Entity::GetIndex(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushInt(l,hEnt->GetIndex());
}

void Lua::Entity::IsCharacter(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushBool(l,hEnt->IsCharacter());
}

void Lua::Entity::IsPlayer(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	lua_pushboolean(l,hEnt->IsPlayer());
}

void Lua::Entity::IsNPC(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	lua_pushboolean(l,hEnt->IsNPC());
}

void Lua::Entity::IsWorld(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	lua_pushboolean(l,hEnt->IsWorld());
}

void Lua::Entity::IsInert(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	lua_pushboolean(l,hEnt->IsInert());
}

void Lua::Entity::Spawn(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	if(hEnt->IsSpawned())
		return;
	hEnt->Spawn();
}

void Lua::Entity::GetClass(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	std::string classname = hEnt->GetClass();
	lua_pushstring(l,classname.c_str());
}

// Obsolete
/*void Lua::Entity::AddCallback(lua_State *l,EntityHandle &hEnt,std::string identifier,luabind::object o)
{
	luaL_checkfunction(l,3);
	LUA_CHECK_ENTITY(l,hEnt);
	auto callback = hEnt->AddLuaCallback(identifier,o);
	Lua::Push<CallbackHandle>(l,callback);
}
*/
void Lua::Entity::IsScripted(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	bool b = hEnt->IsScripted();
	lua_pushboolean(l,b);
}

void Lua::Entity::IsSpawned(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	auto b = hEnt->IsSpawned();
	lua_pushboolean(l,b);
}

void Lua::Entity::SetKeyValue(lua_State *l,EntityHandle &hEnt,std::string key,std::string val)
{
	LUA_CHECK_ENTITY(l,hEnt);
	hEnt->SetKeyValue(key,val);
}

void Lua::Entity::IsWeapon(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	bool b = hEnt->IsWeapon();
	lua_pushboolean(l,b);
}

void Lua::Entity::IsVehicle(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	bool b = hEnt->IsVehicle();
	lua_pushboolean(l,b);
}
void Lua::Entity::RemoveSafely(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	hEnt->RemoveSafely();
}
void Lua::Entity::RemoveEntityOnRemoval(lua_State *l,EntityHandle &hEnt,EntityHandle &hEntOther,Bool)
{
	LUA_CHECK_ENTITY(l,hEnt);
	LUA_CHECK_ENTITY(l,hEntOther);
	hEnt->RemoveEntityOnRemoval(hEntOther);
}
void Lua::Entity::RemoveEntityOnRemoval(lua_State *l,EntityHandle &hEnt,EntityHandle &hEntOther) {Lua::Entity::RemoveEntityOnRemoval(l,hEnt,hEntOther,true);}
void Lua::Entity::GetSpawnFlags(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushInt(l,hEnt->GetSpawnFlags());
}
void Lua::Entity::IsStatic(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushBool(l,hEnt->IsStatic());
}
void Lua::Entity::IsDynamic(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushBool(l,hEnt->IsDynamic());
}

void Lua::Entity::Save(lua_State *l,EntityHandle &hEnt,::DataStream &ds)
{
	LUA_CHECK_ENTITY(l,hEnt);
	hEnt->Save(ds);
}
void Lua::Entity::Load(lua_State *l,EntityHandle &hEnt,::DataStream &ds)
{
	LUA_CHECK_ENTITY(l,hEnt);
	hEnt->Load(ds);
}
void Lua::Entity::Copy(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	auto *ent = hEnt->Copy();
	if(ent == nullptr)
		return;
	ent->GetLuaObject()->push(l);
}
void Lua::Entity::GetAirDensity(lua_State *l,EntityHandle &hEnt)
{
	LUA_CHECK_ENTITY(l,hEnt);
	Lua::PushNumber(l,1.225f); // Placeholder
}

// Obsolete
// Callbacks
/*static void call_callbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,uint32_t numArgs)
{
	LUA_CHECK_ENTITY(l,hEnt);
	auto *callbacks = hEnt->GetLuaCallbacks(name);
	if(callbacks == nullptr)
		return;
	const uint32_t argOffset = 3;
	for(auto &hCb : *callbacks)
	{
		if(hCb.IsValid() == false)
			continue;
		auto *luaCallback = static_cast<LuaCallback*>(hCb.get());
		auto &o = luaCallback->GetLuaObject();
		auto bReturn = false;
		Lua::Execute(l,[l,&o,numArgs,argOffset,&bReturn,&name](int(*traceback)(lua_State *l)) {
			auto n = Lua::GetStackTop(l);
			auto r = Lua::ProtectedCall(l,[&o,numArgs,argOffset](lua_State *l) {
				o.push(l);
				for(auto i=decltype(numArgs){0};i<numArgs;++i)
				{
					auto arg = argOffset +i;
					Lua::PushValue(l,arg);
				}
				return Lua::StatusCode::Ok;
			},LUA_MULTRET,traceback);
			if(r == Lua::StatusCode::Ok)
			{
				auto numResults = Lua::GetStackTop(l) -n;
				if(numResults > 0)
					bReturn = true;
			}
			return r;
		},Lua::GetErrorColorMode(l));
		if(bReturn == true)
			break;
	}
}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name) {call_callbacks(l,hEnt,name,0);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1) {call_callbacks(l,hEnt,name,1);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2) {call_callbacks(l,hEnt,name,2);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3) {call_callbacks(l,hEnt,name,3);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4) {call_callbacks(l,hEnt,name,4);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5) {call_callbacks(l,hEnt,name,5);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6) {call_callbacks(l,hEnt,name,6);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7) {call_callbacks(l,hEnt,name,7);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8) {call_callbacks(l,hEnt,name,8);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9) {call_callbacks(l,hEnt,name,9);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10) {call_callbacks(l,hEnt,name,10);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10,luabind::object o11) {call_callbacks(l,hEnt,name,11);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10,luabind::object o11,luabind::object o12) {call_callbacks(l,hEnt,name,12);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10,luabind::object o11,luabind::object o12,luabind::object o13) {call_callbacks(l,hEnt,name,13);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10,luabind::object o11,luabind::object o12,luabind::object o13,luabind::object o14) {call_callbacks(l,hEnt,name,14);}
void Lua::Entity::CallCallbacks(lua_State *l,EntityHandle &hEnt,const std::string &name,luabind::object o1,luabind::object o2,luabind::object o3,luabind::object o4,luabind::object o5,luabind::object o6,luabind::object o7,luabind::object o8,luabind::object o9,luabind::object o10,luabind::object o11,luabind::object o12,luabind::object o13,luabind::object o14,luabind::object o15) {call_callbacks(l,hEnt,name,15);}
*/