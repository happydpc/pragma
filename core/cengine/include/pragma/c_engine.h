#ifndef __C_ENGINE_H__
#define __C_ENGINE_H__

#include "pragma/c_enginedefinitions.h"
#include <pragma/engine.h>
#include "pragma/launchparameters.h"
#include "pragma/rendering/c_render_context.hpp"
#include "pragma/input/c_keybind.h"
#include <unordered_map>

enum class GPUTimerEvent : uint32_t;
class CSciGPUTimerManager;
class ClientState;
namespace GLFW {class Joystick;};
namespace al {class SoundSystem;class Effect;};
namespace prosper {class RenderTarget;};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLCENGINE CEngine
	: public Engine,public pragma::RenderContext
{
public:
	CEngine(int argc,char* argv[]);
	virtual ~CEngine() override;
	virtual void Release() override;
	static const unsigned int MAX_STEREO_SOURCES = 64;
	// Threshold at which axis value represents a key press
	static const float AXIS_PRESS_THRESHOLD;
private:
	struct DLLCENGINE ConVarInfo
	{
		std::string cvar;
		std::vector<std::string> argv;
	};
	struct DLLCENGINE ConVarInfoList
	{
		std::vector<ConVarInfo> cvars;
		ConVarInfo *find(const std::string &cmd);
	};
	// Sound
	std::shared_ptr<al::SoundSystem> m_soundSystem = nullptr;

	// FPS
	UInt32 m_fps;
	Float m_tFPSTime;
	std::chrono::high_resolution_clock::time_point m_tLastFrame;
	std::chrono::high_resolution_clock::duration m_tDeltaFrameTime;

	std::unordered_map<std::string,std::shared_ptr<al::Effect>> m_auxEffects;

	bool m_bControllersEnabled = false;
	float m_speedCam;
	float m_speedCamMouse;
	float m_nearZ,m_farZ;
	bool m_bFullbright;
	bool m_bWindowFocused;
	std::unique_ptr<StateInstance> m_clInstance;
	std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
	bool m_bFirstFrame = true;
	bool m_bUniformBlocksInitialized;
	bool m_bVulkanValidationEnabled = false;
	mutable std::unique_ptr<CSciGPUTimerManager> m_gpuTimerManager;

	float m_rawInputJoystickMagnitude = 0.f;
	std::unordered_map<GLFW::Key,GLFW::KeyState> m_joystickKeyStates;
	std::unordered_map<short,KeyBind> m_keyMappings;
	std::unique_ptr<ConVarInfoList> m_preloadedConfig;

	virtual void Think() override;
	virtual void Tick() override;
	void Input(int key,GLFW::KeyState state,GLFW::Modifier mods={},float magnitude=1.f);
	void Input(int key,GLFW::KeyState inputState,GLFW::KeyState pressState,GLFW::Modifier mods,float magnitude=1.f);
	void UpdateFPS(float t);
	void MapKey(short c,std::unordered_map<std::string,std::vector<std::string>> &binds);
protected:
	void DrawScene(std::shared_ptr<prosper::PrimaryCommandBuffer> &drawCmd,std::shared_ptr<prosper::RenderTarget> &rt);
	void WriteClientConfig(VFilePtrReal f);
	void PreloadClientConfig();
	virtual void OnWindowInitialized() override;
	virtual void LoadConfig() override;
	virtual void InitializeExternalArchiveManager() override;
	virtual void DrawFrame(prosper::PrimaryCommandBuffer &drawCmd,uint32_t n_current_swapchain_image) override;
	void InitializeStagingTarget();
public:
	using pragma::RenderContext::DrawFrame;
	virtual void DumpDebugInformation(ZIPFile &zip) const override;
	virtual bool Initialize(int argc,char *argv[]) override;
	StateInstance &GetClientStateInstance();
	virtual void OnResolutionChanged(uint32_t width,uint32_t height) override;
	virtual void Start() override;
	virtual void Close() override;
	void UseFullbrightShader(bool b);
	uint32_t GetFPSLimit() const;
	virtual void EndGame() override;
	virtual bool IsClientConnected() override;
	bool IsWindowFocused() const;
	virtual void LoadMap(const char *map) override;
	bool IsValidAxisInput(float axisInput) const;
	void GetMappedKeys(const std::string &cmd,std::vector<GLFW::Key> &keys,uint32_t maxKeys=1);
	// Returns true if the input is a valid button input state (pressed or released)
	// If the input is an axis input, inOutState may change to represent actual button state
	bool GetInputButtonState(float axisInput,GLFW::Modifier mods,GLFW::KeyState &inOutState) const;

	void SetVulkanValidationLayersEnabled(bool b);

	// Debug
	void StartGPUTimer(GPUTimerEvent ev) const;
	void StopGPUTimer(GPUTimerEvent ev) const;
	bool GetGPUTimerResult(GPUTimerEvent ev,float &r) const;
	float GetGPUTimerResult(GPUTimerEvent ev) const;
	CSciGPUTimerManager &GetGPUTimerManager() const;

	// Config
	void LoadClientConfig();
	void SaveClientConfig();
	bool ExecConfig(const std::string &cfg,std::vector<ConVarInfo> &cmds);
	void ExecCommands(ConVarInfoList &cmds);
	void SetControllersEnabled(bool b);
	bool GetControllersEnabled() const;

	// Sound
	const al::SoundSystem *GetSoundSystem() const;
	al::SoundSystem *GetSoundSystem();
	al::SoundSystem *InitializeSoundEngine();
	void CloseSoundEngine();
	void SetHRTFEnabled(bool b);
	unsigned int GetStereoSourceCount();
	unsigned int GetMonoSourceCount();
	unsigned int GetStereoSource(unsigned int idx);
	template<class TEfxProperties>
		std::shared_ptr<al::Effect> CreateAuxEffect(const std::string &name,const TEfxProperties &props);
	std::shared_ptr<al::Effect> GetAuxEffect(const std::string &name);
	// Lua
	virtual NetworkState *GetNetworkState(lua_State *l) override;
	virtual Lua::Interface *GetLuaInterface(lua_State *l) override;

	float GetNearZ();
	float GetFarZ();
	// Input
	void MouseInput(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	void KeyboardInput(GLFW::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods,float magnitude=1.f);
	void CharInput(GLFW::Window &window,unsigned int c);
	void ScrollInput(GLFW::Window &window,Vector2 offset);
	void OnWindowFocusChanged(GLFW::Window &window,bool bFocus);
	void OnFilesDropped(GLFW::Window &window,std::vector<std::string> &files);
	void JoystickButtonInput(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state);
	void JoystickAxisInput(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t axis,GLFW::Modifier mods,float newVal,float deltaVal);
	float GetRawJoystickAxisMagnitude() const;
	// Util
	virtual bool IsServerOnly() override;
	// Convars
	virtual ConConf *GetConVar(const std::string &cv) override;
	virtual bool RunConsoleCommand(std::string cmd,std::vector<std::string> &argv,KeyState pressState=KeyState::Press,float magnitude=1.f,const std::function<bool(ConConf*,float&)> &callback=nullptr) override;
	// ClientState
	virtual NetworkState *GetClientState() const override;
	ClientState *OpenClientState();
	void CloseClientState();
	void Connect(const std::string &ip,const std::string &port="29150");
	void Disconnect();
	virtual void HandleLocalPlayerClientPacket(NetPacket &p) override;
	// KeyMappings
	void MapKey(short c,std::string cmd);
	void MapKey(short c,int function);
	void AddKeyMapping(short c,std::string cmd);
	void RemoveKeyMapping(short c,std::string cmd);
	void ClearLuaKeyMappings();
	void UnmapKey(short c);
	const std::unordered_map<short,KeyBind> &GetKeyMappings() const;
	void ClearKeyMappings();

	// Shaders
	::util::WeakHandle<prosper::Shader> ReloadShader(const std::string &name);
	void ReloadShaderPipelines();
	//

	Double GetDeltaFrameTime() const;
	UInt32 GetFPS() const;
	UInt32 GetFrameTime() const;
};

template<class TEfxProperties>
	std::shared_ptr<al::Effect> CEngine::CreateAuxEffect(const std::string &name,const TEfxProperties &props)
{
	auto lname = name;
	ustring::to_lower(lname);
	auto effect = GetAuxEffect(lname);
	if(effect != nullptr)
		return effect;
	auto *soundSys = GetSoundSystem();
	if(soundSys == nullptr)
		return nullptr;
	try
	{
		effect = soundSys->CreateEffect(props);
	}
	catch(const std::runtime_error &e)
	{
		Con::cwar<<"WARNING: Unable to create auxiliary effect '"<<name<<"': "<<e.what()<<Con::endl;
		return nullptr;
	}
	m_auxEffects.insert(decltype(m_auxEffects)::value_type(name,effect));
	return effect;
}
#pragma warning(pop)

#endif