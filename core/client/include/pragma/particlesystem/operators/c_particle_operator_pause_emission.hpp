#ifndef __C_PARTICLE_OPERATOR_PAUSE_EMISSION_HPP__
#define __C_PARTICLE_OPERATOR_PAUSE_EMISSION_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/particlesystem/c_particlemodifier.h"
#include "pragma/particlesystem/modifiers/c_particle_modifier_component_time.hpp"

class DLLCLIENT CParticleOperatorPauseEmissionBase
	: public CParticleOperator
{
public:
	virtual void Simulate(double tDelta) override;
	virtual void Initialize() override;
protected:
	CParticleOperatorPauseEmissionBase(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual pragma::CParticleSystemComponent *GetTargetParticleSystem()=0;
private:
	enum class State : uint32_t
	{
		Initial = 0u,
		Paused,
		Unpaused
	};
	float m_fStart = 0.f;
	float m_fEnd = 0.f;
	State m_state = State::Initial;
};

/////////////////////

class DLLCLIENT CParticleOperatorPauseEmission
	: public CParticleOperatorPauseEmissionBase
{
public:
	CParticleOperatorPauseEmission(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual pragma::CParticleSystemComponent *GetTargetParticleSystem() override;
};

/////////////////////

class DLLCLIENT CParticleOperatorPauseChildEmission
	: public CParticleOperatorPauseEmissionBase
{
public:
	CParticleOperatorPauseChildEmission(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual pragma::CParticleSystemComponent *GetTargetParticleSystem() override;
private:
	util::WeakHandle<pragma::CParticleSystemComponent> m_hChildSystem = {};
};

#endif