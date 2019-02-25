#ifndef __C_PARTICLE_MOD_INITIAL_ANIMATION_FRAME_HPP__
#define __C_PARTICLE_MOD_INITIAL_ANIMATION_FRAME_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/particlesystem/c_particlemodifier.h"

class DLLCLIENT CParticleInitializerInitialAnimationFrame
	: public CParticleInitializer
{
private:
	float m_minFrame = 0.f;
	float m_maxFrame = 0.f;
	bool m_bUseFraction = false;
public:
	CParticleInitializerInitialAnimationFrame(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual void Initialize(CParticle &particle) override;
};

#endif