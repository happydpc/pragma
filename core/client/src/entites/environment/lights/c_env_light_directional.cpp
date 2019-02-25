#include "stdafx_client.h"
#include "pragma/entities/environment/lights/c_env_light_directional.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/rendering/lighting/c_light_directional.h"
#include "pragma/entities/baseentity_luaobject.h"
#include "pragma/rendering/uniformbinding.h"
#include "pragma/rendering/world_environment.hpp"
#include "pragma/entities/components/c_color_component.hpp"
#include "pragma/lua/c_lentity_handles.hpp"
#include "pragma/rendering/lighting/shadows/c_shadowmapcasc.h"
#include "pragma/model/c_modelmesh.h"
#include <pragma/math/intersection.h>
#include <pragma/entities/components/base_transform_component.hpp>

extern DLLCLIENT CGame *c_game;

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_light_environment,CEnvLightDirectional);

void CLightDirectionalComponent::Initialize()
{
	BaseEnvLightDirectionalComponent::Initialize();

	BindEvent(CLightComponent::EVENT_GET_TRANSFORMATION_MATRIX,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &trData = static_cast<CEGetTransformationMatrix&>(evData.get());
		trData.transformation = &MVPBias<1>::GetTransformationMatrix(trData.index);
		return util::EventReply::Handled;
	});
	BindEvent(CLightComponent::EVENT_SHOULD_UPDATE_RENDER_PASS,[](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		static_cast<CEShouldUpdateRenderPass&>(evData.get()).shouldUpdate = true; // CSM Update requirements are determined through ShadowMapCasc::ShouldUpdateLayer
		return util::EventReply::Handled;
	});
	BindEvent(CLightComponent::EVENT_SHOULD_PASS_ENTITY,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &shouldPassData = static_cast<CEShouldPassEntity&>(evData.get());
		auto &ent = shouldPassData.entity;
		auto pTrComponent = ent.GetTransformComponent();
		auto pRenderComponent = ent.GetRenderComponent();
		if(pTrComponent.expired() || pRenderComponent.expired())
		{
			shouldPassData.shouldPass = false;
			return util::EventReply::Handled;
		}
		auto &pos = pTrComponent->GetPosition();
		Vector3 min;
		Vector3 max;
		pRenderComponent->GetRenderBounds(&min,&max);
		min += pos;
		max += pos;

		auto pLightComponent = GetEntity().GetComponent<CLightComponent>();
		auto *pShadowMap = pLightComponent.valid() ? pLightComponent->GetShadowMap() : nullptr;
		if(pShadowMap == nullptr || pShadowMap->GetType() != ShadowMap::Type::Cascaded)
			return util::EventReply::Unhandled;
		auto &shadowMap = static_cast<ShadowMapCasc&>(*pShadowMap);
		auto numLayers = shadowMap.GetLayerCount();
		for(auto i=decltype(numLayers){0};i<numLayers;++i)
		{
			auto &frustum = *shadowMap.GetFrustumSplit(i);
			if(Intersection::AABBAABB(min,max,frustum.aabb.min +frustum.obbCenter,frustum.aabb.max +frustum.obbCenter) != INTERSECT_OUTSIDE)
				shouldPassData.renderFlags |= 1<<i;
		}
		if(shouldPassData.renderFlags == 0)
		{
			shouldPassData.shouldPass = false;
			return util::EventReply::Handled;
		}
		return util::EventReply::Unhandled;
	});
	BindEvent(CLightComponent::EVENT_SHOULD_PASS_ENTITY_MESH,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &shouldPassData = static_cast<CEShouldPassEntityMesh&>(evData.get());
		auto &ent = static_cast<CBaseEntity&>(GetEntity());
		auto pLightComponent = ent.GetComponent<CLightComponent>();
		auto *pShadowMap = pLightComponent.valid() ? pLightComponent->GetShadowMap() : nullptr;
		if(pShadowMap != nullptr && pShadowMap->GetType() == ShadowMap::Type::Cascaded)
		{
			auto &entTgt = shouldPassData.entity;
			auto pTrComponent = entTgt.GetTransformComponent();
			if(pTrComponent.expired())
			{
				shouldPassData.shouldPass = false;
				return util::EventReply::Handled;
			}
			auto &pos = pTrComponent->GetPosition();
			Vector3 min;
			Vector3 max;
			shouldPassData.mesh.GetBounds(min,max);
			min += pos;
			max += pos;

			auto &shadowMap = static_cast<ShadowMapCasc&>(*pShadowMap);
			auto numLayers = pShadowMap->GetLayerCount();
			for(auto i=decltype(numLayers){0};i<numLayers;++i)
			{
				auto &frustum = *shadowMap.GetFrustumSplit(i);
				if(Intersection::AABBAABB(min,max,frustum.aabb.min +frustum.obbCenter,frustum.aabb.max +frustum.obbCenter) != INTERSECT_OUTSIDE)
					shouldPassData.renderFlags |= 1<<i;
			}
		}
		if(shouldPassData.renderFlags == 0)
		{
			shouldPassData.shouldPass = false;
			return util::EventReply::Handled;
		}
		return util::EventReply::Unhandled;
	});
	BindEvent(CLightComponent::EVENT_HANDLE_SHADOW_MAP,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &shadowMap = static_cast<CEHandleShadowMap&>(evData.get()).shadowMap = std::make_unique<ShadowMapCasc>();
		static_cast<ShadowMapCasc&>(*shadowMap).SetFrustumUpdateCallback([this]() {
			//m_bFullUpdateRequired = true;
		});
		return util::EventReply::Handled;
	});

	auto &ent = GetEntity();
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.valid())
	{
		Vector3 pos = pTrComponent->GetPosition();
		auto dir = pTrComponent->GetForward();
		SetViewMatrix(glm::lookAtRH(pos,pos +dir,uvec::get_perpendicular(dir)));
		FlagCallbackForRemoval(pTrComponent->GetOrientationProperty()->AddCallback([this](std::reference_wrapper<const Quat> oldRot,std::reference_wrapper<const Quat> rot) {
			auto &ent = GetEntity();
			auto pLightComponent = ent.GetComponent<CLightComponent>();
			auto type = LightType::Invalid;
			auto *pLight = pLightComponent.valid() ? pLightComponent->GetLight(type) : nullptr;
			if(pLight == nullptr || type != LightType::Directional)
				return;
			auto pTrComponent = pLight->GetEntity().GetTransformComponent();
			if(pTrComponent.valid())
				pTrComponent->SetOrientation(rot);
		}),CallbackType::Entity);
	}

	auto pLightComponent = ent.GetComponent<CLightComponent>();
	if(pLightComponent.valid())
	{
		pLightComponent->UpdateTransformationMatrix(GetBiasTransformationMatrix(),GetViewMatrix(),GetProjectionMatrix());
		pLightComponent->SetStateFlag(CLightComponent::StateFlags::UseDualTextureSet,false);
	}

	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_ON,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		if(GetEntity().IsSpawned() == false)
			return;
		c_game->UpdateEnvironmentLightSource();
	});
	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_OFF,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		if(GetEntity().IsSpawned() == false)
			return;
		c_game->UpdateEnvironmentLightSource();
	});
}
void CLightDirectionalComponent::OnEntitySpawn()
{
	BaseEnvLightDirectionalComponent::OnEntitySpawn();
	auto pToggleComponent = GetEntity().GetComponent<CToggleComponent>();
	if(pToggleComponent.expired() || pToggleComponent->IsTurnedOn())
		c_game->UpdateEnvironmentLightSource();
}
void CLightDirectionalComponent::ReceiveData(NetPacket &packet)
{
	auto r = packet->Read<short>();
	auto g = packet->Read<short>();
	auto b = packet->Read<short>();
	auto a = packet->Read<short>();
	*m_ambientColor = Color{r,g,b,a};
	m_maxExposure = packet->Read<Float>();
}
Bool CLightDirectionalComponent::ReceiveNetEvent(pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netEvSetAmbientColor)
	{
		Color color;
		color.r = packet->Read<short>();
		color.g = packet->Read<short>();
		color.b = packet->Read<short>();
		color.a = packet->Read<short>();
		SetAmbientColor(color);
	}
	else
		return CBaseNetComponent::ReceiveNetEvent(eventId,packet);
	return true;
}
util::EventReply CLightDirectionalComponent::HandleEvent(ComponentEventId eventId,ComponentEvent &evData)
{
	if(BaseEnvLightDirectionalComponent::HandleEvent(eventId,evData) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(eventId == BaseToggleComponent::EVENT_ON_TURN_ON)
		UpdateAmbientColor();
	return util::EventReply::Unhandled;
}

void CLightDirectionalComponent::SetAmbientColor(const Color &color)
{
	BaseEnvLightDirectionalComponent::SetAmbientColor(color);
	UpdateAmbientColor();
}

void CLightDirectionalComponent::UpdateAmbientColor()
{
	auto pToggleComponent = GetEntity().GetComponent<CToggleComponent>();
	if(pToggleComponent.valid() && pToggleComponent->IsTurnedOn() == false)
		return;
	c_game->GetWorldEnvironment().SetAmbientColor((*m_ambientColor)->ToVector4());
	c_game->SetMaxHDRExposure(m_maxExposure);
}
luabind::object CLightDirectionalComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CLightDirectionalComponentHandleWrapper>(l);}

//////////////

void CEnvLightDirectional::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CLightComponent>();
	AddComponent<CLightDirectionalComponent>();
}

//////////////

void CLightDirectionalComponent::RenderStaticWorldGeometry()
{
	 // prosper TODO
#if 0
	m_cmdShadowBuffers.clear();
	auto *entWorld = c_game->GetWorld();
	if(entWorld == nullptr)
		return;
	auto hShaderShadow = c_engine->GetShader("shadowcsmstatic");
	if(!hShaderShadow.IsValid())
		return;
	auto hMdl = entWorld->GetModel();
	if(hMdl == nullptr)
		return;
	auto &model = *hMdl.get();
	auto *shadow = GetShadowMap();
	if(shadow == nullptr)
		return;
	auto *tex = shadow->GetDepthTexture(); // TODO
	if(tex == nullptr)
		return;
	auto &shaderShadow = static_cast<Shader::ShadowCSMStatic&>(*hShaderShadow.get());
	//auto &context = c_engine->GetRenderContext();

	//auto &renderPass = shadow->GetRenderPass(CLightBase::RenderPass::Dynamic);
	//auto &img = tex->GetImage();

	CLightBase::EntityInfo info(this,entWorld);
	info.meshes.reserve(model.GetSubMeshCount());
	//auto &materials = model.GetMaterials();
	for(auto &group : model.GetMeshGroups())
	{
		for(auto &mesh : group->GetMeshes())
		{
			for(auto &subMesh : mesh->GetSubMeshes())
			{
				if(CLightBase::ShouldPass(model,*static_cast<CModelSubMesh*>(subMesh.get())) == false)
					continue;
				info.meshes.push_back(static_cast<CModelSubMesh*>(subMesh.get()));
			}
		}
	}
	shaderShadow.Render(*this,m_cmdShadowBuffers,info);
#endif
}
void CLightDirectionalComponent::ReloadShadowCommandBuffers()
{
	m_bShadowBufferUpdateScheduled = true;
}
/*const std::vector<Vulkan::SwapCommandBuffer> &CLightDirectionalComponent::GetShadowCommandBuffers() const {return m_cmdShadowBuffers;}
const Vulkan::SwapCommandBuffer *CLightDirectionalComponent::GetShadowCommandBuffer(uint32_t layer) const
{
	if(layer >= m_cmdShadowBuffers.size())
		return nullptr;
	return &m_cmdShadowBuffers[layer];
}*/ // prosper TODO

bool CLightDirectionalComponent::ShouldPass(uint32_t layer,const Vector3 &min,const Vector3 &max)
{
	auto pLightComponent = GetEntity().GetComponent<CLightComponent>();
	auto *pShadowMap = pLightComponent.valid() ? pLightComponent->GetShadowMap() : nullptr;
	if(pShadowMap == nullptr || pShadowMap->GetType() != ShadowMap::Type::Cascaded)
		return false;
	auto &shadowMap = static_cast<ShadowMapCasc&>(*pShadowMap);
	auto numLayers = pShadowMap->GetLayerCount();
	auto &frustum = *shadowMap.GetFrustumSplit(layer);
	return (Intersection::AABBAABB(min,max,frustum.aabb.min +frustum.obbCenter,frustum.aabb.max +frustum.obbCenter) != INTERSECT_OUTSIDE) ? true : false;
}

void CLightDirectionalComponent::OnEntityComponentAdded(BaseEntityComponent &component)
{
	BaseEnvLightDirectionalComponent::OnEntityComponentAdded(component);
	if(typeid(component) == typeid(CTransformComponent))
	{
		FlagCallbackForRemoval(static_cast<CTransformComponent&>(component).GetOrientationProperty()->AddCallback([this,&component](std::reference_wrapper<const Quat> oldRot,std::reference_wrapper<const Quat> rot) {
			auto dir = uquat::forward(rot);
			auto &trComponent = static_cast<CTransformComponent&>(component);
			SetViewMatrix(glm::lookAtRH(trComponent.GetPosition(),trComponent.GetPosition() +dir,uvec::get_perpendicular(dir)));

			auto pLightComponent = GetEntity().GetComponent<CLightComponent>();
			if(pLightComponent.valid())
				pLightComponent->SetStaticResolved(false);
		}),CallbackType::Component,&component);
	}
	else if(typeid(component) == typeid(CLightComponent))
		static_cast<CLightComponent&>(component).SetLight(*this);
}

void CLightDirectionalComponent::UpdateFrustum(uint32_t frustumId)
{
	auto &ent = GetEntity();
	auto pLightComponent = ent.GetComponent<CLightComponent>();
	auto pTrComponent = ent.GetTransformComponent();
	auto pToggleComponent = ent.GetComponent<CToggleComponent>();
	if((pToggleComponent.valid() && pToggleComponent->IsTurnedOn() == false) || pLightComponent.expired() || pTrComponent.expired())
		return;
	auto *shadowMap = pLightComponent->GetShadowMap();
	if(shadowMap->GetType() != ShadowMap::Type::Cascaded)
		return;
	auto *csm = static_cast<ShadowMapCasc*>(shadowMap);
	if(csm->IsValid() == false)
		return;
	auto &scene = c_game->GetScene();
	auto &cam = scene->camera;
	csm->UpdateFrustum(frustumId,*cam.get(),GetViewMatrix(),pTrComponent->GetForward());
}

void CLightDirectionalComponent::UpdateFrustum()
{
	auto &ent = GetEntity();
	auto pLightComponent = ent.GetComponent<CLightComponent>();
	auto pTrComponent = ent.GetTransformComponent();
	auto pToggleComponent = ent.GetComponent<CToggleComponent>();
	if((pToggleComponent.valid() && pToggleComponent->IsTurnedOn() == false) || pLightComponent.expired() || pTrComponent.expired())
		return;
	auto *shadowMap = pLightComponent->GetShadowMap();
	if(shadowMap->GetType() != ShadowMap::Type::Cascaded)
		return;
	auto *csm = static_cast<ShadowMapCasc*>(shadowMap);
	if(csm->IsValid() == false)
		return;
	auto &scene = c_game->GetScene();
	auto &cam = scene->camera;
	csm->UpdateFrustum(*cam.get(),GetViewMatrix(),pTrComponent->GetForward());
}