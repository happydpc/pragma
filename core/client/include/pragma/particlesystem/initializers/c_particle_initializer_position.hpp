#ifndef __C_PARTICLE_INITIALIZER_POSITION_HPP__
#define __C_PARTICLE_INITIALIZER_POSITION_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/particlesystem/c_particlemodifier.h"
#include "pragma/particlesystem/modifiers/c_particle_modifier_component_random_variable.hpp"

class DLLCLIENT CParticleInitializerPositionRandomBox
	: public CParticleInitializer
{
public:
	CParticleInitializerPositionRandomBox(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual void Initialize(CParticle &particle) override;
private:
	Vector3 m_min = {};
	Vector3 m_max = {};
	Vector3 m_origin = {};
	bool m_bOnSides = false;
};

//////////////////////////////

class DLLCLIENT CParticleInitializerPositionRandomSphere
	: public CParticleInitializer
{
public:
	CParticleInitializerPositionRandomSphere(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual void Initialize(CParticle &particle) override;
private:
	float m_distMin = 0.f;
	float m_distMax = 0.f;
	Vector3 distBias = {};
	Vector3 m_origin = {};
};

//////////////////////////////

class DLLCLIENT CParticleInitializerPositionRandomCircle
	: public CParticleInitializer
{
public:
	CParticleInitializerPositionRandomCircle(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values);
	virtual void Initialize(CParticle &particle) override;
private:
	Vector3 m_vAxis = Vector3(0.f,1.f,0.f);
	float m_fMinDist = 0.f;
	float m_fMaxDist = 0.f;
	Vector3 m_origin = {};
};

#endif