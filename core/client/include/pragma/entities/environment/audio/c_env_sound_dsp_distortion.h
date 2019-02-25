#ifndef __C_ENV_SOUND_DSP_DISTORTION_H__
#define __C_ENV_SOUND_DSP_DISTORTION_H__
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/environment/audio/c_env_sound_dsp.h"
#include "pragma/entities/environment/audio/env_sound_dsp_distortion.h"

namespace pragma
{
	class DLLCLIENT CSoundDspDistortionComponent final
		: public CBaseSoundDspComponent,
		public BaseEnvSoundDspDistortion
	{
	public:
		CSoundDspDistortionComponent(BaseEntity &ent) : CBaseSoundDspComponent(ent) {}
		virtual void ReceiveData(NetPacket &packet) override;
		virtual void OnEntitySpawn() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	};
};

class DLLCLIENT CEnvSoundDspDistortion
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif