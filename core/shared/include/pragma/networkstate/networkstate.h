#ifndef __NETWORKSTATE_H__
#define __NETWORKSTATE_H__
#include "pragma/networkdefinitions.h"
#include <pragma/lua/luaapi.h>
#include <pragma/physics/physapi.h>
#include <sharedutils/callback_handler.h>
#include <pragma/console/cvar_handler.h>
#include <sharedutils/chronotime.h>
#include "pragma/lua/luafunction.h"
#include <pragma/console/fcvar.h>
#include <sharedutils/util_cpu_profiler.hpp>
#include <pragma/input/key_state.hpp>

#define GLFW_RELEASE                0
#define GLFW_PRESS                  1

#define CHECK_CHEATS(scmd,state,ret) \
	{ \
		if(state->CheatsEnabled() == false) \
		{ \
			Con::cout<<"Can't use cheat cvar "<<scmd<<" in multiplayer, unless the server has sv_cheats set to 1."<<Con::endl; \
			return ret; \
		} \
	}

struct DLLNETWORK SoundCacheInfo
{
	SoundCacheInfo()
		: duration(0.f),mono(false),stereo(false)
	{}
	float duration;
	bool mono;
	bool stereo;
};

class ALSound;
enum class DLLNETWORK ALChannel : uint32_t
{
	Auto = 0,
	Mono,
	Both
};

enum class DLLNETWORK ALCreateFlags : uint32_t
{
	None = 0,
	Mono = 1,
	Stream = Mono<<1,
	DontTransmit = Stream<<1 // Serverside only
};
REGISTER_BASIC_BITWISE_OPERATORS(ALCreateFlags);

class ConConf;
class ConVar;
class ConVarHandle;
class PtrConVar;
class ConCommand;
class ConVarMap;
class TCallback;
class Game;
struct MapInfo;
class CvarCallback;
class SoundScriptManager;
class SoundScript;
class Material;
class ModelMesh;
class ModelSubMesh;
class MaterialManager;
class ResourceWatcherManager;
enum class ALSoundType : int32_t;
namespace Lua {enum class ErrorColorMode : uint32_t;class Interface;};
namespace util {class Library;};
using ALSoundRef = std::reference_wrapper<ALSound>;
class DLLNETWORK NetworkState
	: public CallbackHandler,public CVarHandler
{
// For internal use only! Not to be used directly!
protected:
	static ConVarHandle GetConVarHandle(std::unordered_map<std::string,std::shared_ptr<PtrConVar>> &ptrs,std::string scvar);
public:
	virtual std::unordered_map<std::string,std::shared_ptr<PtrConVar>> &GetConVarPtrs()=0;
//
protected:
	static UInt8 STATE_COUNT;
	std::unique_ptr<MapInfo> m_mapInfo = nullptr;
	bool m_bTCPOnly;
	bool m_bTerminateSocket;
	std::vector<CallbackHandle> m_luaEnumRegisterCallbacks;
	std::unique_ptr<ResourceWatcherManager> m_resourceWatcher;

	std::unordered_map<std::string,std::string> m_conOverrides;
	ChronoTime m_ctReal;
	double m_tReal;
	double m_tDelta;
	double m_tLast;
	std::unique_ptr<SoundScriptManager> m_soundScriptManager = nullptr;
	std::vector<CallbackHandle> m_thinkCallbacks;
	std::vector<CallbackHandle> m_tickCallbacks;

	std::vector<std::shared_ptr<util::Library>> m_libHandles;
	std::shared_ptr<util::Library> m_lastModuleHandle = nullptr;
	static std::unordered_map<std::string,std::shared_ptr<util::Library>> s_loadedLibraries;
	std::unordered_map<lua_State*,std::vector<std::shared_ptr<util::Library>>> m_initializedLibraries;

	void InitializeDLLModule(lua_State *l,std::shared_ptr<util::Library> module);

	virtual void InitializeResourceManager();
	void ClearGameConVars();
	virtual void implFindSimilarConVars(const std::string &input,std::vector<SimilarCmdInfo> &similarCmds) const override;
public:
	// Internal
	std::vector<CallbackHandle> &GetLuaEnumRegisterCallbacks();
	void TerminateLuaModules(lua_State *l);
	void DeregisterLuaModules(void *l,const std::string &identifier);
	virtual bool ShouldRemoveSound(ALSound &snd);

	ResourceWatcherManager &GetResourceWatcher();

	// Textures
	virtual Material *LoadMaterial(const std::string &path,bool bReload=false);
	bool PortMaterial(const std::string &path,const std::function<Material*(const std::string&,bool)> &fLoadMaterial);
	MapInfo *GetMapInfo();
	std::string GetMap();
	virtual void Close();
	virtual void Initialize();
	virtual void Think();
	virtual void Tick();
	// Lua
	lua_State *GetLuaState();
	virtual Lua::ErrorColorMode GetLuaErrorColorMode()=0;
	static void RegisterSharedLuaGlobals(Lua::Interface &lua);
	static void RegisterSharedLuaClasses(Lua::Interface &lua);
	static void RegisterSharedLuaLibraries(Lua::Interface &lua);
#ifdef PHYS_ENGINE_PHYSX
	// PhysX
	physx::PxPhysics *GetPhysics();
#endif
	// Time
	double &RealTime();
	double &DeltaTime();
	double &LastThink();
	void WriteToLog(std::string str);

	void AddThinkCallback(CallbackHandle callback);
	void AddTickCallback(CallbackHandle callback);
	
	void InitializeLuaModules(lua_State *l);
	virtual std::shared_ptr<util::Library> InitializeLibrary(std::string library,std::string *err=nullptr,lua_State *l=nullptr);
	std::shared_ptr<util::Library> LoadLibraryModule(const std::string &lib,const std::vector<std::string> &additionalSearchDirectories={},std::string *err=nullptr);
	std::shared_ptr<util::Library> GetLibraryModule(const std::string &lib) const;

	std::unordered_map<std::string,unsigned int> &GetConCommandIDs();

	// Sound
	std::vector<ALSoundRef> m_sounds;
	std::unordered_map<std::string,std::shared_ptr<SoundCacheInfo>> m_soundsPrecached;
	void UpdateSounds(std::vector<std::shared_ptr<ALSound>> &sounds);
public:
	NetworkState();
	virtual ~NetworkState();
	virtual bool IsServer() const;
	virtual bool IsClient() const;
	virtual bool IsMultiPlayer() const=0;
	virtual bool IsSinglePlayer() const=0;
	bool CheatsEnabled() const;
	virtual MaterialManager &GetMaterialManager()=0;
	virtual ModelSubMesh *CreateSubMesh() const=0;
	virtual ModelMesh *CreateMesh() const=0;

	void TranslateConsoleCommand(std::string &cmd);
	void SetConsoleCommandOverride(const std::string &src,const std::string &dst);
	void ClearConsoleCommandOverride(const std::string &src);
	void ClearConsoleCommandOverrides();

	// Game
	virtual void StartGame();
	virtual void EndGame();
	virtual Game *GetGameState();
	virtual bool IsGameActive();
	virtual void LoadMap(const char *map,bool bDontReload=false);

	// Sound
	float GetSoundDuration(std::string snd);
	virtual std::shared_ptr<ALSound> CreateSound(std::string snd,ALSoundType type,ALCreateFlags flags=ALCreateFlags::None)=0;
	virtual void UpdateSounds()=0;
	virtual bool PrecacheSound(std::string snd,ALChannel mode=ALChannel::Auto)=0;
	virtual void StopSounds()=0;
	virtual void StopSound(std::shared_ptr<ALSound> pSnd)=0;
	virtual std::shared_ptr<ALSound> GetSoundByIndex(unsigned int idx)=0;
	const std::vector<ALSoundRef> &GetSounds() const;
	std::vector<ALSoundRef> &GetSounds();

	SoundScriptManager *GetSoundScriptManager();
	SoundScript *FindSoundScript(const char *name);
	virtual bool LoadSoundScripts(const char *file,bool bPrecache=false);
	Bool IsSoundPrecached(const std::string &snd) const;

	// ConVars
	virtual ConVarMap *GetConVarMap() override;
	virtual bool RunConsoleCommand(std::string scmd,std::vector<std::string> &argv,pragma::BasePlayerComponent *pl=nullptr,KeyState pressState=KeyState::Press,float magnitude=1.f,const std::function<bool(ConConf*,float&)> &callback=nullptr);
	virtual ConVar *SetConVar(std::string scmd,std::string value,bool bApplyIfEqual=false) override;

	ConVar *CreateConVar(const std::string &scmd,const std::string &value,ConVarFlags flags,const std::string &help="");
	virtual ConCommand *CreateConCommand(const std::string &scmd,LuaFunction fc,ConVarFlags flags=ConVarFlags::None,const std::string &help="");

	virtual bool IsTCPOpen() const=0;
	virtual bool IsUDPOpen() const=0;

	const util::CPUProfiler::Stage &StartStageProfiling(uint32_t stage);
	std::chrono::nanoseconds EndStageProfiling(uint32_t stage,bool addDuration=false);
};

#endif