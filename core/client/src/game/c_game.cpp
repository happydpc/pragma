#include "stdafx_client.h"
#include "pragma/game/c_game.h"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/model/c_polymesh.h"
#include "pragma/model/brush/c_brushmesh.h"
#include "pragma/entities/c_world.h"
#include "pragma/c_engine.h"
#include "pragma/debug/c_debugoverlay.h"
#include <wgui/wgui.h>
#include <wgui/wibase.h>
#include "pragma/gui/wiluabase.h"
#include "luasystem.h"
#include <wgui/wihandle.h>
#include "pragma/lua/classes/c_lwibase.h"
#include "cmaterialmanager.h"
#include "pragma/rendering/shaders/c_shader.h"
//#include "shader_screen.h" // prosper TODO
#include "pragma/lua/classes/c_ldef_wgui.h"
#include "pragma/lua/libraries/c_listener.h"
#include "pragma/entities/components/c_player_component.hpp"
#include "pragma/entities/components/c_render_component.hpp"
#include "pragma/entities/components/c_bsp_leaf_component.hpp"
#include "pragma/level/mapgeometry.h"
#include "pragma/model/c_modelmanager.h"
#include "pragma/gui/wiluahandlewrapper.h"
#include "pragma/entities/c_viewmodel.h"
#include "pragma/entities/c_viewbody.h"
#include "pragma/entities/c_player.hpp"
#include <pragma/physics/physobj.h>
#include "pragma/console/c_cvar.h"
#include "pragma/rendering/c_rendermode.h"
#include "pragma/rendering/shaders/post_processing/c_shader_postprocessing.h"
#include "pragma/rendering/shaders/post_processing/c_shader_hdr.hpp"
#include "pragma/ai/c_navsystem.h"
#include <texturemanager/texturemanager.h>
#include "pragma/physics/c_physdebug.h"
#include <pragma/physics/physenvironment.h>
#include "pragma/rendering/rendersystem.h"
#include "pragma/model/c_model.h"
#include "pragma/model/c_modelmesh.h"
#include <pragma/lua/luacallback.h>
#include "pragma/opengl/renderhierarchy.h"
#include "pragma/rendering/occlusion_culling/chc.hpp"
#include "pragma/rendering/scene/scene.h"
#include <pragma/performancetimer.h>
#include "pragma/rendering/c_msaa.h"
#include "pragma/gui/wgui_luainterface.h"
#include "textureinfo.h"
#include <pragma/networking/nwm_util.h>
#include "pragma/debug/renderdebuginfo.hpp"
#include "pragma/game/c_game_callback.h"
#include "pragma/rendering/shaders/image/c_shader_additive.h"
#include "pragma/opengl/opengl_helper.h"
#include "pragma/rendering/occlusion_culling/c_occlusion_octree_impl.hpp"
#include "pragma/rendering/world_environment.hpp"
#include "pragma/audio/c_sound_efx.hpp"
#include <pragma/lua/luafunction_call.h>
#include "pragma/networking/wvclient.h"
#include "pragma/rendering/lighting/c_light.h"
#include "pragma/rendering/lighting/shadows/c_shadowmap.h"
#include "pragma/rendering/shaders/world/c_shader_textured.hpp"
#include "pragma/rendering/shaders/c_shader_lua.hpp"
#include "pragma/entities/c_entityfactories.h"
#include "pragma/entities/components/c_weapon_component.hpp"
#include "pragma/entities/components/c_player_component.hpp"
#include "pragma/entities/components/c_character_component.hpp"
#include "pragma/entities/components/c_vehicle_component.hpp"
#include "pragma/entities/components/c_ai_component.hpp"
#include "pragma/entities/components/c_physics_component.hpp"
#include "pragma/rendering/c_settings.hpp"
#include <pragma/entities/baseplayer.hpp>
#include <pragma/game/game_clear_resources.h>
#include <pragma/util/giblet_create_info.hpp>
#include <alsound_effect.hpp>
#include <alsoundsystem.hpp>
#include <prosper_command_buffer.hpp>
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/components/base_physics_component.hpp>
#include <pragma/entities/components/velocity_component.hpp>
#include <prosper_util.hpp>
#include <pragma/entities/entity_iterator.hpp>
#include <pragma/entities/components/map_component.hpp>
#include <pragma/networking/snapshot_flags.hpp>

extern EntityClassMap<CBaseEntity> *g_ClientEntityFactories;
extern ClientEntityNetworkMap *g_ClEntityNetworkMap;
extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
DLLCLIENT CGame *c_game = NULL;
DLLCLIENT PhysEnv *c_physEnv = NULL;

CGame::MessagePacketTracker::MessagePacketTracker()
	: lastInMessageId(0),outMessageId(0)
{
	std::fill(messageTimestamps.begin(),messageTimestamps.end(),0);
}

bool CGame::MessagePacketTracker::IsMessageInOrder(uint8_t messageId) const
{
	return (messageTimestamps[messageId] > messageTimestamps[lastInMessageId]) ? true : false;
}

void CGame::MessagePacketTracker::CheckMessages(uint8_t messageId,std::vector<double> &lostPacketTimestamps,const double &tCur)
{
	if(messageId != static_cast<decltype(messageId)>(lastInMessageId +1)) // We've lost at least 1 packet
	{
		for(auto id=static_cast<decltype(messageId)>(lastInMessageId +1);id!=messageId;++id) // Assume all messages inbetween are lost packets
			lostPacketTimestamps.push_back(tCur);
	}
	lastInMessageId = messageId;
}

void CGame::MessagePacketTracker::CheckMessages(uint8_t messageId,std::vector<double> &lostPacketTimestamps)
{
	if(messageId != static_cast<decltype(messageId)>(lastInMessageId +1)) // We've lost at least 1 packet
	{
		for(auto id=static_cast<decltype(messageId)>(lastInMessageId +1);id!=messageId;++id) // Check which packets we've lost
		{
			auto &ts = messageTimestamps[id];
			if(ts < messageTimestamps[messageId] && ts > messageTimestamps[lastInMessageId]) // We've lost this packet
				lostPacketTimestamps.push_back(ts);
		}
	}
	lastInMessageId = messageId;
}

//////////////////////////

CGame::CGame(NetworkState *state)
	: Game(state),
	m_tServer(0),m_renderScene(NULL),
	m_matOverride(NULL),m_colScale(1,1,1,1),
	//m_shaderOverride(NULL), // prosper TODO
#ifdef PHYS_ENGINE_BULLET
	m_btDebugDraw(NULL),
#endif
	m_matLoad(),m_scene(nullptr),m_camPosOverride(nullptr),m_camRotOverride(nullptr),
	/*m_dummyVertexBuffer(nullptr),*/m_tLastClientUpdate(0.0), // prosper TODO
	m_snapshotTracker{},m_userInputTracker{}
{
	std::fill(m_renderModesEnabled.begin(),m_renderModesEnabled.end(),true);
	c_game = this;

	m_luaShaderManager = std::make_shared<pragma::LuaShaderManager>();

	RegisterCallback<void,CGame*>("OnGameEnd");
	RegisterCallback<void,pragma::CLightDirectionalComponent*,pragma::CLightDirectionalComponent*>("OnEnvironmentLightSourceChanged");
	RegisterCallback<void>("PreRenderSkybox");
	RegisterCallback<void>("PostRenderSkybox");
	RegisterCallback<void>("PreRenderWorld");
	RegisterCallback<void>("PostRenderWorld");
	RegisterCallback<void>("PreRenderParticles");
	RegisterCallback<void>("PostRenderParticles");
	RegisterCallback<void>("Render");
	RegisterCallback<void>("PreRenderDebug");
	RegisterCallback<void>("PostRenderDebug");
	RegisterCallback<void>("PreRenderWater");
	RegisterCallback<void>("PostRenderWater");
	RegisterCallback<void>("PreRenderView");
	RegisterCallback<void>("PostRenderView");
	RegisterCallback<void>("PreRenderScenes");
	RegisterCallbackWithOptionalReturn<bool,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>,std::reference_wrapper<std::shared_ptr<prosper::RenderTarget>>>("DrawScene");
	RegisterCallback<void>("PostRenderScenes");
	RegisterCallback<void>("PostRenderScenes");
	RegisterCallback<void,FRender>("RenderPostProcessing");
	RegisterCallback<void>("OnPreRender");
	RegisterCallback<void>("RenderPrepass");
	RegisterCallback<void>("PostRenderScene");
	RegisterCallback<void,std::reference_wrapper<Vector3>,std::reference_wrapper<Quat>>("CalcView");
	RegisterCallback<void,std::reference_wrapper<Vector3>,std::reference_wrapper<Quat>>("CalcViewOffset");
	RegisterCallback<
		void,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>,
		std::reference_wrapper<std::shared_ptr<prosper::RenderTarget>>
	>("PreRender");
	RegisterCallback<
		void,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>,
		std::reference_wrapper<std::shared_ptr<prosper::RenderTarget>>
	>("PostRender");
	RegisterCallback<void,CBaseEntity*>("UpdateEntityModel");

	auto &staticCallbacks = get_static_client_callbacks();
	for(auto it=staticCallbacks.begin();it!=staticCallbacks.end();++it)
	{
		auto &name = it->first;
		auto &hCallback = it->second;
		AddCallback(name,hCallback);
	}
#ifdef PHYS_ENGINE_BULLET
	c_physEnv = m_physEnvironment.get();
	m_btDebugDraw = new WVBtIDebugDraw;
	m_btDebugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);//btIDebugDraw::DBG_MAX_DEBUG_DRAW_MODE);
	auto *world = m_physEnvironment->GetWorld();
	world->setDebugDrawer(m_btDebugDraw);
#endif

	LoadAuxEffects("fx_generic.txt");
	for(auto &rsnd : client->GetSounds())
	{
		auto &snd = static_cast<CALSound&>(rsnd.get());
		snd.SetPitchModifier(GetTimeScale()); // TODO Implement SetPitchModifier for SoundScripts
	}
	AddCallback("OnSoundCreated",FunctionCallback<void,ALSound*>::Create([](ALSound *snd) {
		auto *csnd = dynamic_cast<CALSound*>(snd);
		if(csnd == nullptr)
			return;
		csnd->SetPitchModifier(c_game->GetTimeScale());
	}));

	/*m_dummyVertexBuffer = Vulkan::Buffer::Create(c_engine->GetRenderContext(),Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT,
		umath::to_integral(GameLimits::MaxMeshVertices) *sizeof(Vector4), // Allocate about 50MB of junk data (For ~1'872'457 vertices)
		nullptr,true,nullptr
	);*/ // prosper TODO

	AddCallback("OnEntitySpawned",FunctionCallback<void,BaseEntity*>::Create([this](BaseEntity *ent) {
		// Add entity to scene
		m_scene->AddEntity(*static_cast<CBaseEntity*>(ent));
	}));
}

CGame::~CGame()
{
	c_engine->WaitIdle();
	if(m_hCbDrawFrame.IsValid())
		m_hCbDrawFrame.Remove();
	CallCallbacks<void,CGame*>("OnGameEnd",this);
	while(!m_luaGUIObjects.empty())
	{
		auto &hElement = m_luaGUIObjects.front();
		if(hElement.IsValid())
			hElement->Remove();
		m_luaGUIObjects.pop();
	}
	Lua::gui::clear_lua_callbacks(GetLuaState());

	DebugRenderer::ClearObjects();

	SetTimeScale(1.f);

	for(unsigned int i=0;i<m_ents.size();i++)
	{
		if(m_ents[i] != NULL)
		{
			m_ents[i]->OnRemove();
			m_ents[i]->Remove();
		}
	}
	m_listener = {};
	m_worldComponent = {};
	m_plLocal = {};
	m_viewModel = {};
	m_viewBody = {};
	CModelManager::MarkAllForDeletion();

	c_engine->ClearLuaKeyMappings();
	/*auto shaders = ShaderSystem::get_shaders();
	for(auto *shader : shaders)
	{
		auto *base = dynamic_cast<Shader::Lua::Base*>(shader);
		if(base != nullptr)
			base->ClearLuaObject();
	}*/ // prosper TODO

	if(m_camPosOverride != NULL)
		delete m_camPosOverride;
	if(m_camRotOverride != NULL)
		delete m_camRotOverride;

	ShadowMap::ClearShadowMapDepthBuffers();

#ifdef PHYS_ENGINE_BULLET
	if(m_btDebugDraw != NULL)
		delete m_btDebugDraw;
	c_physEnv = NULL;
#endif

	ClearSoundCache();
}

std::shared_ptr<pragma::EntityComponentManager> CGame::InitializeEntityComponentManager() {return std::make_shared<pragma::CEntityComponentManager>();;}

void CGame::OnReceivedRegisterNetEvent(NetPacket &packet)
{
	auto name = packet->ReadString();
	Game::GetEntityNetEventManager().RegisterNetEvent(name);
}

pragma::NetEventId CGame::SetupNetEvent(const std::string &name) {return FindNetEvent(name);}

std::shared_ptr<pragma::nav::Mesh> CGame::LoadNavMesh(const std::string &fname) {return pragma::nav::CMesh::Load(*this,fname);}

WorldEnvironment &CGame::GetWorldEnvironment() {return *m_worldEnvironment;}
const WorldEnvironment &CGame::GetWorldEnvironment() const {return const_cast<CGame*>(this)->GetWorldEnvironment();}

void CGame::InitializeWorldEnvironment()
{
	m_worldEnvironment = WorldEnvironment::Create();
}

void CGame::SetRenderClipPlane(const Vector4 &clipPlane) {m_clipPlane = clipPlane;}
const Vector4 &CGame::GetRenderClipPlane() const {return m_clipPlane;}

pragma::CParticleSystemComponent *CGame::CreateParticleTracer(const Vector3 &start,const Vector3 &end,float radius,const Color &col,float length,float speed,const std::string &material,float bloomScale)
{
	std::stringstream ssColor;
	ssColor<<col.r<<" "<<col.g<<" "<<col.b<<" "<<col.a;
	std::unordered_map<std::string,std::string> values {
		{"maxparticles","1"},
		{"max_node_count","2"},
		{"emission_rate","10000"},
		{"material",material},
		{"radius",std::to_string(radius)},
		{"color",ssColor.str()},
		{"sort_particles","0"},
		{"bloom_scale",std::to_string(bloomScale)}
	};
	auto *particle = pragma::CParticleSystemComponent::Create(values,nullptr);
	if(particle == nullptr)
		return nullptr;
	std::unordered_map<std::string,std::string> beamValues {
		{"node_start","1"},
		{"node_end","2"},
		{"curvature","0.0"}
	};
	particle->AddRenderer("beam",beamValues);
	auto hParticle = particle->GetHandle<pragma::CParticleSystemComponent>();

	auto dir = end -start;
	auto prevDist = uvec::length(dir);
	if(prevDist > 0.f)
		dir /= prevDist;
	particle->SetNodeTarget(1,start);
	particle->SetNodeTarget(2,start +dir *static_cast<float>(length));
	particle->Start();
	auto cb = FunctionCallback<void>::Create(nullptr);
	auto cStart = start;
	auto bSkip = true;
	static_cast<Callback<void>*>(cb.get())->SetFunction([cb,hParticle,cStart,dir,end,speed,length,prevDist,bSkip]() mutable {
		if(hParticle.expired())
		{
			cb.Remove();
			return;
		}
		if(bSkip == true) // Skip the first tick (Wait until particle has actually spawned!)
		{
			bSkip = false;
			return;
		}
		auto &tDelta = c_game->DeltaTime();
		cStart += dir *static_cast<float>(speed *tDelta);
		auto dist = uvec::distance(cStart,end);
		length = umath::min(static_cast<float>(length),dist);
		if(length <= 0.f || dist > prevDist)
		{
			hParticle->GetEntity().Remove();
			cb.Remove();
			return;
		}
		auto nEnd = cStart +dir *static_cast<float>(length);
		auto *pt = hParticle.get();
		pt->SetNodeTarget(1,cStart);
		pt->SetNodeTarget(2,nEnd);
		prevDist = dist;
	});
	c_game->AddCallback("Think",cb);
	return particle;
}

void CGame::SetRenderModeEnabled(RenderMode renderMode,bool bEnabled) {m_renderModesEnabled[umath::to_integral(renderMode)] = bEnabled;}
void CGame::EnableRenderMode(RenderMode renderMode) {SetRenderModeEnabled(renderMode,true);}
void CGame::DisableRenderMode(RenderMode renderMode) {SetRenderModeEnabled(renderMode,false);}
bool CGame::IsRenderModeEnabled(RenderMode renderMode) const {return m_renderModesEnabled[umath::to_integral(renderMode)];}

//const Vulkan::Buffer &CGame::GetDummyVertexBuffer() const {return m_dummyVertexBuffer;} // prosper TODO

Material *CGame::GetLoadMaterial() {return m_matLoad.get();}
void CGame::OnEntityCreated(BaseEntity *ent)
{
	Game::OnEntityCreated(ent);
}

void CGame::SetCameraPosition(Vector3 *pos,Quat *rot)
{
	if(m_camPosOverride != NULL)
		delete m_camPosOverride;
	if(pos == NULL)
	{
		m_camPosOverride = NULL;
		//CPlayer *pl = GetLocalPlayer();
		//if(pl != NULL)
		//	pl->SetObserverMode(OBSERVERMODE::FIRSTPERSON);
	}
	else
	{
		m_camPosOverride = new Vector3(*pos);
		//CPlayer *pl = GetLocalPlayer();
		//if(pl != NULL)
		//	pl->SetObserverMode(OBSERVERMODE::ROAMING);
	}
	if(m_camRotOverride != NULL)
		delete m_camRotOverride;
	if(rot == NULL)
		m_camRotOverride = NULL;
	else
		m_camRotOverride = new Quat(*rot);
}
void CGame::SetCameraOrientation(Quat *rot)
{
	m_camRotOverride = rot;
}

pragma::CViewModelComponent *CGame::GetViewModel()
{
	if(m_viewModel.expired())
		return NULL;
	return m_viewModel.get();
}

pragma::CViewBodyComponent *CGame::GetViewBody()
{
	if(m_viewBody.expired())
		return NULL;
	return m_viewBody.get();
}

static void shader_handler(Material *mat)
{
	if(mat->IsLoaded() == false)
		return;
	auto &shaderManager = c_engine->GetShaderManager();
	auto whShader = shaderManager.GetShader(mat->GetShaderIdentifier());
	if(whShader.expired())
		return;
	auto &cmat = *static_cast<CMaterial*>(mat);
	// Clear descriptor set in case the shader has changed; The new shader will re-create it appropriately
	cmat.SetDescriptorSetGroup(*whShader.get(),nullptr);

	auto *shader = dynamic_cast<pragma::ShaderTexturedBase*>(whShader.get());
	auto *pShader = dynamic_cast<prosper::Shader*>(shader);
	if(pShader == nullptr || pShader->IsValid() == false)
		return;
	mat->SetUserData(shader);
	shader->InitializeMaterialDescriptorSet(cmat,false);
}

void CGame::ReloadMaterialShader(CMaterial *mat)
{
	auto *shader = static_cast<pragma::ShaderTexturedBase*>(mat->GetUserData());
	if(shader == nullptr)
		return;
	shader->InitializeMaterialDescriptorSet(*mat,true);
}

void CGame::Initialize()
{
	Game::Initialize();
	auto &materialManager = static_cast<CMaterialManager&>(client->GetMaterialManager());
	materialManager.SetShaderHandler(&shader_handler);
	pragma::CRenderComponent::InitializeBuffers();
	pragma::CLightComponent::InitializeBuffers();
	CModelSubMesh::InitializeBuffers();
	pragma::CParticleSystemComponent::InitializeBuffers();

	InitShaders();

	m_globalRenderSettingsBufferData = std::make_unique<GlobalRenderSettingsBufferData>();
	ShadowMap::InitializeDescriptorSet();
	pragma::CParticleSystemComponent::Precache("impact");
	pragma::CParticleSystemComponent::Precache("muzzleflash");
	pragma::CParticleSystemComponent::Precache("explosion");

	// Initialize Scene (Has to be initialized AFTER shaders!)

	InitializeWorldEnvironment();

	m_scene = Scene::Create(Scene::CreateInfo{c_engine->GetWindowWidth(),c_engine->GetWindowHeight(),GetConVarFloat("cl_render_fov"),GetConVarFloat("cl_fov_viewmodel")/*,c_engine->GetAspectRatio()*/,c_engine->GetNearZ(),c_engine->GetFarZ()});
	m_scene->SetWorldEnvironment(GetWorldEnvironment());
	m_scene->SetSSAOEnabled(GetConVarBool("cl_render_ssao"));

	SetRenderScene(m_scene);

	Resize();

	m_matLoad = client->LoadMaterial("loading",CallbackHandle{},false,true);
}

void CGame::InitializeGame() // Called by NET_cl_resourcecomplete
{
	InitializeLua(); // Lua has to be initialized completely before any entites are created
	SetupLua();

	m_hCbDrawFrame = c_engine->AddCallback("DrawFrame",FunctionCallback<void,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>>::Create([this](std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>> drawCmd) {
		auto baseDrawCmd = std::static_pointer_cast<prosper::CommandBuffer>(drawCmd.get());
		CallLuaCallbacks<void,std::shared_ptr<prosper::CommandBuffer>>("DrawFrame",baseDrawCmd);
	}));

	auto &materialManager = static_cast<CMaterialManager&>(client->GetMaterialManager());
	materialManager.ReloadMaterialShaders();
	m_surfaceMaterials.Load("scripts\\physics\\materials.txt");

	c_engine->SavePipelineCache();

	CallCallbacks<void,Game*>("OnGameInitialized",this);
	m_flags |= GameFlags::GameInitialized;
}

void CGame::RequestResource(const std::string &fileName)
{
	Con::ccl<<"[CGame] RequestResource '"<<fileName<<"'"<<Con::endl;
	auto fName = FileManager::GetCanonicalizedPath(fileName);
	auto it = std::find(m_requestedResources.begin(),m_requestedResources.end(),fName);
	if(it != m_requestedResources.end())
		return;
	m_requestedResources.push_back(fName);
	NetPacket p;
	p->WriteString(fName);
	client->SendPacketTCP("query_resource",p);
	Con::ccl<<"[CGame] Request sent!"<<Con::endl;
}

void CGame::Resize()
{
	if(m_scene == nullptr)
		return;
	ReloadRenderFrameBuffer();
	auto &cam = GetSceneCamera();
	cam.SetAspectRatio(c_engine->GetAspectRatio());
	cam.UpdateMatrices();
}

void CGame::PreGUIDraw()
{
	CallLuaCallbacks("PreGUIDraw");
}
void CGame::PostGUIDraw()
{
	CallLuaCallbacks("PostGUIDraw");
}
void CGame::SetRenderScene(const std::shared_ptr<Scene> &scene)
{
	if(scene == nullptr)
	{
		m_renderScene = m_scene;
		return;
	}
	m_renderScene = scene;
}
std::shared_ptr<Scene> &CGame::GetRenderScene() {return m_renderScene;}
Camera *CGame::GetRenderCamera() const
{
	if(m_renderScene == nullptr)
		return nullptr;
	return m_renderScene->camera.get();
}

void CGame::SetMaterialOverride(Material *mat) {m_matOverride = mat;}
Material *CGame::GetMaterialOverride() {return m_matOverride;}

void CGame::SetColorScale(const Vector4 &col) {m_colScale = col;}
Vector4 &CGame::GetColorScale() {return m_colScale;}
void CGame::SetAlphaScale(float a) {m_colScale.a = a;}
float CGame::GetAlphaScale() {return m_colScale.a;}

WIBase *CGame::CreateGUIElement(std::string className,WIBase *parent)
{
	auto *o = m_luaGUIElements.GetClassObject(className);
	auto &gui = WGUI::GetInstance();
	WIBase *el = nullptr;
	if(o != nullptr)
	{
		luabind::object r;
#ifndef LUABIND_NO_EXCEPTIONS
		try
		{
#endif
			r = (*o)();
#ifndef LUABIND_NO_EXCEPTIONS
		}
		catch(luabind::error&)
		{
			Lua::HandleLuaError(GetLuaState());
			return nullptr;
		}
#endif
		if(!r)
		{
			Con::csv<<"WARNING: Unable to create lua GUI Element '"<<className<<"'!"<<Con::endl;
			return nullptr;
		}
		if(luabind::object_cast_nothrow<WILuaHandle*>(r))
		{
			el = new WILuaBase(r,className);
			gui.Setup<WILuaBase>(el,parent);
		}
		else
		{
			Con::csv<<"WARNING: Unable to create lua GUI Element '"<<className<<"': Lua class is not derived from valid GUI base!"<<Con::endl;
			return nullptr;
		}
	}
	else
		el = gui.Create(className,parent);
	if(el == nullptr)
	{
		static auto skipLuaFile = false;
		if(skipLuaFile == true)
			return nullptr;
		auto lclassName = className;
		ustring::to_lower(lclassName);
		auto luaPath = "gui/" +lclassName +".lua";
		if(FileManager::Exists("lua/" +luaPath) && ExecuteLuaFile(luaPath))
		{
			o = m_luaGUIElements.GetClassObject(className);
			if(o != nullptr)
			{
				skipLuaFile = true;
				auto *r = CreateGUIElement(className,parent);
				skipLuaFile = false;
				return r;
			}
		}
		return nullptr;
	}
	/*if(bLuaEntity == true)
	{
		luabind::object *o = WGUILuaInterface::GetLuaObject(m_lua,p);
		o->push(m_lua);
		int idxObj = Lua::GetStackTop(m_lua);
		Lua::PushNil(m_lua);
		while(Lua::GetNextPair(m_lua,data) != 0)
		{
			Lua::PushValue(m_lua,-2); // We need the key for the next iteration
			Lua::Insert(m_lua,-2);
			Lua::SetTableValue(m_lua,idxObj);
		}
		static_cast<WILuaBase*>(p)->SetUp(name);
	}*/
	m_luaGUIObjects.push(el->GetHandle());
	return el;
	/*StringToLower(name);
	int ref = Lua::PushTable(m_lua,m_luaRefGUITable);
	Lua::PushString(m_lua,name);
	Lua::GetTableValue(m_lua,ref);
	bool setup = Lua::IsTable(m_lua,-1);
	int data = Lua::GetStackTop(m_lua);
	WIBase *p;
	if(setup)
		p = static_cast<WIBase*>(WGUI::Create<WILuaBase>(parent));
	else
	{
		p = WGUI::Create(name,parent);
		if(p == NULL)
			return NULL;
	}
	if(setup) // It's a lua-scripted gui element
	{
		luabind::object *o = WGUILuaInterface::GetLuaObject(m_lua,p);
		o->push(m_lua);
		int idxObj = Lua::GetStackTop(m_lua);
		Lua::PushNil(m_lua);
		while(Lua::GetNextPair(m_lua,data) != 0)
		{
			Lua::PushValue(m_lua,-2); // We need the key for the next iteration
			Lua::Insert(m_lua,-2);
			Lua::SetTableValue(m_lua,idxObj);
		}
		static_cast<WILuaBase*>(p)->SetUp(name);
	}
	m_luaGUIObjects.push(p->CreateHandle());
	return p;*/
}

static CVar cvLODBias = GetClientConVar("cl_render_lod_bias");
void CGame::SetLODBias(int32_t bias) {client->SetConVar("cl_render_lod_bias",std::to_string(bias));}
int32_t CGame::GetLODBias() const {return cvLODBias->GetInt();}
uint32_t CGame::GetLOD(float dist,uint32_t maxLod) const
{
	auto lod = CUInt32(dist /LOD_SWAP_DISTANCE) +GetLODBias();
	if(lod < 0)
		lod = 0;
	if(lod > maxLod)
		lod = maxLod;
	return static_cast<uint32_t>(lod);
}

void CGame::SetFogOverride(const std::shared_ptr<prosper::DescriptorSetGroup> &descSetGroup) {m_descSetGroupFogOverride = descSetGroup;}

void CGame::CreateGiblet(const GibletCreateInfo &info,pragma::CParticleSystemComponent **particle)
{
	if(particle != nullptr)
		*particle = nullptr;
	if(info.lifetime <= 0.f)
		return;
	auto *pt = pragma::CParticleSystemComponent::Create({
		{"maxparticles","1"},
		{"emission_rate","10000"},
		{"cast_shadows","1"},
		{"radius",std::to_string(info.scale)},
		{"world_rotation",std::to_string(info.rotation.w) +" " +std::to_string(info.rotation.x) +" " +std::to_string(info.rotation.y) +" " +std::to_string(info.rotation.z)}
	});
	if(pt == nullptr)
		return;
	pt->AddInitializer("lifetime_random",{
		{"lifetime_min",std::to_string(info.lifetime)},
		{"lifetime_max",std::to_string(info.lifetime)}
	});
	pt->AddInitializer("initial_velocity",{
		{"velocity",std::to_string(info.velocity.x) +" " +std::to_string(info.velocity.y) +" " +std::to_string(info.velocity.z)}
	});
	pt->AddInitializer("initial_angular_velocity",{
		{"velocity",std::to_string(info.angularVelocity.x) +" " +std::to_string(info.angularVelocity.y) +" " +std::to_string(info.angularVelocity.z)}
	});
	pt->AddRenderer("model",{
		{"model",info.model},
		{"skin",std::to_string(info.skin)}
	});

	if(info.physShape != GibletCreateInfo::PhysShape::None)
	{
		std::stringstream ssTranslationOffset;
		ssTranslationOffset<<info.physTranslationOffset.x<<" "<<info.physTranslationOffset.y<<" "<<info.physTranslationOffset.z;
		std::stringstream ssRotationOffset;
		ssRotationOffset<<info.physRotationOffset.p<<" "<<info.physRotationOffset.y<<" "<<info.physRotationOffset.r;
		switch(info.physShape)
		{
			case GibletCreateInfo::PhysShape::Model:
				pt->AddOperator("physics_model",{
					{"model",info.model},
					{"mass",std::to_string(info.mass)},
					{"translation_offset",ssTranslationOffset.str()},
					{"rotation_offset",ssRotationOffset.str()}
				});
				break;
			case GibletCreateInfo::PhysShape::Sphere:
				pt->AddOperator("physics_sphere",{
					{"mass",std::to_string(info.mass)},
					{"translation_offset",ssTranslationOffset.str()},
					{"rotation_offset",ssRotationOffset.str()},
					{"radius",std::to_string(info.physRadius)}
				});
				break;
			case GibletCreateInfo::PhysShape::Box:
				pt->AddOperator("physics_box",{
					{"mass",std::to_string(info.mass)},
					{"translation_offset",ssTranslationOffset.str()},
					{"rotation_offset",ssRotationOffset.str()},
					{"extent",std::to_string(info.physRadius)}
				});
				break;
			case GibletCreateInfo::PhysShape::Cylinder:
				pt->AddOperator("physics_cylinder",{
					{"mass",std::to_string(info.mass)},
					{"translation_offset",ssTranslationOffset.str()},
					{"rotation_offset",ssRotationOffset.str()},
					{"radius",std::to_string(info.physRadius)},
					{"height",std::to_string(info.physHeight)}
				});
				break;
		}
	}
	pt->AddOperator("gravity",{
		{"effective_scale","1.0"}
	});
	pt->AddOperator("color_fade",{
		{"alpha","0"},
		{"fade_start",std::to_string(info.lifetime -0.2f)},
		{"fade_end",std::to_string(info.lifetime)}
	});
	auto pTrComponent = pt->GetEntity().GetTransformComponent();
	if(pTrComponent.valid())
		pTrComponent->SetPosition(info.position);
	pt->SetRemoveOnComplete(true);
	pt->Start();
	if(particle != nullptr)
		*particle = pt;
}

void CGame::CreateGiblet(const GibletCreateInfo &info) {CreateGiblet(info,nullptr);}

WIBase *CGame::CreateGUIElement(std::string name,WIHandle *hParent)
{
	StringToLower(name);
	WIBase *pParent = NULL;
	if(hParent != NULL && hParent->IsValid())
		pParent = hParent->get();
	return CreateGUIElement(name,pParent);
}
LuaGUIManager &CGame::GetLuaGUIManager() {return m_luaGUIElements;}
pragma::LuaShaderManager &CGame::GetLuaShaderManager() {return *m_luaShaderManager;}

void CGame::SetUp()
{
	CListener *listener = CreateEntity<CListener>();
	m_listener = listener->GetComponent<pragma::CListenerComponent>();

	CViewModel *vm = CreateEntity<CViewModel>();
	m_viewModel = vm->GetComponent<pragma::CViewModelComponent>();

	CViewBody *body = CreateEntity<CViewBody>();
	m_viewBody = body->GetComponent<pragma::CViewBodyComponent>();
}

#include "pragma/rendering/lighting/shadows/c_shadowmap.h"

bool WriteTGA(const char *name,int w,int h,unsigned char *pixels,int size);

void WriteCubeMapSide(int w,int,int blockSize,int block,float *inPixels,unsigned char *outPixels)
{
	//Scene &scene = *c_engine->GetScene(0);
	//Camera &cam = scene.camera;
	float n = 1.f;//cam.GetZNear();
	float f = 1000.f;//cam.GetZFar();
	int numBlocksW = w /blockSize;
	//int numBlocksH = h /blockSize;
	unsigned int offset = (block %4) *blockSize *3 +(block /4) *(w *blockSize) *3;
	int p = 0;
	for(int y=0;y<blockSize *3;y+=3)
	{
		for(int x=0;x<blockSize *3;x+=3)
		{
			float v = inPixels[p];
			v = (2.f *n) /(f +n -v *(f -n));
			v *= 255.f;

			outPixels[offset +x] = CUChar(v);
			outPixels[offset +x +1] = CUChar(v);
			outPixels[offset +x +2] = CUChar(v);
			p++;
		}
		offset += blockSize *numBlocksW *3;
	}
}

bool SaveCubeMapAsTGA(const char *name,unsigned int texture,int,int,int w,int h,unsigned int)
{
	Con::crit<<"Saving cubemap texture as '"<<name<<"'..."<<Con::endl;
	int sz = w *h;
	float *inPixels = new float[sz];
	unsigned char *outPixels = new unsigned char[sz *12 *3];
	for(int i=0;i<(sz *12 *3);i++)
		outPixels[i] = 128;
	for(int i=0;i<6;i++)
	{
		 // prosper TODO
#if 0
		int err = glGetError();
		/*glFramebufferTexture2D(
			GL_READ_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X +i,
			texture,
			0
		);
		err = glGetError();

		glReadPixels(
			0,0,
			w,h,
			GL_DEPTH_COMPONENT,
			GL_FLOAT,
			&inPixels[0]
		);*/
		/*for(int i=0;i<sz;i++)
		{
			if(inPixels[i] < 1.f)
				inPixels[i] = 0;
		}*/
		err = glGetError();
		std::cout<<err<<std::endl;
#endif
		//glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,&inPixels[0]);
		//for(int i=0;i<sz;i++)
		//	inPixels[i] = 255;
		//unsigned int offset;
		switch(i)
		{
		case 0:
			WriteCubeMapSide(w *4,h *3,w,6,&inPixels[0],&outPixels[0]);
			break;
		case 1:
			WriteCubeMapSide(w *4,h *3,w,4,&inPixels[0],&outPixels[0]);
			break;
		case 2:
			WriteCubeMapSide(w *4,h *3,w,1,&inPixels[0],&outPixels[0]);
			break;
		case 3:
			WriteCubeMapSide(w *4,h *3,w,9,&inPixels[0],&outPixels[0]);
			break;
		case 4:
			WriteCubeMapSide(w *4,h *3,w,5,&inPixels[0],&outPixels[0]);
			break;
		case 5:
			WriteCubeMapSide(w *4,h *3,w,7,&inPixels[0],&outPixels[0]);
			break;
		}
	}
	delete[] inPixels;
	bool b = WriteTGA(name,w *4,h *3,outPixels,sz *12 *3);
	delete[] outPixels;
	return b;
}

static CVar cvAntiAliasing = GetClientConVar("cl_render_anti_aliasing");
static CVar cvMsaaSamples = GetClientConVar("cl_render_msaa_samples");
uint32_t CGame::GetMSAASampleCount()
{
	auto bMsaaEnabled = static_cast<AntiAliasing>(cvAntiAliasing->GetInt()) == AntiAliasing::MSAA;
	unsigned int numSamples = bMsaaEnabled ? umath::pow(2,cvMsaaSamples->GetInt()) : 0;
	ClampMSAASampleCount(&numSamples);
	return numSamples;
}
void CGame::ReloadRenderFrameBuffer()
{
	if(m_scene != nullptr)
		m_scene->ReloadRenderTarget();
}

void CGame::Think()
{
#ifdef ENABLE_PERFORMANCE_TIMER
	static unsigned int thinkTimer = PerformanceTimer::InitializeTimer("CGame::Think");
	PerformanceTimer::StartMeasurement(thinkTimer);
#endif
	Game::Think();
	auto &scene = GetRenderScene();
	auto &cam = scene->camera;
	cam->UpdateFrustumPlanes();
	//m_entsOccluded.clear();
	//GetOccludedEntities(m_entsOccluded);

	double tDelta = m_stateNetwork->DeltaTime();
	m_tServer += DeltaTime();
	CalcLocalPlayerOrientation();
	CallCallbacks<void>("Think");
	CallLuaCallbacks("Think");
	CalcView();

	SetRenderScene(m_scene);

	auto &info = get_render_debug_info();
	info.Reset();
	
	//Render(); // Vulkan TODO // TODO: Interpolation
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
	//glBlitFramebuffer(0,0,800,600,0,0,800,600,GL_COLOR_BUFFER_BIT,GL_NEAREST);
	//std::cout<<"Error: "<<err<<std::endl;
	//CallCallbacks<void>("RenderScreen"); // Vulkan TODO

	//OpenGL::ClearColor(1,0,0,1);
	//OpenGL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//OpenGL::ClearColor(0,0,0,1);
	//OpenGL::SetViewPort(0,0,w,h);

//#if RENDER_TO_TEXTURE
	//OpenGL::BindFrameBuffer(m_renderFBO,GL_FRAMEBUFFER); // Vulkan TODO
	{
		//CallLuaCallbacks("RenderScreen"); // Vulkan TODO
	};

	//int w,h;
	//OpenGL::GetViewportSize(&w,&h);
	// Render from multi-sample to screen frame buffer
	//OpenGL::BindFrameBuffer(m_renderFBO,GL_READ_FRAMEBUFFER);
	//OpenGL::BindFrameBuffer(m_renderFBOSingleSample,GL_DRAW_FRAMEBUFFER);
	//OpenGL::BlitFrameBuffer(w,h); // Multi-Sample texture to regular texture // Vulkan TODO

	//auto outputFbo = m_renderFBOSingleSample;
	//auto outputTex = m_renderTextureSingleSample;

	//static auto *shader = static_cast<ShaderAdditive*>(GetShader("additive")); // Vulkan TODO
	// Add Bloom
	/*OpenGL::BindFrameBuffer(m_tempFBO,GL_DRAW_FRAMEBUFFER);
	if(shader != nullptr)
	{
		shader->Render(outputTex,m_bloomTexture,ShaderTexturedBase::GetQuadVertexBuffer());
		outputFbo = m_tempFBO;
		outputTex = m_tempTexture;
	}*/ // Vulkan TODO
	//

	// Add Glow

	/*if(shader != nullptr && m_bFirstGlow == false) // No need for this step if there are no glowing objects on screen
	{
		const float additiveScale = 4.f;
		OpenGL::BindFrameBuffer(m_renderFBOPostProcessing,GL_DRAW_FRAMEBUFFER);
		shader->Render(outputTex,m_glowBlurTexture,ShaderTexturedBase::GetQuadVertexBuffer(),additiveScale);
		outputFbo = m_renderFBOPostProcessing;
		outputTex = m_renderTexturePostProcessing;
	}*/ // Vulkan TODO
	//

	//OpenGL::BindFrameBuffer(outputFbo,GL_READ_FRAMEBUFFER);
	//CallCallbacks<void,unsigned int,unsigned int>("RenderPostProcessing",outputFbo,outputTex); // Vulkan TODO

	// Apply post-processing effects
	/*static auto *pp = static_cast<ShaderPostProcessing*>(GetShader("postprocessing"));
	if(pp != nullptr)
		pp->Render(m_renderFBOScreen,outputTex);
	//
	*/ // Vulkan TODO
	// Convert HDR to 0-1 color range
	/*OpenGL::BindFrameBuffer(m_renderFBOScreen,GL_READ_FRAMEBUFFER);
	OpenGL::BindFrameBuffer(m_hdrInfo.fbo,GL_DRAW_FRAMEBUFFER);
	//static auto *shaderHdr = static_cast<ShaderHDR*>(GetShader("hdr"));
	//if(shaderHdr != nullptr)
	//	shaderHdr->Render(m_renderTextureScreen,ShaderTexturedBase::GetQuadVertexBuffer()); // Vulkan TODO
	//
	
	// Render the result to the main screen
	OpenGL::BindFrameBuffer(m_hdrInfo.fbo,GL_READ_FRAMEBUFFER);
	OpenGL::BindFrameBuffer(0,GL_DRAW_FRAMEBUFFER);
	OpenGL::BlitFrameBuffer(w,h);
	OpenGL::BindFrameBuffer(0,GL_READ_FRAMEBUFFER);*/ // Vulkan TODO
	//

	PostThink();
#ifdef ENABLE_PERFORMANCE_TIMER
	PerformanceTimer::EndMeasurement(thinkTimer);
#endif
}

static CVar cvUpdateRate = GetClientConVar("cl_updaterate");
void CGame::Tick()
{
#ifdef ENABLE_PERFORMANCE_TIMER
	static unsigned int tickTimer = PerformanceTimer::InitializeTimer("CGame::Tick");
	PerformanceTimer::StartMeasurement(tickTimer);
#endif
	Game::Tick();
	//HandlePlayerMovement();
	auto &t = RealTime();
	auto updateRate = cvUpdateRate->GetFloat();
	if(updateRate > 0.f && t -m_tLastClientUpdate >= (1.0 /static_cast<double>(updateRate)))
	{
		m_tLastClientUpdate = t;
		SendUserInput();
	}
	CallCallbacks<void>("Tick");
	CallLuaCallbacks("Tick");
	PostTick();
#ifdef ENABLE_PERFORMANCE_TIMER
	PerformanceTimer::EndMeasurement(tickTimer);
#endif
}

static CVar cvSimEnabled = GetClientConVar("cl_physics_simulation_enabled");
bool CGame::IsPhysicsSimulationEnabled() const {return cvSimEnabled->GetBool();}

const util::WeakHandle<prosper::Shader> &CGame::GetGameShader(GameShader shader) const {return m_gameShaders.at(umath::to_integral(shader));}

LuaCallbackHandler &CGame::GetInputCallbackHandler() {return m_inputCallbackHandler;}

std::shared_ptr<Model> CGame::CreateModel(const std::string &mdl) const {return CModelManager::Create(const_cast<CGame*>(this),mdl);}
std::shared_ptr<Model> CGame::CreateModel(bool bAddReference) const {return CModelManager::Create(const_cast<CGame*>(this),bAddReference);}
std::shared_ptr<BrushMesh> CGame::CreateBrushMesh() const {return std::make_shared<CBrushMesh>();}
std::shared_ptr<Side> CGame::CreateSide() const {return std::make_shared<CSide>();}
std::shared_ptr<ModelMesh> CGame::CreateModelMesh() const {return std::make_shared<CModelMesh>();}
std::shared_ptr<ModelSubMesh> CGame::CreateModelSubMesh() const {return std::make_shared<CModelSubMesh>();}
std::shared_ptr<Model> CGame::LoadModel(const std::string &mdl,bool bReload)
{
	auto bNewModel = false;
	auto r = CModelManager::Load(this,mdl,bReload,&bNewModel);
	if(bNewModel == true && r != nullptr)
	{
		CallCallbacks<void,std::reference_wrapper<std::shared_ptr<Model>>>("OnModelLoaded",r);
		CallLuaCallbacks<void,std::shared_ptr<Model>>("OnModelLoaded",r);
	}
	return r;
}
std::unordered_map<std::string,std::shared_ptr<Model>> &CGame::GetModels() const {return CModelManager::GetModels();}

Float CGame::GetHDRExposure() const {return m_scene->GetHDRExposure();}
Float CGame::GetMaxHDRExposure() const {return m_scene->GetMaxHDRExposure();}
void CGame::SetMaxHDRExposure(Float exposure) {m_scene->SetMaxHDRExposure(exposure);}

bool CGame::LoadMap(const char *map,const Vector3 &origin,std::vector<EntityHandle> *entities)
{
	bool r = Game::LoadMap(map,origin,entities);
	ClearResources<CModelManager>();
	if(r == true)
	{
		CallCallbacks<void>("OnMapLoaded");
		CallLuaCallbacks<void>("OnMapLoaded");
	}
	m_flags |= GameFlags::MapLoaded;

	std::string dsp = "fx_";
	dsp += map;
	dsp += ".txt";
	LoadAuxEffects(dsp.c_str());
	return r;
}

void CGame::LoadMapEntities(uint32_t version,const char*,VFilePtr f,const pragma::level::BSPInputData &bspInputData,std::vector<Material*> &materials,const Vector3 &origin,std::vector<EntityHandle> *entities)
{
	auto lightMap = pragma::CLightMapComponent::LoadLightMap(const_cast<pragma::level::BSPInputData&>(bspInputData));
	std::vector<std::vector<Vector2>> lightMapUvCoordinates {}; // Light map uv coordinates per mesh per entity (used to build a global lightmap uv buffer for the world)

	struct EntityUvData
	{
		EntityHandle hEntity;
		size_t start;
		size_t numUvSets;
	};
	std::vector<EntityUvData> lightMapUvCoordinateRanges {}; // Range into lightMapUvCoordinates indicating which uv sets belong to which entity

	unsigned int numEnts = f->Read<unsigned int>();
	std::vector<EntityHandle> ents;
	ents.reserve(numEnts);
	if(entities != nullptr)
		entities->reserve(entities->size() +numEnts);
	for(unsigned int i=0;i<numEnts;i++)
	{
		auto startOffset = f->Tell();
		auto offsetToEndOfEntity = std::numeric_limits<uint64_t>::max();
		if(version > 1)
		{
			offsetToEndOfEntity = startOffset +f->Read<uint64_t>();
			if(version < 4)
				offsetToEndOfEntity += sizeof(uint64_t);
		}

		auto offsetMeshes = f->Tell();
		offsetMeshes += f->Read<uint64_t>();
		if(version < 4)
			offsetMeshes += sizeof(uint64_t);

		auto offsetLeaves = 0ull;
		if(version >= 9)
		{
			offsetLeaves = f->Tell();
			offsetLeaves += f->Read<uint64_t>();
		}

		auto bClientsideEntity = false;
		if(version >= 4)
		{
			enum class EntityFlags : uint64_t
			{
				None = 0ull,
				ClientsideOnly = 1ull
			};
			auto flags = f->Read<uint64_t>();
			bClientsideEntity = (flags &umath::to_integral(EntityFlags::ClientsideOnly)) != 0;
			if(bClientsideEntity == true)
			{
				auto className = f->ReadString();
				CreateMapEntity(version,className,f,bspInputData,materials,origin,offsetToEndOfEntity,ents,entities);
			}
		}

		BaseEntity *ent = nullptr;
		if(bClientsideEntity == false)
		{
			EntityIterator entIt {*this,EntityIterator::FilterFlags::Default | EntityIterator::FilterFlags::Pending};
			entIt.AttachFilter<TEntityIteratorFilterComponent<pragma::MapComponent>>();
			entIt.AttachFilter<EntityIteratorFilterUser>([this](BaseEntity &ent) -> bool {
				auto pMapComponent = ent.GetComponent<pragma::MapComponent>();
				return pMapComponent->GetMapIndex() == m_mapEntityIdx;
			});

			auto it = entIt.begin();
			if(it != entIt.end())
				ent = *it;
		}
		if(ent == nullptr && offsetToEndOfEntity != std::numeric_limits<uint64_t>::max())
			f->Seek(offsetToEndOfEntity);
		else
		{
			if(offsetLeaves != 0ull)
			{
				f->Seek(offsetLeaves);
				auto numLeaves = f->Read<uint32_t>();
				if(numLeaves > 0u)
				{
					auto pBspLeafComponent = ent->AddComponent<pragma::CBSPLeafComponent>();
					std::vector<uint16_t> leaves {};
					leaves.resize(numLeaves);
					f->Read(leaves.data(),leaves.size() *sizeof(leaves.front()));
					pBspLeafComponent->SetLeaves(leaves);
				}
			}
			f->Seek(offsetMeshes);
			pragma::level::load_map_brushes(*this,version,f,ent,materials,GetSurfaceMaterials(),origin);
			if(version >= 8)
			{
				auto uvSetOffset = lightMapUvCoordinates.size();
				pragma::level::load_map_faces(*this,f,*ent,bspInputData,materials,&lightMapUvCoordinates);
				auto numUvSets = lightMapUvCoordinates.size() -uvSetOffset;
				if(numUvSets > 0u)
					lightMapUvCoordinateRanges.push_back({(ent != nullptr) ? ent->GetHandle() : EntityHandle{},uvSetOffset,numUvSets});
			}
			if(ent != NULL)
			{
				if(ent->IsWorld())
				{
					auto pWorldComponent = ent->GetComponent<pragma::CWorldComponent>();
					if(pWorldComponent.valid())
						pWorldComponent->SetBSPTree(std::make_shared<util::BSPTree>(bspInputData.bspTree)); // TODO: Avoid copy
				}
				auto mdlComponent = ent->GetModelComponent();
				auto mdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
				if(mdl == nullptr)
				{
					auto pRenderComponent = static_cast<CBaseEntity*>(ent)->GetRenderComponent();
					if(pRenderComponent.valid())
					{
						Vector3 min {};
						Vector3 max {};
						auto pPhysComponent = ent->GetPhysicsComponent();
						if(pPhysComponent.valid())
							pPhysComponent->GetCollisionBounds(&min,&max);
						pRenderComponent->SetRenderBounds(min,max);
					}
				}
				auto pMapComponent = ent->GetComponent<pragma::MapComponent>();
				if(pMapComponent.valid())
					pMapComponent->SetMapIndex(m_mapEntityIdx);
				auto pTrComponent = ent->GetTransformComponent();
				if(pTrComponent.valid())
					pTrComponent->SetPosition(pTrComponent->GetPosition() +origin);
				ents.push_back(ent->GetHandle());
				if(entities != nullptr)
					entities->push_back(ent->GetHandle());
			}
			f->Seek(offsetToEndOfEntity);
		}
		m_mapEntityIdx++;
	}

	if(bspInputData.lightMapInfo.luxelData.empty() == false)
	{
		std::vector<std::shared_ptr<prosper::Buffer>> uvBuffers {};
		auto lightMapUvBuffer = pragma::CLightMapComponent::LoadLightMapUvBuffers(lightMapUvCoordinates,uvBuffers);
		for(auto &uvData : lightMapUvCoordinateRanges)
		{
			if(uvData.hEntity.IsValid() == false)
				continue;
			auto pLightMapComponent = uvData.hEntity->AddComponent<pragma::CLightMapComponent>();
			if(pLightMapComponent.expired())
				continue;
			std::vector<std::shared_ptr<prosper::Buffer>> entityUvBuffers {};
			entityUvBuffers.reserve(uvData.numUvSets);
			for(auto i=uvData.start;i<(uvData.start +uvData.numUvSets);++i)
				entityUvBuffers.push_back(uvBuffers.at(i));
			pLightMapComponent->InitializeLightMapData(lightMap,lightMapUvBuffer,entityUvBuffers);
		}
	}

	for(auto &hEnt : ents)
	{
		if(hEnt.IsValid() == false)
			continue;
		hEnt->Spawn();
	}

	EntityIterator itEnt {*this,(EntityIterator::FilterFlags::Default &~EntityIterator::FilterFlags::Spawned) | EntityIterator::FilterFlags::Pending};
	itEnt.AttachFilter<TEntityIteratorFilterComponent<pragma::MapComponent>>();
	for(auto *ent : itEnt)
	{
		auto pMapComponent = ent->GetComponent<pragma::MapComponent>();
		if(pMapComponent->GetMapIndex() == 0u)
			continue;
		Con::cwar<<"WARNING: Uninitialized map-entity found after map creation! Removing..."<<Con::endl;
		ent->RemoveSafely();
	}

	auto &scene = GetScene();
	if(scene != nullptr)
	{
		scene->ReloadOcclusionCullingHandler(); // Required if BSP occlusion culling is specified
		if(lightMap != nullptr)
			scene->SetLightMap(lightMap);
	}
}

void CGame::BuildVMF(const char*)
{
	//Game::BuildVMF<CWorld,CPolyMesh,CPoly,CBrushMesh>(map);
}

void CGame::LoadMapMaterials(uint32_t,VFilePtr f,std::vector<Material*> &materials) {pragma::level::load_map_materials(GetNetworkState(),f,materials);}

static CVar cvTimescale = GetClientConVar("host_timescale");
float CGame::GetTimeScale()
{
	return cvTimescale->GetFloat();
}

void CGame::SetTimeScale(float t)
{
	Game::SetTimeScale(t);
	for(auto &rsnd : client->GetSounds())
	{
		auto &snd = static_cast<CALSound&>(rsnd.get());
		snd.SetPitchModifier(t); // TODO Implement SetPitchModifier for SoundScripts
	}
}

bool CGame::IsServer() {return false;}
bool CGame::IsClient() {return true;}

void CGame::SetLocalPlayer(pragma::CPlayerComponent *pl)
{
	m_plLocal = pl->GetHandle<pragma::CPlayerComponent>();
	pl->SetLocalPlayer(true);
}

void CGame::OnReceivedPlayerInputResponse(uint8_t userInputId)
{
	if(m_userInputTracker.IsMessageInOrder(userInputId) == false)
		return;
	m_userInputTracker.CheckMessages(userInputId,m_lostPackets);
	//UpdateLatency(m_userInputTracker.messageTimestamps[userInputId]);
}

uint16_t CGame::GetLatency() const
{
	auto *cl = client->GetClient();
	if(cl == nullptr)
		return 0;
	return cl->GetLatency();
}

void CGame::SendUserInput()
{
	auto *pl = GetLocalPlayer();
	if(pl == NULL)
		return;
	m_userInputTracker.messageTimestamps[m_userInputTracker.outMessageId] = RealTime();

	NetPacket p;
	p->Write<uint8_t>(m_userInputTracker.outMessageId++);
	//p->Write<uint8_t>(m_latency); // Deprecated: Let the networkmanager handle it
	auto &ent = pl->GetEntity();
	auto charComponent = ent.GetCharacterComponent();
	auto pTrComponent = ent.GetTransformComponent();
	auto orientation = charComponent.valid() ? charComponent->GetViewOrientation() : pTrComponent.valid() ? pTrComponent->GetOrientation() : uquat::identity();
	nwm::write_quat(p,orientation);
	p->Write<Vector3>(pl->GetViewPos());

	auto actions = pl->GetActionInputs();
	p->Write<Action>(actions);
	auto bControllers = c_engine->GetControllersEnabled();
	p->Write<bool>(bControllers);
	if(bControllers == true)
	{
		auto actionValues = umath::get_power_of_2_values(umath::to_integral(actions));
		for(auto v : actionValues)
			p->Write<float>(pl->GetActionInputAxisMagnitude(static_cast<Action>(v)));
	}
	client->SendPacketUDP("userinput",p);
}

double &CGame::ServerTime() {return m_tServer;}
void CGame::SetServerTime(double t) {m_tServer = t;}

bool CGame::RunLua(const std::string &lua) {return Game::RunLua(lua,"lua_run_cl");}

void CGame::UpdateLostPackets()
{
	auto &t = RealTime();
	while(!m_lostPackets.empty() && t -m_lostPackets.front() > 1.0)
		m_lostPackets.erase(m_lostPackets.begin());
}

uint32_t CGame::GetLostPacketCount()
{
	UpdateLostPackets();
	return static_cast<uint32_t>(m_lostPackets.size());
}

void CGame::ReceiveSnapshot(NetPacket &packet)
{
	//Con::ccl<<"[CLIENT] Received snapshot.."<<Con::endl;
	//auto tOld = m_tServer;
	auto latency = GetLatency() /2.f; // Latency is entire roundtrip; We need the time for one way
	auto tActivated = (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() -packet.GetTimeActivated()) /1'000'000.0;
	//Con::ccl<<"Snapshot delay: "<<+latency<<"+ "<<tActivated<<" = "<<(latency +tActivated)<<Con::endl;
	auto tDelta = static_cast<float>((latency +tActivated) /1'000.0);

	auto snapshotId = packet->Read<uint8_t>();
	m_tServer = packet->Read<double>();

	auto &t = RealTime();
	m_snapshotTracker.messageTimestamps[snapshotId] = m_tServer;
	if(m_snapshotTracker.IsMessageInOrder(snapshotId) == false)
		return; // Old snapshot; Just skip it (We're already received a newer snapshot, this one's out of order)
	m_snapshotTracker.CheckMessages(snapshotId,m_lostPackets,t);

	//std::cout<<"Received snapshot with "<<(m_tServer -tOld)<<" time difference to last snapshot"<<std::endl;
	const auto maxCorrectionDistance = umath::pow2(10.f);
	unsigned int numEnts = packet->Read<unsigned int>();
	for(unsigned int i=0;i<numEnts;i++)
	{
		CBaseEntity *ent = static_cast<CBaseEntity*>(nwm::read_entity(packet));
		Vector3 pos = nwm::read_vector(packet);
		Vector3 vel = nwm::read_vector(packet);
		Vector3 angVel = nwm::read_vector(packet);
		auto orientation = nwm::read_quat(packet);
		auto entDataSize = packet->Read<UInt8>();
		if(ent != NULL)
		{
			pos += vel *tDelta;
			if(uvec::length_sqr(angVel) > 0.0)
				orientation = uquat::create(EulerAngles(umath::rad_to_deg(angVel.x),umath::rad_to_deg(angVel.y),umath::rad_to_deg(angVel.z)) *tDelta) *orientation; // TODO: Check if this is correct

			// Move the entity to the correct position without teleporting it.
			// Teleporting can lead to odd physics glitches.
			auto pTrComponent = ent->GetTransformComponent();
			auto posEnt = pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{};
			auto correctionVel = pos -posEnt;
			auto l = uvec::length_sqr(correctionVel);
			if(l > maxCorrectionDistance)
			{
				if(pTrComponent.valid())
					pTrComponent->SetPosition(pos); // Too far away, just snap into position
			}
			else
			{
				auto *pPhysComponent = static_cast<pragma::CPhysicsComponent*>(ent->GetPhysicsComponent().get());
				if(pPhysComponent != nullptr)
					pPhysComponent->SetLinearCorrectionVelocity(pos -posEnt);
			}
			//
			auto pVelComponent = ent->GetComponent<pragma::VelocityComponent>();
			if(pVelComponent.valid())
			{
				pVelComponent->SetVelocity(vel);
				pVelComponent->SetAngularVelocity(angVel);
			}
			if(pTrComponent.valid())
				pTrComponent->SetOrientation(orientation);
			ent->ReceiveSnapshotData(packet);
		}
		else
			packet->SetOffset(packet->GetOffset() +entDataSize);

		auto flags = packet->Read<pragma::SnapshotFlags>();
		if((flags &pragma::SnapshotFlags::PhysicsData) != pragma::SnapshotFlags::None)
		{
			auto numObjs = packet->Read<uint8_t>();
			if(ent != NULL)
			{
				auto pPhysComponent = ent->GetPhysicsComponent();
				PhysObj *physObj = pPhysComponent.valid() ? pPhysComponent->GetPhysicsObject() : nullptr;
				if(physObj != NULL && !physObj->IsStatic())
				{
					auto colObjs = physObj->GetCollisionObjects();
					auto numActualObjs = colObjs.size();
					for(auto i=decltype(numObjs){0};i<numObjs;++i)
					{
						auto pos = packet->Read<Vector3>();
						auto rot = packet->Read<Quat>();
						auto vel = packet->Read<Vector3>();
						auto angVel = packet->Read<Vector3>();
						if(physObj->IsController())
						{
							auto *physController = static_cast<ControllerPhysObj*>(physObj);
							//physController->SetPosition(pos);
							physController->SetOrientation(rot);
							physController->SetLinearVelocity(vel);
							physController->SetAngularVelocity(angVel);
						}
						else if(i < numActualObjs)
						{
							auto &hObj = colObjs[i];
							if(hObj.IsValid())
							{
								pos += vel *tDelta;
								auto l = uvec::length_sqr(angVel);
								if(l > 0.0)
									rot = uquat::create(EulerAngles(umath::rad_to_deg(angVel.x),umath::rad_to_deg(angVel.y),umath::rad_to_deg(angVel.z)) *tDelta) *rot; // TODO: Check if this is correct
								auto *o = static_cast<PhysCollisionObject*>(hObj.get());
								o->SetRotation(rot);
								if(o->IsRigid())
								{
									auto *rigid = static_cast<PhysRigidBody*>(o);

									//auto correctionVel = pos -rigid->GetPos();
									//auto l = uvec::length_sqr(correctionVel);
									//if(l > maxCorrectionDistance)
										rigid->SetPos(pos); // Too far away, just snap into position
									//else
									//	rigid->SetLinearCorrectionVelocity(pos -ent->GetPosition());

									rigid->SetLinearVelocity(vel);
									rigid->SetAngularVelocity(angVel);
								}
								else
									o->SetPos(pos);
							}
						}
					}
				}
				else
					packet->SetOffset(packet->GetOffset() +numObjs *(sizeof(Vector3) *3 +sizeof(Quat)));
			}
			else
				packet->SetOffset(packet->GetOffset() +numObjs *(sizeof(Vector3) *3 +sizeof(Quat)));
		}

		if((flags &pragma::SnapshotFlags::ComponentData) != pragma::SnapshotFlags::None)
		{
			auto &componentManager = static_cast<pragma::CEntityComponentManager&>(GetEntityComponentManager());
			auto &svComponentToClComponentTable = componentManager.GetServerComponentIdToClientComponentIdTable();
			auto &componentTypes = componentManager.GetRegisteredComponentTypes();
			auto numComponents = packet->Read<uint8_t>();
			for(auto i=decltype(numComponents){0};i<numComponents;++i)
			{
				auto svId = packet->Read<pragma::ComponentId>();
				auto componentSize = packet->Read<uint8_t>();
				auto componentEndOffset = packet->GetOffset() +componentSize;
				if(ent != nullptr && svId < svComponentToClComponentTable.size() && svComponentToClComponentTable.at(svId) != pragma::CEntityComponentManager::INVALID_COMPONENT)
				{
					auto clId = svComponentToClComponentTable.at(svId);
					if(clId >= componentTypes.size())
						throw std::runtime_error("Invalid client component type index " +std::to_string(clId) +"!");
					auto pComponent = ent->FindComponent(clId);
					if(pComponent.valid())
					{
						auto *pSnapshotComponent = dynamic_cast<pragma::CBaseSnapshotComponent*>(pComponent.get());
						if(pSnapshotComponent != nullptr)
							pSnapshotComponent->ReceiveSnapshotData(packet);
					}
				}

				packet->SetOffset(componentEndOffset);
			}
		}
	}

	unsigned char numPlayers = packet->Read<unsigned char>();
	for(int i=0;i<numPlayers;i++)
	{
		auto *plComponent = nwm::read_player(packet);
		auto *pl = (plComponent != nullptr) ? static_cast<CPlayer*>(plComponent->GetBasePlayer()) : nullptr;
		auto orientation = nwm::read_quat(packet);
		unsigned char numKeys = packet->Read<unsigned char>();
		for(int i=0;i<numKeys;i++)
		{
			unsigned short key = packet->Read<unsigned short>();
			UNUSED(key);
			char pressed = packet->Read<char>();
			int action = (pressed == 1) ? GLFW_PRESS : GLFW_RELEASE;
			UNUSED(action);
			//if(ent != NULL && pl != plLocal)
			//	KeyCallback(pl,key,action); // WEAVETODO
		}
		/*if(ent != nullptr)
		{
			static Quat last(0.f,0.f,0.f,0.f);
			if(orientation.x != last.x || orientation.y != last.y || orientation.z != last.z || orientation.w != last.w)
			{
				last = orientation;
				std::cout<<"Orientation Update: ("<<orientation.w<<","<<orientation.x<<","<<orientation.y<<","<<orientation.z<<")"<<std::endl;
			}
		}*/
		if(plComponent != NULL && pl->IsCharacter())
		{
			auto charComponent = pl->GetCharacterComponent();
			charComponent->SetViewOrientation(orientation);
		}
	}
}

static void set_action_input(Action action,bool b,bool bKeepMagnitude,const float *inMagnitude=nullptr)
{
	auto magnitude = 0.f;
	if(inMagnitude != nullptr)
		magnitude = *inMagnitude;
	else if(bKeepMagnitude == false)
		magnitude = (b == true) ? 1.f : 0.f;
	auto *pl = c_game->GetLocalPlayer();
	if(pl == nullptr)
		return;
	if(bKeepMagnitude == false)
		pl->SetActionInputAxisMagnitude(action,magnitude);
	if(b == false)
	{
		//if(!GetRawActionInputs(action))
		//	return;
		pl->SetActionInput(action,b,true);
		return;
	}
	if(pl->GetRawActionInput(action))
		return;
	pl->SetActionInput(action,b,true);
}
void CGame::SetActionInput(Action action,bool b,bool bKeepMagnitude) {set_action_input(action,b,bKeepMagnitude);}
void CGame::SetActionInput(Action action,bool b,float magnitude) {set_action_input(action,b,false,&magnitude);}

bool CGame::GetActionInput(Action action)
{
	auto *pl = GetLocalPlayer();
	if(pl == NULL)
		return false;
	return pl->GetActionInput(action);
}

void CGame::DrawLine(const Vector3 &start,const Vector3 &end,const Color &color,float duration)
{
	DebugRenderer::DrawLine(start,end,color,duration);
}
void CGame::DrawBox(const Vector3 &start,const Vector3 &end,const EulerAngles &ang,const Color &color,float duration)
{
	DebugRenderer::DrawBox(start,end,ang,color,duration);
}
void CGame::DrawPlane(const Vector3 &n,float dist,const Color &color,float duration)
{
	DebugRenderer::DrawPlane(n,dist,color,duration);
}

bool CGame::LoadAuxEffects(const std::string &fname)
{
	std::string path = "scripts\\soundfx\\";
	path += fname;
	auto f = FileManager::OpenFile(path.c_str(),"r");
	std::shared_ptr<ds::Block> root = nullptr;
	if(f != NULL)
		root = std::shared_ptr<ds::Block>(ds::System::ReadData(f));
	if(root == NULL)
		return false;
	auto *data = root->GetData();
	if(data == NULL)
		return false;
	for(auto it=data->begin();it!=data->end();it++)
	{
		std::shared_ptr<ds::Block> block = nullptr;
		if(it->second->IsBlock())
			block = std::static_pointer_cast<ds::Block>(it->second);
		else if(it->second->IsContainer())
			block = it->second->GetBlock(0);
		if(block != NULL)
		{
			std::string name = it->first;
			StringToLower(name);
			std::string type = block->GetString("type");
			al::create_aux_effect(name,type,*block);
		}
	}
	return true;
}
std::shared_ptr<al::Effect> CGame::GetAuxEffect(const std::string &name) {return c_engine->GetAuxEffect(name);}

static CVar cvFriction = GetClientConVar("sv_friction");
Float CGame::GetFrictionScale() const
{
	return cvFriction->GetFloat();
}
static CVar cvRestitution = GetClientConVar("sv_restitution");
Float CGame::GetRestitutionScale() const
{
	return cvRestitution->GetFloat();
}