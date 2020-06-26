/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/entities/environment/lights/c_env_light.h"
#include "pragma/model/c_model.h"
#include "pragma/model/c_modelmesh.h"
#include "pragma/console/c_cvar.h"
#include "pragma/console/c_cvar_global_functions.h"
#include "pragma/rendering/lighting/c_light_data_buffer_manager.hpp"
#include "pragma/lua/libraries/c_lua_vulkan.h"
#include <pragma/entities/entity_iterator.hpp>
#include <pragma/entities/entity_component_system_t.hpp>
#include <sharedutils/util_shaderinfo.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>

using namespace pragma;

extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
extern DLLCLIENT CGame *c_game;

#pragma optimize("",off)
decltype(CLightComponent::s_lightCount) CLightComponent::s_lightCount = 0u;
prosper::IUniformResizableBuffer &CLightComponent::GetGlobalRenderBuffer() {return pragma::LightDataBufferManager::GetInstance().GetGlobalRenderBuffer();}
prosper::IUniformResizableBuffer &CLightComponent::GetGlobalShadowBuffer() {return pragma::ShadowDataBufferManager::GetInstance().GetGlobalRenderBuffer();}
uint32_t CLightComponent::GetMaxLightCount() {return pragma::LightDataBufferManager::GetInstance().GetMaxCount();}
uint32_t CLightComponent::GetMaxShadowCount() {return pragma::ShadowDataBufferManager::GetInstance().GetMaxCount();}
uint32_t CLightComponent::GetLightCount() {return s_lightCount;}
ComponentEventId CLightComponent::EVENT_SHOULD_PASS_ENTITY = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_SHOULD_PASS_ENTITY_MESH = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_SHOULD_PASS_MESH = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_SHOULD_UPDATE_RENDER_PASS = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_GET_TRANSFORMATION_MATRIX = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_HANDLE_SHADOW_MAP = pragma::INVALID_COMPONENT_ID;
ComponentEventId CLightComponent::EVENT_ON_SHADOW_BUFFER_INITIALIZED = pragma::INVALID_COMPONENT_ID;
void CLightComponent::RegisterEvents(pragma::EntityComponentManager &componentManager)
{
	EVENT_SHOULD_PASS_ENTITY = componentManager.RegisterEvent("SHOULD_PASS_ENTITY",std::type_index(typeid(CLightComponent)));
	EVENT_SHOULD_PASS_ENTITY_MESH = componentManager.RegisterEvent("SHOULD_PASS_ENTITY_MESH",std::type_index(typeid(CLightComponent)));
	EVENT_SHOULD_PASS_MESH = componentManager.RegisterEvent("SHOULD_PASS_MESH",std::type_index(typeid(CLightComponent)));
	EVENT_SHOULD_UPDATE_RENDER_PASS = componentManager.RegisterEvent("SHOULD_UPDATE_RENDER_PASS",std::type_index(typeid(CLightComponent)));
	EVENT_GET_TRANSFORMATION_MATRIX = componentManager.RegisterEvent("GET_TRANSFORMATION_MATRIX",std::type_index(typeid(CLightComponent)));
	EVENT_HANDLE_SHADOW_MAP = componentManager.RegisterEvent("HANDLE_SHADOW_MAP");
	EVENT_ON_SHADOW_BUFFER_INITIALIZED = componentManager.RegisterEvent("ON_SHADOW_BUFFER_INITIALIZED");
}
void CLightComponent::InitializeBuffers()
{
	pragma::LightDataBufferManager::GetInstance().Initialize();
	pragma::ShadowDataBufferManager::GetInstance().Initialize();
}
CLightComponent *CLightComponent::GetLightByBufferIndex(uint32_t idx) {return LightDataBufferManager::GetInstance().GetLightByBufferIndex(idx);}
CLightComponent *CLightComponent::GetLightByShadowBufferIndex(uint32_t idx) {return ShadowDataBufferManager::GetInstance().GetLightByBufferIndex(idx);}
void CLightComponent::ClearBuffers()
{
	LightDataBufferManager::GetInstance().Reset();
	ShadowDataBufferManager::GetInstance().Reset();
}

CLightComponent::CLightComponent(BaseEntity &ent)
	: CBaseLightComponent(ent),m_stateFlags{StateFlags::StaticUpdateRequired | StateFlags::FullUpdateRequired | StateFlags::AddToGameScene}
{}
CLightComponent::~CLightComponent()
{
	--s_lightCount;
	DestroyRenderBuffer();
	DestroyShadowBuffer();
}
luabind::object CLightComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CLightComponentHandleWrapper>(l);}
void CLightComponent::InitializeRenderBuffer()
{
	if(m_renderBuffer != nullptr)
		return;
	m_renderBuffer = LightDataBufferManager::GetInstance().Request(*this,m_bufferData);
}

void CLightComponent::InitializeShadowBuffer()
{
	if(m_shadowBuffer != nullptr)
		return;
	m_shadowBuffer = ShadowDataBufferManager::GetInstance().Request(*this,*m_shadowBufferData);
	BroadcastEvent(EVENT_ON_SHADOW_BUFFER_INITIALIZED,CEOnShadowBufferInitialized{*m_shadowBuffer});
}

void CLightComponent::DestroyRenderBuffer()
{
	//m_bufferUpdateInfo.clear(); // prosper TODO
	if(m_renderBuffer == nullptr)
		return;
	umath::set_flag(m_bufferData.flags,LightBufferData::BufferFlags::TurnedOn,false);
	if(m_renderBuffer != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);

	LightDataBufferManager::GetInstance().Free(m_renderBuffer);
	m_renderBuffer = nullptr;
}

void CLightComponent::DestroyShadowBuffer()
{
	if(m_shadowBuffer == nullptr)
		return;
	ShadowDataBufferManager::GetInstance().Free(m_shadowBuffer);
	m_shadowBuffer = nullptr;
}

bool CLightComponent::ShouldRender() {return true;}

bool CLightComponent::ShouldPass(const Model &mdl,const CModelSubMesh &mesh)
{
	auto &materials = mdl.GetMaterials();
	auto texId = mdl.GetMaterialIndex(mesh);
	if(texId.has_value() == false || *texId >= materials.size() || !materials[*texId].IsValid()) // Ignore meshes with invalid materials
		return false;
	auto &mat = materials[*texId];
	auto *info = mat.get()->GetShaderInfo();
	if(info == nullptr || const_cast<util::ShaderInfo*>(info)->GetShader() == nullptr) // Ignore meshes with nodraw (Or invalid) shaders
		return false;
	CEShouldPassMesh evData {mdl,mesh};
	InvokeEventCallbacks(EVENT_SHOULD_PASS_MESH,evData);
	return evData.shouldPass;
}

void CLightComponent::InitializeLight(BaseEntityComponent &component)
{
	CBaseLightComponent::InitializeLight(component);
}

bool CLightComponent::ShouldPass(const CBaseEntity &ent,uint32_t &renderFlags)
{
	if(ShouldCastShadows() == false)
		return false;
	CEShouldPassEntity evData {ent,renderFlags};
	if(InvokeEventCallbacks(EVENT_SHOULD_PASS_ENTITY,evData) == util::EventReply::Handled)
		return evData.shouldPass;
	return true;
}
bool CLightComponent::ShouldPass(const CBaseEntity &ent,const CModelMesh &mesh,uint32_t &renderFlags)
{
	if(ShouldCastShadows() == false)
		return false;
	CEShouldPassEntityMesh evData {ent,mesh,renderFlags};
	InvokeEventCallbacks(EVENT_SHOULD_PASS_ENTITY_MESH,evData);
	return evData.shouldPass;
}

Scene *CLightComponent::FindShadowScene() const
{
	auto sceneFlags = static_cast<const CBaseEntity&>(GetEntity()).GetSceneFlags();
	// A shadowed light source should always only be assigned to one scene slot, so
	// we'll just pick whichever is the first
	auto lowestBit = static_cast<int32_t>(sceneFlags) &-static_cast<int32_t>(sceneFlags);
	return Scene::GetByIndex(lowestBit);
}
COcclusionCullerComponent *CLightComponent::FindShadowOcclusionCuller() const
{
	auto *scene = FindShadowScene();
	return scene ? scene->FindOcclusionCuller() : nullptr;
}

bool CLightComponent::IsInCone(const CBaseEntity &ent,const Vector3 &dir,float angle) const
{
	auto pRenderComponent = ent.GetRenderComponent();
	auto pTrComponent = ent.GetTransformComponent();
	auto pTrComponentThis = GetEntity().GetTransformComponent();
	if(pRenderComponent.expired() || pTrComponent.expired() || pTrComponentThis.expired())
		return false;
	auto &start = pTrComponentThis->GetPosition();
	auto sphere = pRenderComponent->GetRenderSphereBounds();
	return Intersection::SphereCone(pTrComponent->GetPosition() +sphere.pos,sphere.radius,start,dir,angle);
}
void CLightComponent::SetLightIntensity(float intensity,LightIntensityType type)
{
	CBaseLightComponent::SetLightIntensity(intensity,type);
	UpdateLightIntensity();
}
void CLightComponent::SetLightIntensityType(LightIntensityType type)
{
	CBaseLightComponent::SetLightIntensityType(type);
	UpdateLightIntensity();
}
void CLightComponent::UpdateLightIntensity()
{
	m_bufferData.intensity = GetLightIntensityCandela();
	if(m_renderBuffer == nullptr)
		return;
	c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,intensity),m_bufferData.intensity);
}
bool CLightComponent::IsInRange(const CBaseEntity &ent) const
{
	auto pRadiusComponent = GetEntity().GetComponent<CRadiusComponent>();
	auto pRenderComponent = ent.GetRenderComponent();
	auto pTrComponent = ent.GetTransformComponent();
	auto pTrComponentThis = GetEntity().GetTransformComponent();
	if(pRadiusComponent.expired() || pRenderComponent.expired() || pTrComponent.expired() || pTrComponentThis.expired())
		return false;
	auto &origin = pTrComponentThis->GetPosition();
	auto sphere = pRenderComponent->GetRenderSphereBounds();
	auto &pos = pTrComponent->GetPosition();
	auto radius = pRadiusComponent->GetRadius();
	return (uvec::distance(pos +sphere.pos,origin) <= (radius +sphere.radius)) ? true : false;
}
bool CLightComponent::IsInRange(const CBaseEntity &ent,const CModelMesh &mesh) const
{
	auto pRadiusComponent = GetEntity().GetComponent<CRadiusComponent>();
	auto pTrComponent = ent.GetTransformComponent();
	auto pTrComponentThis = GetEntity().GetTransformComponent();
	if(pRadiusComponent.expired() || pTrComponent.expired() || pTrComponentThis.expired())
		return false;
	auto &origin = pTrComponentThis->GetPosition();
	auto radius = pRadiusComponent->GetRadius();
	auto &pos = pTrComponent->GetPosition();
	Vector3 min;
	Vector3 max;
	mesh.GetBounds(min,max);
	min += pos;
	max += pos;
	return Intersection::AABBSphere(min,max,origin,radius);
}

bool CLightComponent::ShouldUpdateRenderPass(ShadowMapType smType) const
{
	CEShouldUpdateRenderPass evData {};
	if(InvokeEventCallbacks(EVENT_SHOULD_UPDATE_RENDER_PASS,evData) == util::EventReply::Handled)
		return evData.shouldUpdate;
	return umath::is_flag_set(m_stateFlags,(smType == ShadowMapType::Static) ? StateFlags::StaticUpdateRequired : StateFlags::DynamicUpdateRequired);
}

void CLightComponent::UpdateBuffers()
{
	if(m_renderBuffer != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,0ull,m_bufferData);
	if(m_shadowBuffer != nullptr && m_shadowBufferData != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_shadowBuffer,0ull,m_shadowBufferData);
}
void CLightComponent::UpdateShadowTypes()
{
	auto b = ShouldCastShadows();
	auto shadowIndex = 0u;
	if(b == true)
	{
		InitializeShadowBuffer();
		if(m_shadowBuffer != nullptr)
		{
			shadowIndex = m_shadowBuffer->GetBaseIndex() +1u;
			if(m_bufferData.shadowIndex == shadowIndex)
				return;
		}
	}
	else
	{
		DestroyShadowBuffer();
		if(m_bufferData.shadowIndex == 0)
			return;
	}
	m_bufferData.shadowIndex = shadowIndex;
	if(m_renderBuffer != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,shadowIndex),m_bufferData.shadowIndex);
}
bool CLightComponent::ShouldCastShadows() const {return GetShadowType() != ShadowType::None;}
bool CLightComponent::ShouldCastDynamicShadows() const {return GetShadowType() == ShadowType::Full;}
bool CLightComponent::ShouldCastStaticShadows() const {return ShouldCastShadows();}
void CLightComponent::SetShadowType(ShadowType type)
{
	if(type == GetShadowType())
		return;
	CBaseLightComponent::SetShadowType(type);
	if(type != ShadowType::None)
		InitializeShadowMap();
	else
	{
		m_shadowMapStatic = {};
		m_shadowMapDynamic = {};
	}
	UpdateShadowTypes(); // Has to be called AFTER the shadowmap has been initialized!
}

void CLightComponent::SetFalloffExponent(float falloffExponent)
{
	if(falloffExponent == m_bufferData.falloffExponent)
		return;
	BaseEnvLightComponent::SetFalloffExponent(falloffExponent);
	m_bufferData.falloffExponent = falloffExponent;
	if(m_renderBuffer != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,falloffExponent),m_bufferData.falloffExponent);
}

uint32_t CLightComponent::GetShadowMapIndex(ShadowMapType smType) const
{
	switch(smType)
	{
	case ShadowMapType::Dynamic:
		return m_bufferData.shadowMapIndexDynamic;
	case ShadowMapType::Static:
		return m_bufferData.shadowMapIndexStatic;
	}
	return 0u;
}

void CLightComponent::SetShadowMapIndex(uint32_t idx,ShadowMapType smType)
{
	idx = (idx == std::numeric_limits<uint32_t>::max()) ? 0u : (idx +1);
	auto &target = (smType == ShadowMapType::Dynamic) ? m_bufferData.shadowMapIndexDynamic : m_bufferData.shadowMapIndexStatic;
	if(idx == target)
		return;
	target = idx;
	if(m_renderBuffer != nullptr)
	{
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(
			m_renderBuffer,(smType == ShadowMapType::Dynamic) ? offsetof(LightBufferData,shadowMapIndexDynamic) : offsetof(LightBufferData,shadowMapIndexStatic)
			,target
		);
	}
}

void CLightComponent::InitializeShadowMap(CShadowComponent &sm)
{
	sm.SetTextureReloadCallback([this]() {
		UpdateShadowTypes();
	});
	UpdateShadowTypes();
}

void CLightComponent::InitializeShadowMap()
{
	if(GetShadowType() == ShadowType::None)
		return;
	CEHandleShadowMap ceData {};
	if(BroadcastEvent(EVENT_HANDLE_SHADOW_MAP,ceData) == util::EventReply::Unhandled)
		m_shadowMapStatic = GetEntity().AddComponent<CShadowComponent>(true);
	else if(ceData.resultShadow)
		m_shadowMapStatic = ceData.resultShadow->GetHandle<CShadowComponent>();
	if(m_shadowMapStatic.valid())
		InitializeShadowMap(*m_shadowMapStatic);
	if(GetShadowType() == ShadowType::Full)
	{
		CEHandleShadowMap ceData {};
		if(BroadcastEvent(EVENT_HANDLE_SHADOW_MAP,ceData) == util::EventReply::Unhandled)
			m_shadowMapDynamic = GetEntity().AddComponent<CShadowComponent>(true);
		else if(ceData.resultShadow)
			m_shadowMapDynamic = ceData.resultShadow->GetHandle<CShadowComponent>();
		if(m_shadowMapDynamic.valid())
			InitializeShadowMap(*m_shadowMapDynamic);
	}
}

void CLightComponent::SetStateFlag(StateFlags flag,bool enabled) {umath::set_flag(m_stateFlags,flag,enabled);}

void CLightComponent::Initialize()
{
	CBaseLightComponent::Initialize();

	auto &ent = static_cast<CBaseEntity&>(GetEntity());
	ent.AddComponent<LogicComponent>();
	ent.AddComponent<CShadowComponent>();

	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_ON,[this](std::reference_wrapper<ComponentEvent> evData) {
		umath::set_flag(m_bufferData.flags,LightBufferData::BufferFlags::TurnedOn,true);
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
		else
			InitializeRenderBuffer();
		// TODO: This will update all light and shadow buffers for this light source.
		// This shouldn't be necessary, but without light sources seem to have incorrect buffer
		// data when turned on. Once the cause for this has been found and dealt with, this
		// line can be removed!
		UpdateBuffers();
	});
	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_OFF,[this](std::reference_wrapper<ComponentEvent> evData) {
		umath::set_flag(m_bufferData.flags,LightBufferData::BufferFlags::TurnedOn,false);
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
		m_tTurnedOff = c_game->RealTime();
	});
	BindEventUnhandled(LogicComponent::EVENT_ON_TICK,[this](std::reference_wrapper<ComponentEvent> evData) {
		auto frameId = c_engine->GetRenderContext().GetLastFrameId();
		if(m_lastThink == frameId)
			return;
		m_lastThink = frameId;

		if(m_renderBuffer != nullptr && c_game->RealTime() -m_tTurnedOff > 30.0)
		{
			auto pToggleComponent = GetEntity().GetComponent<CToggleComponent>();
			if(pToggleComponent.expired() || pToggleComponent->IsTurnedOn() == false)
				DestroyRenderBuffer(); // Free buffer if light hasn't been on in 30 seconds
		}
	});
	BindEventUnhandled(CBaseEntity::EVENT_ON_SCENE_FLAGS_CHANGED,[this](std::reference_wrapper<ComponentEvent> evData) {
		m_bufferData.sceneFlags = static_cast<CBaseEntity&>(GetEntity()).GetSceneFlags();
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,sceneFlags),m_bufferData.sceneFlags);
	});
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.valid())
		reinterpret_cast<Vector3&>(m_bufferData.position) = pTrComponent->GetPosition();
	if(m_bufferData.direction.x == 0.f && m_bufferData.direction.y == 0.f && m_bufferData.direction.z == 0.f)
		m_bufferData.direction.z = 1.f;
	m_bufferData.sceneFlags = ent.GetSceneFlags();

	++s_lightCount;
}
void CLightComponent::UpdateTransformationMatrix(const Mat4 &biasMatrix,const Mat4 &viewMatrix,const Mat4 &projectionMatrix)
{
	if(m_shadowBufferData != nullptr)
	{
		m_shadowBufferData->view = viewMatrix;
		m_shadowBufferData->projection = projectionMatrix;
		m_shadowBufferData->depthVP = biasMatrix;
	}
	if(m_shadowBuffer != nullptr)
		c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_shadowBuffer,offsetof(ShadowBufferData,depthVP),biasMatrix);
}
void CLightComponent::OnEntityComponentAdded(BaseEntityComponent &component)
{
	CBaseLightComponent::OnEntityComponentAdded(component);
	if(typeid(component) == typeid(CTransformComponent))
	{
		FlagCallbackForRemoval(static_cast<CTransformComponent&>(component).GetPosProperty()->AddCallback([this](std::reference_wrapper<const Vector3> oldPos,std::reference_wrapper<const Vector3> pos) {
			if(uvec::cmp(pos.get(),reinterpret_cast<Vector3&>(m_bufferData.position)) == true)
				return;
			reinterpret_cast<Vector3&>(m_bufferData.position) = pos;
			if(m_renderBuffer != nullptr)
				c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,position),m_bufferData.position);
			umath::set_flag(m_stateFlags,StateFlags::FullUpdateRequired);
		}),CallbackType::Component,&component);
		FlagCallbackForRemoval(static_cast<CTransformComponent&>(component).GetOrientationProperty()->AddCallback([this](std::reference_wrapper<const Quat> oldRot,std::reference_wrapper<const Quat> rot) {
			util::pragma::LightType lightType;
			GetLight(lightType);
			if(lightType == util::pragma::LightType::Point)
				return;
			auto dir = uquat::forward(rot);
			if(uvec::cmp(dir,reinterpret_cast<Vector3&>(m_bufferData.direction)) == true)
				return;
			reinterpret_cast<Vector3&>(m_bufferData.direction) = dir;
			if(m_bufferData.direction.x == 0.f && m_bufferData.direction.y == 0.f && m_bufferData.direction.z == 0.f)
				m_bufferData.direction.z = 1.f;
			if(m_renderBuffer != nullptr)
				c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,direction),m_bufferData.direction);
			umath::set_flag(m_stateFlags,StateFlags::FullUpdateRequired);
		}),CallbackType::Component,&component);
	}
	else if(typeid(component) == typeid(CRadiusComponent))
	{
		FlagCallbackForRemoval(static_cast<CRadiusComponent&>(component).GetRadiusProperty()->AddCallback([this](std::reference_wrapper<const float> oldRadius,std::reference_wrapper<const float> radius) {
			if(radius == m_bufferData.position.w)
				return;
			m_bufferData.position.w = radius;
			if(m_renderBuffer != nullptr)
				c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,position) +offsetof(Vector4,w),m_bufferData.position.w);
			umath::set_flag(m_stateFlags,StateFlags::FullUpdateRequired);
		}),CallbackType::Component,&component);
	}
	else if(typeid(component) == typeid(CColorComponent))
	{
		FlagCallbackForRemoval(static_cast<CColorComponent&>(component).GetColorProperty()->AddCallback([this](std::reference_wrapper<const Color> oldColor,std::reference_wrapper<const Color> color) {
			m_bufferData.color = color.get().ToVector3();
			if(m_renderBuffer != nullptr)
				c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,color),m_bufferData.color);

			if(color.get().a == 0 || (color.get().r == 0 && color.get().g == 0 && color.get().b == 0))
				umath::set_flag(m_bufferData.flags,LightBufferData::BufferFlags::TurnedOn,false);
			else
			{
				auto pToggleComponent = GetEntity().GetComponent<CToggleComponent>();
				if(pToggleComponent.expired() || pToggleComponent->IsTurnedOn() == true)
					umath::set_flag(m_bufferData.flags,LightBufferData::BufferFlags::TurnedOn,true);
			}
		}),CallbackType::Component,&component);
	}
	else if(typeid(component) == typeid(CLightSpotComponent))
	{
		m_bufferData.flags &= ~(LightBufferData::BufferFlags::TypeSpot | LightBufferData::BufferFlags::TypePoint | LightBufferData::BufferFlags::TypeDirectional);
		m_bufferData.flags |= LightBufferData::BufferFlags::TypeSpot;
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
	}
	else if(typeid(component) == typeid(CLightPointComponent))
	{
		m_bufferData.flags &= ~(LightBufferData::BufferFlags::TypeSpot | LightBufferData::BufferFlags::TypePoint | LightBufferData::BufferFlags::TypeDirectional);
		m_bufferData.flags |= LightBufferData::BufferFlags::TypePoint;
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
	}
	else if(typeid(component) == typeid(CLightDirectionalComponent))
	{
		m_bufferData.flags &= ~(LightBufferData::BufferFlags::TypeSpot | LightBufferData::BufferFlags::TypePoint | LightBufferData::BufferFlags::TypeDirectional);
		m_bufferData.flags |= LightBufferData::BufferFlags::TypeDirectional;
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
	}
}
void CLightComponent::OnEntitySpawn()
{
	CBaseLightComponent::OnEntitySpawn();
	InitializeShadowMap();

	if(umath::is_flag_set(m_lightFlags,LightFlags::BakedLightSource))
	{
		m_bufferData.flags |= LightBufferData::BufferFlags::BakedLightSource;
		if(m_renderBuffer != nullptr)
			c_engine->GetRenderContext().ScheduleRecordUpdateBuffer(m_renderBuffer,offsetof(LightBufferData,flags),m_bufferData.flags);
	}
}

const pragma::LightBufferData &CLightComponent::GetBufferData() const {return const_cast<CLightComponent*>(this)->GetBufferData();}
pragma::LightBufferData &CLightComponent::GetBufferData() {return m_bufferData;}
const pragma::ShadowBufferData *CLightComponent::GetShadowBufferData() const {return const_cast<CLightComponent*>(this)->GetShadowBufferData();}
pragma::ShadowBufferData *CLightComponent::GetShadowBufferData() {return m_shadowBufferData.get();}

util::WeakHandle<CShadowComponent> CLightComponent::GetShadowMap(ShadowMapType type) const {return (type == ShadowMapType::Dynamic) ? m_shadowMapDynamic : m_shadowMapStatic;}

Mat4 &CLightComponent::GetTransformationMatrix(unsigned int j)
{
	CEGetTransformationMatrix evData{j};
	InvokeEventCallbacks(EVENT_GET_TRANSFORMATION_MATRIX,evData);
	if(evData.transformation != nullptr)
		return *evData.transformation;
	static Mat4 m;
	m = umat::identity();
	return m;
}

const std::shared_ptr<prosper::IBuffer> &CLightComponent::GetRenderBuffer() const {return m_renderBuffer;}
const std::shared_ptr<prosper::IBuffer> &CLightComponent::GetShadowBuffer() const {return m_shadowBuffer;}
void CLightComponent::SetRenderBuffer(const std::shared_ptr<prosper::IBuffer> &renderBuffer) {m_renderBuffer = renderBuffer;}
void CLightComponent::SetShadowBuffer(const std::shared_ptr<prosper::IBuffer> &renderBuffer) {m_shadowBuffer = renderBuffer;}

///////////////////

void Console::commands::debug_light_sources(NetworkState *state,pragma::BasePlayerComponent *pl,std::vector<std::string> &argv)
{
	EntityIterator entIt {*c_game};
	entIt.AttachFilter<TEntityIteratorFilterComponent<CLightComponent>>();
	std::vector<pragma::CLightComponent*> lights;
	lights.reserve(entIt.GetCount());
	for(auto *ent : entIt)
		lights.push_back(ent->GetComponent<CLightComponent>().get());
	
	auto numLights = 0u;
	auto numTurnedOn = 0u;
	std::vector<size_t> discrepancies {};
	for(auto *l : lights)
	{
		++numLights;
		auto pToggleComponent = l->GetEntity().GetComponent<CToggleComponent>();
		if(pToggleComponent.expired() || pToggleComponent->IsTurnedOn() == true)
			++numTurnedOn;
	}

	auto lightId = 0u;
	for(auto *l : lights)
	{
		Con::cout<<"Light #"<<lightId<<":"<<Con::endl;
		Con::cout<<"\tType: ";
		auto type = util::pragma::LightType::Undefined;
		auto *pLight = l->GetLight(type);
		switch(type)
		{
			case util::pragma::LightType::Directional:
				Con::cout<<"Directional";
				break;
			case util::pragma::LightType::Point:
				Con::cout<<"Point";
				break;
			case util::pragma::LightType::Spot:
				Con::cout<<"Spot";
				break;
			default:
				Con::cout<<"Unknown";
				break;
		}
		Con::cout<<Con::endl;

		auto &buf = l->GetRenderBuffer();
		if(buf == nullptr)
			Con::cout<<"\tBuffer: NULL"<<Con::endl;
		else
		{
			LightBufferData data;
			buf->Read(0ull,sizeof(data),&data);
			std::string type = "Unknown";
			if((data.flags &LightBufferData::BufferFlags::TypeSpot) != LightBufferData::BufferFlags::None)
				type = "Spot";
			else if((data.flags &LightBufferData::BufferFlags::TypePoint) != LightBufferData::BufferFlags::None)
				type = "Point";
			else if((data.flags &LightBufferData::BufferFlags::TypeDirectional) != LightBufferData::BufferFlags::None)
				type = "Directional";
			Con::cout<<"\t\tPosition: ("<<data.position.x<<","<<data.position.y<<","<<data.position.z<<")"<<Con::endl;
			Con::cout<<"\t\tShadow Index: "<<data.shadowIndex<<Con::endl;
			Con::cout<<"\t\tShadow Map Index (static): "<<data.shadowMapIndexStatic<<Con::endl;
			Con::cout<<"\t\tShadow Map Index (dynamic): "<<data.shadowMapIndexDynamic<<Con::endl;
			Con::cout<<"\t\tType: "<<type<<Con::endl;
			Con::cout<<"\t\tColor: ("<<data.color.r<<","<<data.color.g<<","<<data.color.b<<")"<<Con::endl;
			Con::cout<<"\t\tIntensity (candela): "<<data.intensity<<Con::endl;
			Con::cout<<"\t\tDirection: ("<<data.direction.x<<","<<data.direction.y<<","<<data.direction.z<<")"<<Con::endl;
			Con::cout<<"\t\tCone Start Offset: "<<data.direction.w<<Con::endl;
			Con::cout<<"\t\tDistance: "<<data.position.w<<Con::endl;
			Con::cout<<"\t\tOuter cutoff angle (cosine): "<<data.cutoffOuterCos<<Con::endl;
			Con::cout<<"\t\tInner cutoff angle (cosine): "<<data.cutoffInnerCos<<Con::endl;
			Con::cout<<"\t\tAttenuation: "<<data.attenuation<<Con::endl;
			Con::cout<<"\t\tFlags: "<<umath::to_integral(data.flags)<<Con::endl;
			Con::cout<<"\t\tTurned On: "<<(((data.flags &LightBufferData::BufferFlags::TurnedOn) == LightBufferData::BufferFlags::TurnedOn) ? "Yes" : "No")<<Con::endl;
		}
		++lightId;
	}
	Con::cout<<"Number of lights: "<<numLights<<Con::endl;
	Con::cout<<"Turned on: "<<numTurnedOn<<Con::endl;
	if(discrepancies.empty() == false)
	{
		Con::cwar<<"Discrepancies found in "<<discrepancies.size()<<" lights:"<<Con::endl;
		for(auto idx : discrepancies)
			Con::cout<<"\t"<<idx<<Con::endl;
	}
}

/////////////////

CEShouldPassEntity::CEShouldPassEntity(const CBaseEntity &entity,uint32_t &renderFlags)
	: entity{entity},renderFlags{renderFlags}
{}
void CEShouldPassEntity::PushArguments(lua_State *l) {}

/////////////////

CEShouldPassMesh::CEShouldPassMesh(const Model &model,const CModelSubMesh &mesh)
	: model{model},mesh{mesh}
{}
void CEShouldPassMesh::PushArguments(lua_State *l) {}

/////////////////

CEShouldPassEntityMesh::CEShouldPassEntityMesh(const CBaseEntity &entity,const CModelMesh &mesh,uint32_t &renderFlags)
	: entity{entity},mesh{mesh},renderFlags{renderFlags}
{}
void CEShouldPassEntityMesh::PushArguments(lua_State *l) {}

/////////////////

CEShouldUpdateRenderPass::CEShouldUpdateRenderPass()
{}
void CEShouldUpdateRenderPass::PushArguments(lua_State *l) {}

/////////////////

CEGetTransformationMatrix::CEGetTransformationMatrix(uint32_t index)
	: index{index}
{}
void CEGetTransformationMatrix::PushArguments(lua_State *l) {}

/////////////////

CEHandleShadowMap::CEHandleShadowMap()
{}
void CEHandleShadowMap::PushArguments(lua_State *l) {}

/////////////////

CEOnShadowBufferInitialized::CEOnShadowBufferInitialized(prosper::IBuffer &shadowBuffer)
	: shadowBuffer{shadowBuffer}
{}
void CEOnShadowBufferInitialized::PushArguments(lua_State *l)
{
	Lua::Push<std::shared_ptr<Lua::Vulkan::Buffer>>(l,shadowBuffer.shared_from_this());
}
#pragma optimize("",on)
