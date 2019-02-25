#include "stdafx_client.h"
#include "pragma/particlesystem/operators/c_particle_operator_wander.hpp"
#include <mathutil/umath.h>
#include <pragma/math/vector/wvvector3.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include <pragma/math/util_noise.hpp>
#include <algorithm>

extern DLLCENGINE CEngine *c_engine;

REGISTER_PARTICLE_OPERATOR(wander,CParticleOperatorWander);

CParticleOperatorWander::CParticleOperatorWander(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values)
	: CParticleOperatorWorldBase(pSystem,values)
{
	for(auto it=values.begin();it!=values.end();it++)
	{
		auto key = it->first;
		ustring::to_lower(key);
		if(key == "strength")
			m_fStrength = util::to_float(it->second);
		else if(key == "frequency")
			m_fFrequency = util::to_float(it->second);
	}

	m_hashCodes.resize(pSystem.GetMaxParticleCount());

}
void CParticleOperatorWander::Initialize(CParticle &particle)
{
	m_hashCodes.at(particle.GetIndex()) = umath::random(1,std::numeric_limits<int32_t>::max());
}
void CParticleOperatorWander::Simulate(double tDelta)
{
	CParticleOperatorWorldBase::Simulate(tDelta);

	m_dtTime += tDelta *m_fFrequency;
	m_dtStrength = m_fStrength *tDelta *60.f;
}
void CParticleOperatorWander::Simulate(CParticle &particle,double tDelta)
{
	CParticleOperatorWorldBase::Simulate(particle,tDelta);

	// using the system hash gives each particle a consistent unique identity;
	// adding an offset to the time prevents synchronization of the zero points
	// (the noise function is always zero at integers)
	auto pid = m_hashCodes.at(particle.GetIndex());
	auto time = m_dtTime +(pid &255) /256.f;
	particle.SetVelocity(particle.GetVelocity() +Vector3(
		util::noise::get_noise(time,pid) *m_dtStrength,
		util::noise::get_noise(time,pid +1) *m_dtStrength,
		util::noise::get_noise(time,pid +2) *m_dtStrength
	));
}