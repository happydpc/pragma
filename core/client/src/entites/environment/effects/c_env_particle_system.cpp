/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "stdafx_client.h"
#include "pragma/entities/environment/effects/c_env_particle_system.h"
#include "pragma/entities/c_entityfactories.h"
#include <pragma/networking/nwm_util.h>
#include "pragma/entities/baseentity_luaobject.h"
#include "pragma/lua/c_lentity_handles.hpp"
#include <pragma/entities/baseentity_events.hpp>
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(env_particle_system,CEnvParticleSystem);

#pragma optimize("",off)
void CParticleSystemComponent::Initialize()
{
	BaseEnvParticleSystemComponent::Initialize();

	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_ON,[this](std::reference_wrapper<ComponentEvent> evData) {
		Start();
	});
	BindEventUnhandled(BaseToggleComponent::EVENT_ON_TURN_OFF,[this](std::reference_wrapper<ComponentEvent> evData) {
		Stop();
	});
	BindEvent(CIOComponent::EVENT_HANDLE_INPUT,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &inputData = static_cast<pragma::CEInputData&>(evData.get());
		if(ustring::compare(inputData.input,"setcontinuous",false))
		{
			SetContinuous(util::to_boolean(inputData.data));
			return util::EventReply::Handled;
		}
		return util::EventReply::Unhandled;
	});
	BindEvent(BaseEntity::EVENT_HANDLE_KEY_VALUE,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		auto &kvData = static_cast<CEKeyValueData&>(evData.get());
		return HandleKeyValue(kvData.key,kvData.value);
	});
	BindEvent(CAnimatedComponent::EVENT_SHOULD_UPDATE_BONES,[this](std::reference_wrapper<ComponentEvent> evData) -> util::EventReply {
		static_cast<CEShouldUpdateBones&>(evData.get()).shouldUpdate = IsActive();
		return util::EventReply::Handled;
	});

	auto &ent = GetEntity();
	auto pTrComponent = ent.GetTransformComponent();
	if(pTrComponent.valid())
	{
		FlagCallbackForRemoval(pTrComponent->GetPosProperty()->AddCallback([this](std::reference_wrapper<const Vector3> oldPos,std::reference_wrapper<const Vector3> pos) {
			if(IsActive() == false)
				return;
			for(auto it=m_childSystems.begin();it!=m_childSystems.end();++it)
			{
				auto &hChild = *it;
				if(hChild.child.valid())
				{
					auto pTrComponent = hChild.child->GetEntity().GetTransformComponent();
					if(pTrComponent.valid())
						pTrComponent->SetPosition(pos);
				}
			}
		}),CallbackType::Entity);
	}
}
void CParticleSystemComponent::OnEntitySpawn()
{
	CreateParticle();
	BaseEnvParticleSystemComponent::OnEntitySpawn();
}
util::EventReply CParticleSystemComponent::HandleEvent(ComponentEventId eventId,ComponentEvent &evData)
{
	if(BaseEnvParticleSystemComponent::HandleEvent(eventId,evData) == util::EventReply::Handled)
		return util::EventReply::Handled;

	return util::EventReply::Unhandled;
}
void CParticleSystemComponent::ReceiveData(NetPacket &packet)
{
	SetParticleFile(packet->ReadString());
	m_particleName = packet->ReadString();
}
void CParticleSystemComponent::SetParticleFile(const std::string &fileName)
{
	BaseEnvParticleSystemComponent::SetParticleFile(fileName);
	CParticleSystemComponent::Precache(fileName);
}
void CParticleSystemComponent::SetRenderMode(RenderMode mode) {m_renderMode = mode;}
RenderMode CParticleSystemComponent::GetRenderMode() const {return m_renderMode;}

void CParticleSystemComponent::CreateParticle()
{
	if(SetupParticleSystem(m_particleName) == false)
		return;
	/*if(m_hCbRenderCallback.IsValid())
		m_hCbRenderCallback.Remove();
	m_hCbRenderCallback = particle->AddRenderCallback([this]() {
		auto &ent = static_cast<CBaseEntity&>(GetEntity());
		auto pAttComponent = ent.GetComponent<CAttachableComponent>();
		if(pAttComponent.valid())
			pAttComponent->UpdateAttachmentOffset();
	});*/
}
void CParticleSystemComponent::SetRemoveOnComplete(bool b) {BaseEnvParticleSystemComponent::SetRemoveOnComplete(b);}

#include "pragma/rendering/shaders/particles/c_shader_particle_2d_base.hpp"
#include <pragma/model/model.h>
#include <pragma/model/modelmesh.h>
std::shared_ptr<Model> CParticleSystemComponent::GenerateModel(CGame &game,const std::vector<const CParticleSystemComponent*> &particleSystems)
{
	auto *cam = game.GetRenderCamera();
	if(cam == nullptr)
		return nullptr;
	std::unordered_set<const CParticleSystemComponent*> particleSystemList {};
	for(auto *pts : particleSystems)
	{
		particleSystemList.insert(pts);
		for(auto &childData : pts->GetChildren())
		{
			if(childData.child.expired())
				continue;
			particleSystemList.insert(childData.child.get());
		}
	}
	Vector3 camUpWs;
	Vector3 camRightWs;
	float ptNearZ,ptFarZ;
	auto nearZ = cam->GetNearZ();
	auto farZ = cam->GetFarZ();
	auto &posCam = cam->GetEntity().GetPosition();
	auto mdl = game.CreateModel();
	constexpr uint32_t numVerts = pragma::ShaderParticle2DBase::VERTEX_COUNT;
	uint32_t numTris = pragma::ShaderParticle2DBase::TRIANGLE_COUNT *2;
	for(auto *pts : particleSystemList)
	{
		auto &renderers = pts->GetRenderers();
		if(renderers.empty())
			return nullptr;
		auto &renderer = *renderers.front();
		auto *pShader = dynamic_cast<pragma::ShaderParticle2DBase*>(renderer.GetShader());
		if(pShader == nullptr)
			return nullptr;
		auto *mat = pts->GetMaterial();
		if(mat == nullptr)
			continue;
		std::optional<uint32_t> skinTexIdx {};
		mdl->AddMaterial(0,mat,&skinTexIdx);
		if(skinTexIdx.has_value() == false)
			continue;
		auto orientationType = pts->GetOrientationType();
		pShader->GetParticleSystemOrientationInfo(
			cam->GetProjectionMatrix() *cam->GetViewMatrix(),*pts,orientationType,camUpWs,camRightWs,
			ptNearZ,ptFarZ,mat,nearZ,farZ
		);

		auto *spriteSheetAnim = pts->GetSpriteSheetAnimation();
		auto &particles = pts->GetRenderParticleData();
		auto &animData = pts->GetParticleAnimationData();
		auto numParticles = pts->GetRenderParticleCount();

		auto subMesh = game.CreateModelSubMesh();
		subMesh->SetSkinTextureIndex(*skinTexIdx);
		auto &verts = subMesh->GetVertices();
		auto &tris = subMesh->GetTriangles();
		auto numTrisSys = numParticles *numTris;
		auto numVertsSys = numParticles *numVerts;
		tris.reserve(numTrisSys *3);
		verts.resize(numVertsSys);
		uint32_t vertOffset = 0;
		for(auto i=decltype(numParticles){0u};i<numParticles;++i)
		{
			auto &pt = particles.at(i);
			auto ptIdx = pts->TranslateBufferIndex(i);
			auto pos = pts->GetParticlePosition(ptIdx);
			Vector2 uvStart {0.f,0.f};
			Vector2 uvEnd {1.f,1.f};
			if(pts->IsAnimated() && spriteSheetAnim && spriteSheetAnim->sequences.empty() == false)
			{
				auto &animData = pts->GetParticleAnimationData().at(i);
				auto &ptData = *const_cast<CParticleSystemComponent*>(pts)->GetParticle(ptIdx);
				auto seqIdx = ptData.GetSequence();
				assert(seqIdx < spriteSheetAnim->sequences.size());
				auto &seq = (seqIdx < spriteSheetAnim->sequences.size()) ? spriteSheetAnim->sequences.at(seqIdx) : spriteSheetAnim->sequences.back();
				auto frameIndex = (seqIdx < spriteSheetAnim->sequences.size()) ? seq.GetLocalFrameIndex(animData.frameIndex0) : 0;
				auto &frame = seq.frames.at(frameIndex);
				uvStart = frame.uvStart;
				uvEnd = frame.uvEnd;
			}
			for(auto vertIdx=decltype(numVerts){0u};vertIdx<numVerts;++vertIdx)
			{
				auto vertPos = pShader->CalcVertexPosition(*pts,pts->TranslateBufferIndex(i),vertIdx,posCam,camUpWs,camRightWs,nearZ,farZ);
				auto uv = pragma::ShaderParticle2DBase::GetVertexUV(vertIdx);
				auto &v = verts.at(vertOffset +vertIdx);
				v.position = vertPos;
				v.normal = -uvec::RIGHT;
				v.tangent = uvec::FORWARD;
				v.uv = uvStart +uv *(uvEnd -uvStart);
			}
			static_assert(pragma::ShaderParticle2DBase::TRIANGLE_COUNT == 2 && pragma::ShaderParticle2DBase::VERTEX_COUNT == 6);
			std::array<uint32_t,12> indices = {
				0,1,2,
				3,4,5,

				// Back facing
				0,2,1,
				3,5,4
			};
			for(auto idx : indices)
				tris.push_back(vertOffset +idx);

			vertOffset += numVerts;
		}

		auto mesh = game.CreateModelMesh();
		mesh->AddSubMesh(subMesh);
		mdl->GetMeshGroup(0)->AddMesh(mesh);
	}
	return mdl;
}
std::shared_ptr<Model> CParticleSystemComponent::GenerateModel() const
{
	auto &game = static_cast<CGame&>(*GetEntity().GetNetworkState()->GetGameState());
	std::vector<const CParticleSystemComponent*> particleSystems {};
	particleSystems.push_back(this);
	return GenerateModel(game,particleSystems);
}

luabind::object CParticleSystemComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<CParticleSystemComponentHandleWrapper>(l);}

///////////////

void CEnvParticleSystem::Initialize()
{
	CBaseEntity::Initialize();
	AddComponent<CParticleSystemComponent>();
}
#pragma optimize("",on)
