#include "stdafx_client.h"
#include "pragma/clientstate/clientstate.h"
#include "pragma/game/c_game.h"
#include "pragma/particlesystem/operators/c_particle_mod_operator_texture_scrolling.hpp"
#include <mathutil/umath.h>
#include <pragma/math/vector/wvvector3.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include <pragma/model/modelmesh.h>
#include <algorithm>

REGISTER_PARTICLE_OPERATOR(texture_scrolling,CParticleOperatorTextureScrolling);

CParticleOperatorTextureScrolling::CParticleOperatorTextureScrolling(pragma::CParticleSystemComponent &pSystem,const std::unordered_map<std::string,std::string> &values)
	: CParticleOperator(pSystem,values)
{
	for(auto &pair : values)
	{
		auto key = pair.first;
		ustring::to_lower(key);
		if(key == "horizontal_speed")
			m_fHorizontalSpeed = util::to_float(pair.second);
		else if(key == "vertical_speed")
			m_fVerticalSpeed = util::to_float(pair.second);
	}
	pSystem.SetTextureScrollingEnabled(true);
}
void CParticleOperatorTextureScrolling::SetFrameOffset(CParticle &particle,Vector2 uv)
{
	umath::normalize_uv_coordinates(uv);

	auto frameOffset = glm::packHalf2x16(uv);
	particle.SetFrameOffset(reinterpret_cast<float&>(frameOffset));
}
void CParticleOperatorTextureScrolling::Initialize(CParticle &particle) {SetFrameOffset(particle,{});}
void CParticleOperatorTextureScrolling::Simulate(CParticle &particle,double dt)
{
	auto offsetH = m_fHorizontalSpeed *dt;
	auto offsetV = m_fVerticalSpeed *dt;

	// Horizontal and vertical scroll offsets are encoded as 16 bit floating point values
	// as the particle's frame offset. Particles cannot be animated and have texture scrolling
	// at the same time!
	auto frameOffset = particle.GetFrameOffset();
	auto uv = glm::unpackHalf2x16(reinterpret_cast<uint32_t&>(frameOffset));
	uv.x += offsetH;
	uv.y += offsetV;
	umath::normalize_uv_coordinates(uv);

	SetFrameOffset(particle,uv);
}
