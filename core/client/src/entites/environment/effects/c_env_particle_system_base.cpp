#include "stdafx_client.h"
#include <algorithm>
#include "pragma/clientstate/clientstate.h"
#include "pragma/game/c_game.h"
#include "pragma/rendering/shaders/particles/c_shader_particle.hpp"
#include "pragma/entities/environment/effects/c_env_particle_system.h"
#include "pragma/particlesystem/c_particlesystemdata.h"
#include <buffers/prosper_dynamic_resizable_buffer.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <sharedutils/util_file.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

using namespace pragma;

// 10 MiB
#define PARTICLE_BUFFER_SIZE 10'485'760

decltype(CParticleSystemComponent::s_particleData) CParticleSystemComponent::s_particleData;
decltype(CParticleSystemComponent::s_precached) CParticleSystemComponent::s_precached;

extern DLLCLIENT CGame *c_game;
extern DLLCLIENT ClientState *client;
extern DLLCENGINE CEngine *c_engine;

CParticleSystemComponent::Node::Node(CBaseEntity *ent)
	: hEntity((ent != nullptr) ? ent->GetHandle() : EntityHandle{}),bEntity(true)
{}
CParticleSystemComponent::Node::Node(const Vector3 &pos)
	: position(pos),bEntity(false)
{}

CParticleSystemComponent *CParticleSystemComponent::Create(const std::string &fname,CParticleSystemComponent *parent,bool bRecordKeyValues,bool bAutoSpawn)
{
	auto *entChild = c_game->CreateEntity<CEnvParticleSystem>();
	if(entChild == nullptr)
		return nullptr;
	auto pParticleSysComponent = entChild->GetComponent<CParticleSystemComponent>();
	if(pParticleSysComponent.expired() || pParticleSysComponent->SetupParticleSystem(fname,parent,bRecordKeyValues) == false)
	{
		entChild->Remove();
		return nullptr;
	}
	if(bAutoSpawn)
		entChild->Spawn();
	return pParticleSysComponent.get();
}
CParticleSystemComponent *CParticleSystemComponent::Create(const std::unordered_map<std::string,std::string> &values,CParticleSystemComponent *parent,bool bRecordKeyValues,bool bAutoSpawn)
{
	auto *entChild = c_game->CreateEntity<CEnvParticleSystem>();
	if(entChild == nullptr)
		return nullptr;
	auto pParticleSysComponent = entChild->GetComponent<CParticleSystemComponent>();
	if(pParticleSysComponent.expired() || pParticleSysComponent->SetupParticleSystem(values,parent,bRecordKeyValues) == false)
	{
		entChild->Remove();
		return nullptr;
	}
	if(bAutoSpawn)
		entChild->Spawn();
	return pParticleSysComponent.get();
}
CParticleSystemComponent *CParticleSystemComponent::Create(CParticleSystemComponent *parent,bool bAutoSpawn)
{
	auto *entChild = c_game->CreateEntity<CEnvParticleSystem>();
	if(entChild == nullptr)
		return nullptr;
	auto pParticleSysComponent = entChild->GetComponent<CParticleSystemComponent>();
	if(pParticleSysComponent.expired() || pParticleSysComponent->SetupParticleSystem(parent) == false)
	{
		entChild->Remove();
		return nullptr;
	}
	if(bAutoSpawn)
		entChild->Spawn();
	return pParticleSysComponent.get();
}
bool CParticleSystemComponent::IsStatic() const {return (m_operators.empty() && umath::is_flag_set(m_flags,Flags::HasMovingParticles) == false) ? true : false;}
bool CParticleSystemComponent::IsRendererBufferUpdateRequired() const {return umath::is_flag_set(m_flags,Flags::RendererBufferUpdateRequired);}

bool CParticleSystemComponent::IsParticleFilePrecached(const std::string &fname)
{
	auto fid = FileManager::GetCanonicalizedPath(fname);
	ustring::to_lower(fid);
	ufile::remove_extension_from_filename(fid);
	auto it = std::find(s_precached.begin(),s_precached.end(),fid);
	return (it != s_precached.end()) ? true : false;
}

bool CParticleSystemComponent::Precache(std::string fname,bool bReload)
{
	fname = FileManager::GetCanonicalizedPath(fname);
	ustring::to_lower(fname);
	if(fname.length() >= 4 && fname.substr(fname.length() -4) == ".wpt")
		fname = fname.substr(0,fname.length() -4);
	auto it = std::find(s_precached.begin(),s_precached.end(),fname);
	if(it != s_precached.end())
	{
		if(bReload == false)
			return true;
	}
	else
		s_precached.push_back(fname);
	auto path = "particles\\" +fname +".wpt";
	auto f = FileManager::OpenFile(path.c_str(),"rb");
	if(f == nullptr)
		return false;
	std::array<int8_t,3> header = {
		static_cast<int8_t>(f->ReadChar()),
		static_cast<int8_t>(f->ReadChar()),
		static_cast<int8_t>(f->ReadChar())
	};
	if(header[0] != 'W' || header[1] != 'P' || header[2] != 'T')
		return false; // Incorrect format
	auto version = f->Read<uint32_t>();
	UNUSED(version);
	auto numParticles = f->Read<uint32_t>();
	std::vector<std::string> names(numParticles);
	std::vector<uint64_t> offsets(numParticles);
	for(auto i=decltype(numParticles){0};i<numParticles;++i)
	{
		auto name = f->ReadString();
		auto offset = f->Read<uint64_t>();
		names[i] = name;
		offsets[i] = offset;
	}
	for(auto i=decltype(numParticles){0};i<numParticles;++i)
	{
		auto &name = names[i];
		auto offset = offsets[i];
		if(bReload == true || s_particleData.find(name) == s_particleData.end())
		{
			f->Seek(offset);

			auto data = std::make_unique<CParticleSystemData>();
			auto numSettings = f->Read<uint32_t>();
			auto &settings = data->settings;
			for(auto i=decltype(numSettings){0};i<numSettings;++i)
			{
				auto key = f->ReadString();
				auto val = f->ReadString();
				settings.insert(std::remove_reference<decltype(settings)>::type::value_type(key,val));
				if(key == "material")
					client->LoadMaterial(val.c_str());
			}
			std::array<std::vector<std::unique_ptr<CParticleModifierData>>*,3> params = {
				&data->initializers,
				&data->operators,
				&data->renderers
			};
			for(int32_t i=0;i<3;++i)
			{
				auto num = f->Read<uint32_t>();
				for(auto j=decltype(num){0};j<num;++j)
				{
					auto identifier = f->ReadString();
					auto modData = std::make_unique<CParticleModifierData>(identifier);
					auto numKeyValues = f->Read<uint32_t>();
					for(auto k=decltype(numKeyValues){0};k<numKeyValues;++k)
					{
						auto key = f->ReadString();
						auto val = f->ReadString();
						modData->settings.insert(decltype(modData->settings)::value_type(key,val));
					}
					params[i]->push_back(std::move(modData));
				}
			}
			auto &children = data->children;
			auto numChildren = f->Read<uint8_t>();
			for(auto i=decltype(numChildren){0};i<numChildren;++i)
				children.push_back(f->ReadString());
			s_particleData[name] = std::move(data);
		}
	}
	return true;
}

const std::vector<CParticle> &CParticleSystemComponent::GetParticles() const {return m_particles;}
CParticle *CParticleSystemComponent::GetParticle(size_t idx)
{
	if(idx >= GetMaxParticleCount())
		return nullptr;
	return &m_particles[idx];
}

void CParticleSystemComponent::ClearCache()
{
	for(auto it=s_particleData.begin();it!=s_particleData.end();++it)
		it->second = nullptr;
}

bool CParticleSystemComponent::SetupParticleSystem(std::string fname,CParticleSystemComponent *parent,bool bRecordKeyValues)
{
	if(umath::is_flag_set(m_flags,Flags::Setup))
		return true;
	auto it = s_particleData.find(fname);
	if(it == s_particleData.end())
	{
		Con::cwar<<"WARNING: Attempted to create unknown particle system '"<<fname<<"'!"<<Con::endl;
		return nullptr;
	}
	auto &data = it->second;
	auto r = SetupParticleSystem(data->settings,parent,bRecordKeyValues);
	if(r == false)
		return false;

	SetParticleSystemName(fname);

	// Children have to be initialized before operators (in case some operators need to access the children)
	for(auto &child : data->children)
	{
		auto *pt = Create(child,this,bRecordKeyValues);
		if(pt != nullptr)
			pt->GetEntity().Spawn();
	}

	for(auto &modData : data->initializers)
		AddInitializer(modData->name,modData->settings);
	for(auto &modData : data->operators)
		AddOperator(modData->name,modData->settings);
	for(auto &modData : data->renderers)
		AddRenderer(modData->name,modData->settings);
	if(data->renderers.empty())
	{
		std::unordered_map<std::string,std::string> values {};
		AddRenderer("sprite",values); // Default Renderer
	}
	return r;
}

void CParticleSystemComponent::SetParticleSystemName(const std::string &name) {m_particleSystemName = name;}
const std::string &CParticleSystemComponent::GetParticleSystemName() const {return m_particleSystemName;}

bool CParticleSystemComponent::SetupParticleSystem(const std::unordered_map<std::string,std::string> &values,CParticleSystemComponent *parent,bool bRecordKeyValues)
{
	if(umath::is_flag_set(m_flags,Flags::Setup))
		return true;
	umath::set_flag(m_flags,Flags::Setup);

	for(auto &kv : values)
		GetEntity().SetKeyValue(kv.first,kv.second);
	if(bRecordKeyValues)
		RecordKeyValues(values);
	m_nodes.resize(m_maxNodes,{nullptr});
	m_tNextEmission = 0.f;
	if(parent != nullptr)
		SetParent(parent);
	// TODO: 'orientation_axis' / 'orientation'?
	return true;
}

bool CParticleSystemComponent::SetupParticleSystem(CParticleSystemComponent *parent)
{
	std::unordered_map<std::string,std::string> values;
	return SetupParticleSystem(values,parent);
}

///////////////////////////////////////////

decltype(CParticleSystemComponent::PARTICLE_DATA_SIZE) CParticleSystemComponent::PARTICLE_DATA_SIZE = sizeof(CParticleSystemComponent::ParticleData);
decltype(CParticleSystemComponent::VERTEX_COUNT) CParticleSystemComponent::VERTEX_COUNT = 6;
static std::shared_ptr<prosper::DynamicResizableBuffer> s_particleBuffer = nullptr;
static std::shared_ptr<prosper::DynamicResizableBuffer> s_animStartBuffer = nullptr;
static std::shared_ptr<prosper::UniformResizableBuffer> s_animBuffer = nullptr;
static std::shared_ptr<prosper::Buffer> s_vertexBuffer = nullptr;
const auto PARTICLE_BUFFER_INSTANCE_SIZE = sizeof(CParticleSystemComponent::ParticleData);
const auto ANIM_START_BUFFER_INSTANCE_SIZE = sizeof(float);
util::EventReply CParticleSystemComponent::HandleKeyValue(const std::string &key,const std::string &value)
{
#pragma message ("TODO: Calculate max particles automatically!")
	if(ustring::compare(key,"maxparticles",false))
	{
		if(m_state != State::Initial)
			Con::cwar<<"WARNING: Attempted to change max particle count for particle system which has already been started! Ignoring..."<<Con::endl;
		else
			m_maxParticles = util::to_int(value);
	}
	else if(ustring::compare(key,"limit_particle_count"))
		m_particleLimit = util::to_int(value);
	else if(ustring::compare(key,"emission_rate"))
		m_emissionRate = util::to_int(value);
	else if(ustring::compare(key,"cast_shadows"))
		SetCastShadows(util::to_boolean(value));
	else if(ustring::compare(key,"static_scale"))
		m_worldScale = util::to_float(value);
	else if(ustring::compare(key,"random_start_frame"))
		umath::set_flag(m_flags,Flags::RandomStartFrame,util::to_boolean(value));
	else if(ustring::compare(key,"material"))
		SetMaterial(client->LoadMaterial(value));
	else if(ustring::compare(key,"radius"))
		SetRadius(util::to_float(value));
	else if(ustring::compare(key,"extent"))
		SetExtent(util::to_float(value));
	else if(ustring::compare(key,"sort_particles"))
		umath::set_flag(m_flags,Flags::SortParticles,util::to_boolean(value));
	else if(ustring::compare(key,"orientation_type"))
		m_orientationType = static_cast<OrientationType>(util::to_int(value));
	else if(ustring::compare(key,"color"))
		m_initialColor = Color(value);
	else if(ustring::compare(key,"loop"))
		SetContinuous(util::to_boolean(value));
	else if(ustring::compare(key,"origin"))
		m_origin = uvec::create(value);
	else if(ustring::compare(key,"bloom_scale"))
		m_bloomScale = util::to_float(value);
	else if(ustring::compare(key,"intensity"))
		m_intensity = util::to_float(value);
	else if(ustring::compare(key,"max_node_count"))
		m_maxNodes = util::to_int(value);
	else if(ustring::compare(key,"lifetime"))
		m_lifeTime = util::to_float(value);
	else if(ustring::compare(key,"soft_particles"))
		SetSoftParticles(util::to_boolean(value));
	else if(ustring::compare(key,"texture_scrolling_enabled"))
		SetTextureScrollingEnabled(util::to_boolean(value));
	else if(ustring::compare(key,"world_rotation"))
	{
		std::array<float,4> values;
		ustring::string_to_array(value,values.data(),atof,values.size());
		m_particleRot.w = values.at(0);
		m_particleRot.x = values.at(1);
		m_particleRot.y = values.at(2);
		m_particleRot.z = values.at(3);
	}
	else if(ustring::compare(key,"alpha_mode"))
	{
		auto alphaMode = value;
		ustring::to_lower(alphaMode);
		if(alphaMode == "additive_full")
			m_alphaMode = pragma::AlphaMode::AdditiveFull;
		else if(alphaMode == "opaque")
			m_alphaMode = pragma::AlphaMode::Opaque;
		else if(alphaMode == "masked")
			m_alphaMode = pragma::AlphaMode::Masked;
		else if(alphaMode == "translucent")
			m_alphaMode = pragma::AlphaMode::Translucent;
		else if(alphaMode == "additive")
			m_alphaMode = pragma::AlphaMode::Additive;
	}
	else if(ustring::compare(key,"premultiply_alpha"))
		SetAlphaPremultiplied(util::to_boolean(value));
	else if(ustring::compare(key,"angles"))
	{
		auto ang = EulerAngles(value);
		m_particleRot = uquat::create(ang);
	}
	else if(ustring::compare(key,"black_to_alpha"))
		umath::set_flag(m_flags,Flags::BlackToAlpha,util::to_boolean(value));
	else if(ustring::compare(key,"move_with_emitter"))
		umath::set_flag(m_flags,Flags::MoveWithEmitter,util::to_boolean(value));
	else if(ustring::compare(key,"rotate_with_emitter"))
		umath::set_flag(m_flags,Flags::RotateWithEmitter,util::to_boolean(value));
	else if(ustring::compare(key,"transform_with_emitter"))
		umath::set_flag(m_flags,Flags::MoveWithEmitter | Flags::RotateWithEmitter,util::to_boolean(value));
	else
		return util::EventReply::Unhandled;
	return util::EventReply::Handled;
}

CParticleSystemComponent::~CParticleSystemComponent()
{
	Stop();
	for(auto &hChild : m_childSystems)
	{
		if(hChild.valid())
			hChild->GetEntity().RemoveSafely();
	}
}


void CParticleSystemComponent::SetContinuous(bool b)
{
	BaseEnvParticleSystemComponent::SetContinuous(b);
	if(b == false)
	{
		if(GetRemoveOnComplete() && m_state == State::Complete)
			GetEntity().RemoveSafely();
	}
}

bool CParticleSystemComponent::ShouldParticlesRotateWithEmitter() const {return umath::is_flag_set(m_flags,Flags::RotateWithEmitter);}
bool CParticleSystemComponent::ShouldParticlesMoveWithEmitter() const {return umath::is_flag_set(m_flags,Flags::MoveWithEmitter);}

Vector3 CParticleSystemComponent::PointToParticleSpace(const Vector3 &p,bool bRotateWithEmitter) const
{
	auto r = p;
	if(bRotateWithEmitter == true)
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		if(pTrComponent.valid())
			uvec::rotate(&r,pTrComponent->GetOrientation());
	}
	if(ShouldParticlesMoveWithEmitter())
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		if(pTrComponent.valid())
			r += pTrComponent->GetPosition();
	}
	return r;
}
Vector3 CParticleSystemComponent::PointToParticleSpace(const Vector3 &p) const {return PointToParticleSpace(p,ShouldParticlesRotateWithEmitter());}
Vector3 CParticleSystemComponent::DirectionToParticleSpace(const Vector3 &p,bool bRotateWithEmitter) const
{
	auto r = p;
	if(bRotateWithEmitter == true)
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		if(pTrComponent.valid())
			uvec::rotate(&r,pTrComponent->GetOrientation());
	}
	return r;
}
Vector3 CParticleSystemComponent::DirectionToParticleSpace(const Vector3 &p) const {return DirectionToParticleSpace(p,ShouldParticlesRotateWithEmitter());}

void CParticleSystemComponent::InitializeBuffers()
{
	if(s_vertexBuffer == nullptr)
	{
		const std::array<Vector2,VERTEX_COUNT> vertices = {
			Vector2(0.5f,-0.5f),
			Vector2(-0.5f,-0.5f),
			Vector2(-0.5f,0.5f),
			Vector2(0.5f,0.5f),
			Vector2(0.5f,-0.5f),
			Vector2(-0.5f,0.5f)
		};
		auto &dev = c_engine->GetDevice();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		createInfo.size = vertices.size() *sizeof(vertices.front());
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
		s_vertexBuffer = prosper::util::create_buffer(dev,createInfo,vertices.data());
		s_vertexBuffer->SetDebugName("particle_vertex_buf");
	}
	if(s_particleBuffer == nullptr)
	{
		auto instanceCount = 32'768ull;
		auto maxInstanceCount = instanceCount *40u;
		auto instanceSize = PARTICLE_BUFFER_INSTANCE_SIZE;
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		createInfo.size = instanceSize *maxInstanceCount;
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
		s_particleBuffer = prosper::util::create_dynamic_resizable_buffer(*c_engine,createInfo,instanceSize *maxInstanceCount,0.05f);
		s_particleBuffer->SetDebugName("particle_instance_buf");
	}
	if(s_animStartBuffer == nullptr)
	{
		auto instanceCount = 524'288ull;
		auto maxInstanceCount = instanceCount *5u;
		auto instanceSize = ANIM_START_BUFFER_INSTANCE_SIZE;
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		createInfo.size = instanceSize *maxInstanceCount;
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
		s_animStartBuffer = prosper::util::create_dynamic_resizable_buffer(*c_engine,createInfo,instanceSize *maxInstanceCount,0.01f);
		s_animStartBuffer->SetDebugName("particle_anim_start_buf");
	}
	if(s_animBuffer == nullptr)
	{
		auto instanceCount = 64;
		auto maxInstanceCount = instanceCount *5u;
		auto instanceSize = sizeof(AnimationData);
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
		createInfo.size = instanceSize *maxInstanceCount;
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
		s_animBuffer = prosper::util::create_uniform_resizable_buffer(*c_engine,createInfo,instanceSize,instanceSize *maxInstanceCount,0.01f);
		s_animBuffer->SetDebugName("particle_anim_data_buf");
	}
}
void CParticleSystemComponent::ClearBuffers()
{
	s_vertexBuffer = nullptr;
	s_particleBuffer = nullptr;
	s_animStartBuffer = nullptr;
	s_animBuffer = nullptr;
}
float CParticleSystemComponent::GetStaticWorldScale() const {return m_worldScale;}
void CParticleSystemComponent::SetStaticWorldScale(float scale) {m_worldScale = scale;}
void CParticleSystemComponent::SetSoftParticles(bool bSmooth) {umath::set_flag(m_flags,Flags::SoftParticles,bSmooth);}
bool CParticleSystemComponent::GetSoftParticles() const {return umath::is_flag_set(m_flags,Flags::SoftParticles);}

void CParticleSystemComponent::SetCastShadows(bool b) {umath::set_flag(m_flags,Flags::CastShadows,b);}
bool CParticleSystemComponent::GetCastShadows() const {return umath::is_flag_set(m_flags,Flags::CastShadows);}
float CParticleSystemComponent::GetBloomScale() const {return m_bloomScale;}
void CParticleSystemComponent::SetBloomScale(float scale) {m_bloomScale = scale;}
float CParticleSystemComponent::GetIntensity() const {return m_intensity;}
void CParticleSystemComponent::SetIntensity(float intensity) {m_intensity = intensity;}
const std::pair<Vector3,Vector3> &CParticleSystemComponent::GetRenderBounds() const {return m_renderBounds;}

CParticleSystemComponent::OrientationType CParticleSystemComponent::GetOrientationType() const {return m_orientationType;}
void CParticleSystemComponent::SetOrientationType(OrientationType type)
{
	m_orientationType = type;
}

void CParticleSystemComponent::SetRadius(float r)
{
	m_radius = r;
	m_extent = umath::sqrt(umath::pow2(r) *2.f);
}
void CParticleSystemComponent::SetExtent(float ext)
{
	m_extent = ext;
	m_radius = umath::sqrt(umath::pow2(ext) /2.f);
}
float CParticleSystemComponent::GetRadius() const {return m_radius;}
float CParticleSystemComponent::GetExtent() const {return m_extent;}

void CParticleSystemComponent::SetMaterial(Material *mat)
{
	m_material = mat;
	if(mat == nullptr)
		return;
	auto &root = mat->GetDataBlock();
	if(root == nullptr)
		return;
	auto blackToAlpha = false;
	if(root->GetBool("black_to_alpha",&blackToAlpha) == true)
		umath::set_flag(m_flags,Flags::BlackToAlpha,true);
}
void CParticleSystemComponent::SetMaterial(const char *mat) {SetMaterial(client->LoadMaterial(mat));}
Material *CParticleSystemComponent::GetMaterial() const {return m_material;}

void CParticleSystemComponent::AddInitializer(std::string identifier,const std::unordered_map<std::string,std::string> &values)
{
	ustring::to_lower(identifier);
	auto *map = GetParticleModifierMap();
	auto *factory = map->FindInitializer(identifier);
	if(factory == nullptr)
	{
		Con::cwar<<"WARNING: Attempted to create unknown particle initializer '"<<identifier<<"'! Ignoring..."<<Con::endl;
		return;
	}
	auto initializer = factory(*this,values);
	if(IsRecordingKeyValues())
		initializer->RecordKeyValues(values);
	m_initializers.push_back(std::move(initializer));
}
void CParticleSystemComponent::AddOperator(std::string identifier,const std::unordered_map<std::string,std::string> &values)
{
	ustring::to_lower(identifier);
	auto *map = GetParticleModifierMap();
	auto *factory = map->FindOperator(identifier);
	if(factory == nullptr)
	{
		Con::cwar<<"WARNING: Attempted to create unknown particle operator '"<<identifier<<"'! Ignoring..."<<Con::endl;
		return;
	}
	auto op = factory(*this,values);
	if(IsRecordingKeyValues())
		op->RecordKeyValues(values);
	m_operators.push_back(std::move(op));
}
void CParticleSystemComponent::AddRenderer(std::string identifier,const std::unordered_map<std::string,std::string> &values)
{
	ustring::to_lower(identifier);
	auto *map = GetParticleModifierMap();
	auto *factory = map->FindRenderer(identifier);
	if(factory == nullptr)
	{
		Con::cwar<<"WARNING: Attempted to create unknown particle renderer '"<<identifier<<"'! Ignoring..."<<Con::endl;
		return;
	}
	auto op = factory(*this,values);
	if(IsRecordingKeyValues())
		op->RecordKeyValues(values);
	m_renderers.push_back(std::move(op));
}

const CParticleSystemComponent *CParticleSystemComponent::GetParent() const {return const_cast<CParticleSystemComponent*>(this)->GetParent();}
CParticleSystemComponent *CParticleSystemComponent::GetParent()
{
	if(m_hParent.expired())
		return nullptr;
	return m_hParent.get();
}

void CParticleSystemComponent::SetParent(CParticleSystemComponent *particle)
{
	if(m_hParent.valid())
	{
		auto *parent = m_hParent.get();
		if(parent == particle)
			return;
		m_hParent = util::WeakHandle<CParticleSystemComponent>{};
		if(parent != nullptr)
			parent->RemoveChild(this);
	}
	if(particle == nullptr)
	{
		m_hParent = util::WeakHandle<CParticleSystemComponent>{};
		return;
	}
	m_hParent = util::WeakHandle<CParticleSystemComponent>{std::static_pointer_cast<CParticleSystemComponent>(particle->shared_from_this())};
	particle->AddChild(*this);
	auto pTrComponent = GetEntity().GetTransformComponent();
	auto pTrComponentPt = particle->GetEntity().GetTransformComponent();
	if(pTrComponent.valid() && pTrComponentPt.valid())
	{
		pTrComponent->SetPosition(pTrComponentPt->GetPosition());
		pTrComponent->SetOrientation(pTrComponentPt->GetOrientation());
	}
}

CParticleSystemComponent *CParticleSystemComponent::AddChild(const std::string &name)
{
	auto *pt = Create(name,this,IsRecordingKeyValues());
	if(pt == nullptr)
		return nullptr;
	pt->GetEntity().Spawn();
	AddChild(*pt);
	return pt;
}

void CParticleSystemComponent::AddChild(CParticleSystemComponent &particle)
{
	if(HasChild(particle))
		return;
	m_childSystems.push_back(util::WeakHandle<CParticleSystemComponent>{std::static_pointer_cast<CParticleSystemComponent>(particle.shared_from_this())});
	particle.SetParent(this);
}

void CParticleSystemComponent::RemoveChild(CParticleSystemComponent *particle)
{
	for(auto it=m_childSystems.begin();it!=m_childSystems.end();++it)
	{
		auto &hChild = *it;
		if(hChild.get() == particle)
		{
			auto *child = hChild.get();
			m_childSystems.erase(it);
			if(child != nullptr)
				child->SetParent(nullptr);
			break;
		}
	}
}

bool CParticleSystemComponent::HasChild(CParticleSystemComponent &particle)
{
	auto it = std::find_if(m_childSystems.begin(),m_childSystems.end(),[&particle](const util::WeakHandle<CParticleSystemComponent> &hchild) {
		return (hchild.get() == &particle) ? true : false;
	});
	return (it != m_childSystems.end()) ? true : false;
}

const std::shared_ptr<prosper::Buffer> &CParticleSystemComponent::GetVertexBuffer() const {return s_vertexBuffer;}
const std::shared_ptr<prosper::Buffer> &CParticleSystemComponent::GetParticleBuffer() const {return m_bufParticles;}
const std::shared_ptr<prosper::Buffer> &CParticleSystemComponent::GetAnimationStartBuffer() const {return m_bufAnimStart;}
const std::shared_ptr<prosper::Buffer> &CParticleSystemComponent::GetAnimationBuffer() const {return m_bufAnim;}
Anvil::DescriptorSet *CParticleSystemComponent::GetAnimationDescriptorSet() {return (m_descSetGroupAnimation != nullptr) ? (*m_descSetGroupAnimation)->get_descriptor_set(0u) : nullptr;}
const std::shared_ptr<prosper::DescriptorSetGroup> &CParticleSystemComponent::GetAnimationDescriptorSetGroup() const {return m_descSetGroupAnimation;}

bool CParticleSystemComponent::IsAnimated() const {return m_animData != nullptr;}
const CParticleSystemComponent::AnimationData *CParticleSystemComponent::GetAnimationData() const {return m_animData.get();}

void CParticleSystemComponent::Start()
{
	CreateParticle();
	if(IsActiveOrPaused())
		Stop();
	umath::remove_flag(m_flags,Flags::Dying);
	for(auto &pt : m_particles)
		pt.Resurrect();
	m_state = State::Active;
	m_tLastEmission = 0.0;
	m_tLifeTime = 0.0;
	m_currentParticleLimit = m_particleLimit;

	// Children have to be started before operators are initialized,
	// in case one of the operators needs to access a child
	for(auto &hChild : m_childSystems)
	{
		if(hChild.valid())
			hChild->Start();
	}
	//

	//
	for(auto &init : m_initializers)
		init->Initialize();
	for(auto &op : m_operators)
		op->Initialize(); // Operators have to be initialized before buffers are initialized
	for(auto &r : m_renderers)
		r->Initialize();
	//

	if(m_maxParticles > 0)
	{
		m_animData = nullptr;
		m_bufAnim = nullptr;
		m_descSetGroupAnimation = nullptr;
		if(m_material != nullptr)
		{
			if(IsTextureScrollingEnabled())
			{
				// Animations and texture scrolling cannot be
				// used at the same time
				m_animData = std::make_unique<AnimationData>();
				m_animData->frames = 1;
				m_animData->rows = 1;
				m_animData->columns = 1;
			}
			else if(pragma::ShaderParticle2DBase::DESCRIPTOR_SET_ANIMATION.IsValid())
			{
				auto &data = m_material->GetDataBlock();
				if(data != nullptr)
				{
					auto &anim = data->GetValue("animation");
					if(anim != nullptr && anim->IsBlock())
					{
						auto &animBlock = *std::static_pointer_cast<ds::Block>(anim);
						m_animData = std::make_unique<AnimationData>();

						animBlock.GetInt("offset",&m_animData->offset);
						animBlock.GetInt("frames",&m_animData->frames);
						animBlock.GetInt("fps",&m_animData->fps);
						animBlock.GetInt("rows",&m_animData->rows);
						animBlock.GetInt("columns",&m_animData->columns);
						m_bufAnim = s_animBuffer->AllocateBuffer(m_animData.get());

						auto &dev = c_engine->GetDevice();
						m_descSetGroupAnimation = prosper::util::create_descriptor_set_group(dev,pragma::ShaderParticle2DBase::DESCRIPTOR_SET_ANIMATION);
						prosper::util::set_descriptor_set_binding_uniform_buffer(
							*(*m_descSetGroupAnimation)->get_descriptor_set(0u),*m_bufAnim,0u
						);
					}
				}
			}
		}
		
		m_bufParticles = s_particleBuffer->AllocateBuffer(m_maxParticles *PARTICLE_BUFFER_INSTANCE_SIZE);
		auto bAnimated = IsAnimated();
		if(bAnimated)
			m_bufAnimStart = s_animStartBuffer->AllocateBuffer(m_maxParticles *ANIM_START_BUFFER_INSTANCE_SIZE);
		else
			m_bufAnimStart = nullptr;

		//if(m_maxParticles != m_maxParticlesCur)
		//{
			m_particles.resize(m_maxParticles);
			m_sortedParticleIndices.resize(m_particles.size());

			m_particleIndicesToBufferIndices.resize(m_particles.size());
			std::fill(m_particleIndicesToBufferIndices.begin(),m_particleIndicesToBufferIndices.end(),0);
			m_bufferIndicesToParticleIndices.resize(m_particles.size());
			std::fill(m_bufferIndicesToParticleIndices.begin(),m_bufferIndicesToParticleIndices.end(),0);

			for(auto i=decltype(m_maxParticles){0};i<m_maxParticles;++i)
			{
				m_particles[i].SetIndex(i);
				m_sortedParticleIndices[i] = i;
			}
			m_instanceData.resize(m_maxParticles);
			if(bAnimated)
				m_dataAnimStart.resize(m_maxParticles);
		//}
		m_maxParticlesCur = m_maxParticles;
	}
}

void CParticleSystemComponent::Stop()
{
	if(!IsActiveOrPaused())
		return;
	m_state = State::Complete;
	for(auto &init : m_initializers)
		init->Destroy();
	for(auto &op : m_operators)
		op->Destroy();
	for(auto &r : m_renderers)
		r->Destroy();
	m_particles.clear();
	m_sortedParticleIndices.clear();
	m_instanceData.clear();
	m_particleIndicesToBufferIndices.clear();
	m_bufferIndicesToParticleIndices.clear();
	m_dataAnimStart.clear();
	m_bufParticles = nullptr;
	m_bufAnimStart = nullptr;
	for(auto &hChild : m_childSystems)
	{
		if(hChild.valid())
			hChild->Stop();
	}
	m_tLifeTime = 0.0;
	OnComplete();
}

double CParticleSystemComponent::GetLifeTime() const {return m_tLifeTime;}

const std::vector<CParticleSystemComponent::ParticleData> &CParticleSystemComponent::GetRenderParticleData() const {return m_instanceData;}

bool CParticleSystemComponent::IsActive() const {return m_state == State::Active;}
bool CParticleSystemComponent::IsEmissionPaused() const {return m_state == State::Paused;}
bool CParticleSystemComponent::IsActiveOrPaused() const {return IsActive() || IsEmissionPaused();}

uint32_t CParticleSystemComponent::GetParticleCount() const {return m_numParticles;}
uint32_t CParticleSystemComponent::GetRenderParticleCount() const {return m_numRenderParticles;}
uint32_t CParticleSystemComponent::GetMaxParticleCount() const {return m_maxParticles;}

void CParticleSystemComponent::OnRemove()
{
	BaseEnvParticleSystemComponent::OnRemove();
	for(auto &hChild : m_childSystems)
	{
		if(hChild.valid())
			hChild->GetEntity().RemoveSafely();
	}
}

void CParticleSystemComponent::Die(float maxRemainingLifetime)
{
	umath::add_flag(m_flags,Flags::Dying);
	for(auto &pt : m_particles)
	{
		pt.Die();
		if(pt.GetLife() > maxRemainingLifetime)
			pt.SetLife(maxRemainingLifetime);
	}
	for(auto &hChild : m_childSystems)
	{
		if(hChild.expired())
			continue;
		hChild->Die();
	}
}

bool CParticleSystemComponent::FindFreeParticle(uint32_t *idx)
{
	if(umath::is_flag_set(m_flags,Flags::Dying))
		return false;
	for(auto i=m_idxLast;i<m_maxParticlesCur;++i)
	{
		auto &p = m_particles[i];
		if(p.GetLife() <= 0)
		{
			m_idxLast = i +1;
			*idx = i;
			return true;
		}
	}
	if(IsContinuous() == false)
		return false;
	for(auto i=decltype(m_idxLast){0};i<m_idxLast;++i)
	{
		auto &p = m_particles[i];
		if(p.GetLife() <= 0)
		{
			m_idxLast = i;
			*idx = i;
			return true;
		}
	}
	*idx = 0;
	return true;
}

void CParticleSystemComponent::SetNodeTarget(uint32_t node,CBaseEntity *ent)
{
	if(node == 0)
		return;
	--node;
	if(node >= m_nodes.size())
		return;
	m_nodes[node].hEntity = ent->GetHandle();
	m_nodes[node].bEntity = true;
}
void CParticleSystemComponent::SetNodeTarget(uint32_t node,const Vector3 &pos)
{
	if(node == 0)
		return;
	--node;
	if(node >= m_nodes.size())
		return;
	m_nodes[node].position = pos;
	m_nodes[node].bEntity = false;
}
uint32_t CParticleSystemComponent::GetNodeCount() const {return m_nodes.size() +1;}
Vector3 CParticleSystemComponent::GetNodePosition(uint32_t node) const
{
	if(node == 0)
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		return pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{};
	}
	--node;
	if(node >= m_nodes.size() || (m_nodes[node].bEntity == true && !m_nodes[node].hEntity.IsValid()))
		return {0.f,0.f,0.f};
	if(m_nodes[node].bEntity == false)
		return m_nodes[node].position;
	auto pTrComponent = m_nodes[node].hEntity.get()->GetTransformComponent();
	return pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{};
}
CBaseEntity *CParticleSystemComponent::GetNodeTarget(uint32_t node) const
{
	if(node == 0)
		return nullptr;
	--node;
	if(node >= m_nodes.size())
		return nullptr;
	return static_cast<CBaseEntity*>(m_nodes[node].hEntity.get());
}

CallbackHandle CParticleSystemComponent::AddRenderCallback(const std::function<void(void)> &cb)
{
	auto hCb = FunctionCallback<void>::Create(cb);
	AddRenderCallback(hCb);
	return hCb;
}
void CParticleSystemComponent::AddRenderCallback(const CallbackHandle &hCb) {m_renderCallbacks.push_back(hCb);}
pragma::AlphaMode CParticleSystemComponent::GetAlphaMode() const {return m_alphaMode;}
void CParticleSystemComponent::SetAlphaMode(pragma::AlphaMode alphaMode) {m_alphaMode = alphaMode;}
void CParticleSystemComponent::SetTextureScrollingEnabled(bool b) {umath::set_flag(m_flags,Flags::TextureScrollingEnabled,b);}
bool CParticleSystemComponent::IsTextureScrollingEnabled() const {return umath::is_flag_set(m_flags,Flags::TextureScrollingEnabled);}

bool CParticleSystemComponent::IsAlphaPremultiplied() const {return umath::is_flag_set(m_flags,Flags::PremultiplyAlpha);}
void CParticleSystemComponent::SetAlphaPremultiplied(bool b) {umath::set_flag(m_flags,Flags::PremultiplyAlpha,b);}
uint32_t CParticleSystemComponent::GetEmissionRate() const {return m_emissionRate;}
void CParticleSystemComponent::SetEmissionRate(uint32_t emissionRate) {m_emissionRate = emissionRate;}
void CParticleSystemComponent::SetNextParticleEmissionCount(uint32_t count) {m_nextParticleEmissionCount = count;}
void CParticleSystemComponent::PauseEmission() {m_state = State::Paused;}
void CParticleSystemComponent::ResumeEmission()
{
	if(m_state != State::Paused)
		return;
	m_state = State::Active;
}
void CParticleSystemComponent::SetAlwaysSimulate(bool b) {umath::set_flag(m_flags,Flags::AlwaysSimulate,b);}

void CParticleSystemComponent::Render(const std::shared_ptr<prosper::PrimaryCommandBuffer> &drawCmd,Scene &scene,bool bloom)
{
	m_tLastEmission = c_game->RealTime();
	if(IsActiveOrPaused() == false)
	{
		if(umath::is_flag_set(m_flags,Flags::Dying))
			Stop();
		return;
	}
	auto numRenderParticles = GetRenderParticleCount();
	for(auto &hChild : m_childSystems)
	{
		if(hChild.expired() || hChild->IsActiveOrPaused() == false)
			continue;
		numRenderParticles += hChild->GetRenderParticleCount();
		hChild->Render(drawCmd,scene,bloom);
	}
	if(numRenderParticles == 0)
	{
		if(umath::is_flag_set(m_flags,Flags::Dying))
			Stop();
		return;
	}

	for(auto &r : m_renderers)
		r->Render(drawCmd,scene,bloom);
	umath::set_flag(m_flags,Flags::RendererBufferUpdateRequired,false);
}

void CParticleSystemComponent::RenderShadow(const std::shared_ptr<prosper::PrimaryCommandBuffer> &drawCmd,Scene &scene,pragma::CLightComponent *light,uint32_t layerId)
{
	if(!IsActiveOrPaused() || m_numRenderParticles == 0)
		return;
	for(auto &hChild : m_childSystems)
	{
		if(hChild.valid() && hChild->IsActiveOrPaused())
			hChild->RenderShadow(drawCmd,scene,light,layerId);
	}
	for(auto &r : m_renderers)
		r->RenderShadow(drawCmd,scene,*light,layerId);
}

void CParticleSystemComponent::CreateParticle(uint32_t idx)
{
	auto &particle = m_particles[idx];
	if(particle.IsAlive())
		OnParticleDestroyed(particle);
	particle.Reset(static_cast<float>(c_game->CurTime()));
	particle.SetAlive(true);
	particle.SetColor(m_initialColor);
	particle.SetLife(m_lifeTime);
	particle.SetRadius(m_radius);
	auto pos = m_origin;
	if(umath::is_flag_set(m_flags,Flags::MoveWithEmitter) == false) // If the particle is moving with the emitter, the position is added elsewhere!
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		if(pTrComponent.valid())
			pos += pTrComponent->GetPosition();
	}
	particle.SetPosition(pos);
	auto rot = m_particleRot;
	if(umath::is_flag_set(m_flags,Flags::RotateWithEmitter) == false)
	{
		auto pTrComponent = GetEntity().GetTransformComponent();
		if(pTrComponent.valid())
			rot = pTrComponent->GetOrientation() *rot;
	}
	particle.SetWorldRotation(rot);
	if(IsAnimated() == true && IsTextureScrollingEnabled() == false)
	{
		if(m_material != nullptr)
		{
			auto &data = m_material->GetDataBlock();
			if(data != nullptr)
			{
				auto &anim = data->GetValue("animation");
				if(anim != nullptr && anim->IsBlock())
				{
					auto block = std::static_pointer_cast<ds::Block>(anim);
					if(block->GetBool("start_random") == true || umath::is_flag_set(m_flags,Flags::RandomStartFrame) == true)
					{
						auto frames = block->GetInt("frames");
						auto fps = block->GetInt("fps");
						auto frame = umath::random(0,frames -1);
						auto offset = 0.f;
						if(fps > 0)
							offset = static_cast<float>(frame) /static_cast<float>(fps);
						else
							offset = static_cast<float>(frame) /static_cast<float>(frames);

						particle.SetFrameOffset(offset);
					}
				}
			}
		}
	}
	for(auto &init : m_initializers)
		init->Initialize(particle);
	for(auto &op : m_operators)
		op->Initialize(particle);
	for(auto &r : m_renderers)
		r->Initialize(particle);
}

uint32_t CParticleSystemComponent::CreateParticles(uint32_t count)
{
	auto bHasLimit = m_currentParticleLimit != std::numeric_limits<uint32_t>::max();
	if(bHasLimit)
		count = umath::min(count,m_currentParticleLimit);
	for(auto i=decltype(count){0};i<count;++i)
	{
		uint32_t idx;
		if(FindFreeParticle(&idx) == false)
		{
			if(bHasLimit)
				m_currentParticleLimit -= i;
			return i;
		}
		else
			CreateParticle(idx);
	}
	if(bHasLimit)
	{
		m_currentParticleLimit -= count;
		if(m_currentParticleLimit == 0u && IsContinuous() == true)
			m_currentParticleLimit = m_particleLimit;
	}
	return count;
}

void CParticleSystemComponent::OnComplete()
{
	if(m_bRemoveOnComplete == true)
		GetEntity().RemoveSafely();
}

void CParticleSystemComponent::Simulate(double tDelta)
{
	if(!IsActiveOrPaused())
		return;
	auto pTsComponent = GetEntity().GetTimeScaleComponent();
	if(pTsComponent.valid())
		tDelta *= pTsComponent->GetTimeScale();
	m_tLifeTime += tDelta;

	auto &scene = c_game->GetScene();
	auto &cam = scene->camera;
	m_numParticles = 0;
	m_numRenderParticles = 0;
	for(auto i=decltype(m_maxParticlesCur){0};i<m_maxParticlesCur;++i)
	{
		auto &p = m_particles[i];
		auto life = p.GetLife();
		if(life > 0.f)
		{
			life -= static_cast<float>(tDelta);
			p.SetLife(life);
			p.SetTimeAlive(p.GetTimeAlive() +static_cast<float>(tDelta));
			if(life > 0)
				m_numParticles++;
			else
				p.SetCameraDistance(-1);
		}
		if(life <= 0.f && p.IsAlive())
		{
			OnParticleDestroyed(p);
			p.SetAlive(false);
		}
	}

	// Simulate particle operators
	// This has to be before particles are created, otherwise operators like
	// "emission_rate_random" will not work properly!
	for(auto &op : m_operators)
		op->Simulate(tDelta);
	//unsigned int numRender = 0;
	if(umath::is_flag_set(m_flags,Flags::MoveWithEmitter) || umath::is_flag_set(m_flags,Flags::RotateWithEmitter))
	{
		auto pAttComponent = GetEntity().GetComponent<pragma::CAttachableComponent>();
		if(pAttComponent.valid())
			pAttComponent->UpdateAttachmentOffset();
	}

	auto bMoving = (umath::is_flag_set(m_flags,Flags::MoveWithEmitter) && GetEntity().HasStateFlag(BaseEntity::StateFlags::PositionChanged))
		|| (umath::is_flag_set(m_flags,Flags::RotateWithEmitter) && GetEntity().HasStateFlag(BaseEntity::StateFlags::RotationChanged));
	umath::set_flag(m_flags,Flags::HasMovingParticles,bMoving);
	for(auto i=decltype(m_maxParticlesCur){0};i<m_maxParticlesCur;++i)
	{
		auto &p = m_particles[i];
		if(p.GetLife() > 0.f)
		{
			for(auto &op : m_operators)
				op->PreSimulate(p,tDelta);
			for(auto &op : m_operators)
				op->Simulate(p,tDelta);

			auto velAng = p.GetAngularVelocity() *static_cast<float>(tDelta);
			if(uvec::length_sqr(velAng) > 0.f)
			{
				// Update world rotation
				auto rotOld = p.GetWorldRotation();
				auto rotNew = glm::quat_cast(glm::eulerAngleYXZ(velAng.y,velAng.x,velAng.z)) *rotOld;
				p.SetWorldRotation(rotNew);
				if(rotOld.w != rotNew.w || rotOld.x != rotNew.x || rotOld.y != rotNew.y || rotOld.z != rotNew.z)
					umath::set_flag(m_flags,Flags::HasMovingParticles,true);

				// Update sprite rotation
				auto rot = p.GetRotation();
				rot += umath::rad_to_deg(velAng.y);
				p.SetRotation(rot);
			}

			auto pos = p.GetPosition();
			auto &vel = p.GetVelocity();
			if(uvec::length(vel) > 0.f)
			{
				pos += vel *static_cast<float>(tDelta);
				p.SetPosition(pos);
				if(umath::is_flag_set(m_flags,Flags::HasMovingParticles) == false && uvec::length_sqr(vel) > 0.f)
					umath::set_flag(m_flags,Flags::HasMovingParticles,true);
			}
			p.SetCameraDistance(glm::length2(pos -cam->GetPos()));
			for(auto &op : m_operators)
				op->PostSimulate(p,tDelta);
			//numRender++;
		}
	}
	//

	auto numFill = m_maxParticlesCur -m_numParticles;
	auto bEmissionPaused = IsEmissionPaused();
	if(numFill > 0)
	{
		int32_t numCreate = 0;
		if(m_nextParticleEmissionCount != std::numeric_limits<uint32_t>::max())
		{
			if(bEmissionPaused == false)
			{
				numCreate = m_nextParticleEmissionCount;
				m_nextParticleEmissionCount = std::numeric_limits<uint32_t>::max();
				if(numCreate > 0u)
					m_tNextEmission -= static_cast<float>(tDelta);
			}
		}
		else
		{
			auto emissionRate = bEmissionPaused ? 0u : m_emissionRate;
			if(emissionRate > 0)
			{
				m_tNextEmission -= static_cast<float>(tDelta);
				auto rate = 1.f /static_cast<float>(emissionRate);
				if(m_tNextEmission <= 0.f)
				{
					numCreate = umath::floor(-m_tNextEmission /rate);
					if(numCreate == 0 && m_tLastEmission == 0.0 && rate > 0.f)
						++numCreate; // Make sure at least one particle is created right away when the particle system was started
					m_tNextEmission = -fmodf(-m_tNextEmission,rate);
				}
			}
		}
		if(numCreate > numFill)
			numCreate = numFill;
		numCreate = CreateParticles(numCreate);
		m_numParticles += numCreate;
	}

	auto bChildrenSimulated = false;
	const auto fSimulateChildren = [this,tDelta,&bChildrenSimulated]() {
		if(bChildrenSimulated)
			return;
		bChildrenSimulated = true;
		for(auto &hChild : m_childSystems)
		{
			if(hChild.valid() && hChild->IsActiveOrPaused())
				hChild->Simulate(tDelta);
		}
	};

	if(
		m_numParticles == 0 && IsContinuous() == false && m_tLastEmission != 0.0 && bEmissionPaused == false && 
		(m_currentParticleLimit == 0u || m_currentParticleLimit == std::numeric_limits<uint32_t>::max())
	)
	{
		m_renderBounds = {{},{}};
		auto bChildActive = false;
		for(auto &hChild : m_childSystems)
		{
			if(hChild.valid() && hChild->IsActiveOrPaused())
			{
				bChildActive = true;
				break;
			}
		}
		if(bChildActive == false)
		{
			// TODO: In some cases particle systems are marked as completed, even though they're not actually complete yet.
			// This can happen if no particles are being emitted for some time. Find a better way to handle this!
			m_state = State::Complete;
			OnComplete();
			return;
		}
		fSimulateChildren();
		if(umath::is_flag_set(m_flags,Flags::AlwaysSimulate) == false)
			return;
	}
	if(umath::is_flag_set(m_flags,Flags::SortParticles))
		SortParticles(); // TODO Sort every frame?
	fSimulateChildren();

	auto bStatic = IsStatic();
	auto bUpdateBounds = (bStatic == true || m_tLastEmission == 0.0 || m_maxParticlesCur != m_prevMaxParticlesCur) ? true : false;
	if(bUpdateBounds == true)
	{
		m_renderBounds.first = uvec::MAX;
		m_renderBounds.second = uvec::MIN;
	}
	if(m_maxParticlesCur > 0)
	{
		// Call render callbacks; Last chance to update particle transforms and such
		for(auto it=m_renderCallbacks.begin();it!=m_renderCallbacks.end();)
		{
			auto &hCb = *it;
			if(hCb.IsValid() == false)
			{
				it = m_renderCallbacks.erase(it);
				continue;
			}
			hCb();
			++it;
		}
	}
	auto pTrComponent = GetEntity().GetTransformComponent();
	auto psPos = pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{};
	for(auto i=decltype(m_maxParticlesCur){0};i<m_maxParticlesCur;++i)
	{
		auto sortedIdx = m_sortedParticleIndices[i];
		auto &p = m_particles[sortedIdx];
		auto radius = p.GetRadius();
		if(p.ShouldDraw() == true)
		{
			auto &data = m_instanceData[m_numRenderParticles];
			auto &pos = p.GetPosition();
			auto vCol = p.GetColor().ToVector4();
			if(umath::is_flag_set(m_flags,Flags::PremultiplyAlpha))
				pragma::premultiply_alpha(vCol,m_alphaMode);
			auto &col = data.color;
			col = {static_cast<uint16_t>(vCol.x *255.f),static_cast<uint16_t>(vCol.y *255.f),static_cast<uint16_t>(vCol.z *255.f),static_cast<uint16_t>(vCol.a *255.f)};

			auto &rot = p.GetWorldRotation();
			auto origin = p.GetOrigin() *p.GetRadius(); // TODO: What is this for?
			uvec::rotate(&origin,rot);

			data.position = Vector4{pos.x +origin.x,pos.y +origin.y,pos.z +origin.z,radius};
			if(umath::is_flag_set(m_flags,Flags::MoveWithEmitter))
			{
				for(auto i=0u;i<3u;++i)
					data.position[i] += psPos[i];
			}
			data.rotation = p.GetRotation();
			data.length = p.GetLength();
			m_particleIndicesToBufferIndices[sortedIdx] = m_numRenderParticles;
			m_bufferIndicesToParticleIndices[m_numRenderParticles] = sortedIdx;

			if(bUpdateBounds == true)
			{
				const Vector3 minExtents = {-1.f,-1.f,-1.f};
				const Vector3 maxExtents = {1.f,1.f,1.f};
				auto r = p.GetExtent();
				auto &ptPos = Vector3{data.position.x,data.position.y,data.position.z};
				auto minBounds = ptPos +minExtents *r;
				auto maxBounds = ptPos +maxExtents *r;

				uvec::to_min_max(m_renderBounds.first,m_renderBounds.second,minBounds,maxBounds);
			}
			if(m_animData != nullptr)
			{
				m_dataAnimStart[m_numRenderParticles] = p.GetFrameOffset();
				if(m_dataAnimStart[m_numRenderParticles] == 0.f && m_animData->fps > 0)
					m_dataAnimStart[m_numRenderParticles] += p.GetTimeCreated();
			}
			++m_numRenderParticles;
		}
	}
	if(m_numRenderParticles == 0)
		m_renderBounds = {{},{}};
	for(auto &r : m_renderers)
	{
		auto rendererBounds = r->GetRenderBounds();
		uvec::to_min_max(m_renderBounds.first,m_renderBounds.second,rendererBounds.first,rendererBounds.second);
	}
	auto &bufParticles = GetParticleBuffer();
	auto bUpdateBuffers = (bStatic == false || m_numRenderParticles != m_numPrevRenderParticles) ? true : false;
	m_numPrevRenderParticles = m_numRenderParticles;
	if(bufParticles != nullptr && bUpdateBuffers == true && m_numRenderParticles > 0u)
	{
		c_engine->ScheduleRecordUpdateBuffer(bufParticles,0ull,m_numRenderParticles *sizeof(ParticleData),m_instanceData.data());
		umath::set_flag(m_flags,Flags::RendererBufferUpdateRequired,true);
	}
	if(IsAnimated())
	{
		auto &bufAnimStart = GetAnimationStartBuffer();
		if(bufAnimStart != nullptr && m_numRenderParticles > 0u)
			c_engine->ScheduleRecordUpdateBuffer(bufAnimStart,0ull,m_numRenderParticles *sizeof(float),m_dataAnimStart.data());
	}
	for(auto &r : m_renderers)
		r->PostSimulate(tDelta);
}

const std::vector<util::WeakHandle<CParticleSystemComponent>> &CParticleSystemComponent::GetChildren() const {return const_cast<CParticleSystemComponent*>(this)->GetChildren();}
std::vector<util::WeakHandle<CParticleSystemComponent>> &CParticleSystemComponent::GetChildren() {return m_childSystems;}

bool CParticleSystemComponent::ShouldUseBlackAsAlpha() const {return umath::is_flag_set(m_flags,Flags::BlackToAlpha);}

std::size_t CParticleSystemComponent::TranslateParticleIndex(std::size_t particleIdx) const
{
	if(particleIdx >= m_particleIndicesToBufferIndices.size())
		return std::numeric_limits<std::size_t>::max();
	return m_particleIndicesToBufferIndices[particleIdx];
}

std::size_t CParticleSystemComponent::TranslateBufferIndex(std::size_t particleIdx) const
{
	if(particleIdx >= m_bufferIndicesToParticleIndices.size())
		return std::numeric_limits<std::size_t>::max();
	return m_bufferIndicesToParticleIndices[particleIdx];
}

void CParticleSystemComponent::SortParticles()
{
	std::sort(m_sortedParticleIndices.begin(),m_sortedParticleIndices.end(),[this](std::size_t idx0,std::size_t idx1) {
		return m_particles[idx0] < m_particles[idx1];
	});
}

void CParticleSystemComponent::OnParticleDestroyed(CParticle &particle)
{
	for(auto &init : m_initializers)
		init->Destroy(particle);
	for(auto &op : m_operators)
		op->Destroy(particle);
	for(auto &r : m_renderers)
		r->Destroy(particle);
}