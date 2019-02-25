#include "stdafx_server.h"
#include "pragma/entities/components/s_shooter_component.hpp"
#include "pragma/entities/components/s_character_component.hpp"
#include "pragma/entities/components/s_player_component.hpp"
#include "pragma/networking/wvserverclient.h"
#include "pragma/lua/s_lentity_handles.hpp"
#include <pragma/entities/components/damageable_component.hpp>
#include <pragma/util/bulletinfo.h>
#include <sharedutils/scope_guard.h>
#include <servermanager/interface/sv_nwm_manager.hpp>
#include <pragma/physics/raytraces.h>

using namespace pragma;

extern DLLSERVER ServerState *server;
extern DLLSERVER SGame *s_game;

Bool SShooterComponent::ReceiveNetEvent(pragma::BasePlayerComponent &pl,pragma::NetEventId eventId,NetPacket &packet)
{
	if(eventId == m_netEvFireBullets)
		ReceiveBulletEvent(packet,&pl);
	else
		return false;
	return true;
}
luabind::object SShooterComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SShooterComponentHandleWrapper>(l);}
void SShooterComponent::FireBullets(const BulletInfo &bulletInfo,const std::function<bool(DamageInfo&,BaseEntity*)> &fCallback,std::vector<TraceResult> &outHitTargets,bool bMaster)
{
	DamageInfo dmg;
	dmg.SetAttacker(bulletInfo.hAttacker.IsValid() ? bulletInfo.hAttacker.get() : &GetEntity());
	dmg.SetInflictor(bulletInfo.hInflictor.IsValid() ? bulletInfo.hInflictor.get() : &GetEntity());
	dmg.SetDamageType(DAMAGETYPE::BULLET);

	auto bCustomForce = (isnan(bulletInfo.force) == false) ? true : false;
	if(bCustomForce == true)
		dmg.SetForce(Vector3(bulletInfo.force,0,0));

	auto bCustomDamageType = (bulletInfo.damageType != util::declvalue(&BulletInfo::damageType)) ? true : false;
	if(bCustomDamageType == true)
		dmg.SetDamageType(bulletInfo.damageType);

	auto bCustomDamage = (bulletInfo.damage != util::declvalue(&BulletInfo::damage)) ? true : false;
	if(bCustomDamage == true)
		dmg.SetDamage(static_cast<uint16_t>(bulletInfo.damage));

	if(!bulletInfo.ammoType.empty())
	{
		auto *ammoType = s_game->GetAmmoType(bulletInfo.ammoType);
		if(ammoType != nullptr)
		{
			if(bCustomDamage == false)
				dmg.SetDamage(static_cast<uint16_t>(ammoType->damage));
			if(bCustomDamageType == false)
				dmg.SetDamageType(ammoType->damageType);
			if(bCustomForce == false)
				dmg.SetForce(Vector3(ammoType->force,0,0)); // Force is stored in x-axis, later converted to actual velocity
		}
	}
	FireBullets(bulletInfo,dmg,outHitTargets,fCallback,bMaster);
}
void SShooterComponent::FireBullets(const BulletInfo &bulletInfo,std::vector<TraceResult> &results,bool bMaster) {FireBullets(bulletInfo,nullptr,results,bMaster);}

void SShooterComponent::FireBullets(const BulletInfo &bulletInfo,DamageInfo &dmgInfo,std::vector<TraceResult> &outHitTargets,const std::function<bool(DamageInfo&,BaseEntity*)> &fCallback,bool bMaster)
{
	pragma::BasePlayerComponent *pl = nullptr;
	if(bMaster == false)
	{
		if(m_nextBullet == nullptr) // No bullet has been scheduled
			return;
		auto *ent = static_cast<SBaseEntity*>(m_nextBullet->source.get());
		if(ent == nullptr || ent->IsPlayer() == false)
			return;
		pl = ent->GetPlayerComponent().get();
	}
	Vector3 origin {};
	Vector3 dir {};
	OnFireBullets(bulletInfo,origin,dir);
	if(bMaster == true)
	{
		m_nextBullet = std::unique_ptr<NextBulletInfo>(new NextBulletInfo);
		m_nextBullet->destinations = GetBulletDestinations(origin,dir,bulletInfo);
	}
	ScopeGuard sg([this]() {
		m_nextBullet = nullptr; // We're done with this
	});

	if(m_nextBullet->destinations.size() != bulletInfo.bulletCount) // These are not the bullets we've been looking for
		return;
	dmgInfo.SetSource(origin);

	std::vector<Vector3> dstPositions;
	dstPositions.reserve(bulletInfo.bulletCount);

	TraceData data;
	GetBulletTraceData(bulletInfo,data);
	outHitTargets.reserve(bulletInfo.bulletCount);
	for(auto i=decltype(bulletInfo.bulletCount){0};i<bulletInfo.bulletCount;++i)
	{
		auto &bulletDst = m_nextBullet->destinations[i];
		auto bulletDir = bulletDst -origin;
		uvec::normalize(&bulletDir);
		// TODO: Add sanity checks to make sure client isn't cheating
		auto dst = origin +bulletDir *bulletInfo.distance;
		data.SetSource(origin);
		data.SetTarget(dst);
		dstPositions.push_back(dst);

		auto result = s_game->RayCast(data);
		outHitTargets.push_back(result);
		if(result.hit && result.entity.IsValid())
		{
			auto pDamageableComponent = result.entity->GetComponent<pragma::DamageableComponent>();
			if(pDamageableComponent.valid())
			{
				auto hitGroup = HitGroup::Generic;
				if(result.collisionObj.IsValid())
				{
					auto charComponent = result.entity.get()->GetCharacterComponent();
					if(charComponent.valid())
						charComponent->FindHitgroup(*result.collisionObj.get(),hitGroup);
				}
				dmgInfo.SetHitGroup(hitGroup);
				dmgInfo.SetForce(bulletDir *dmgInfo.GetForce().x);
				dmgInfo.SetHitPosition(result.position);
				if((fCallback == nullptr || fCallback(dmgInfo,result.entity.get()) == true) && pDamageableComponent.valid())
					pDamageableComponent->TakeDamage(dmgInfo);
			}
		}
	}
	auto &ent = static_cast<SBaseEntity&>(GetEntity());
	if(ent.IsShared() == true)
	{
		auto numBullets = dstPositions.size();
		NetPacket p {};
		p->Write<uint32_t>(static_cast<uint32_t>(numBullets));
		for(auto &hitPos : dstPositions)
			p->Write<Vector3>(hitPos);
		if(bMaster == false)
		{
			auto *session = static_cast<pragma::SPlayerComponent*>(pl)->GetClientSession();
			nwm::RecipientFilter rp {};
			rp.SetFilterType(nwm::RecipientFilter::Type::Exclude);
			if(session != nullptr)
				rp.Add(session);
			ent.SendNetEventUDP(m_netEvFireBullets,p,rp);
		}
		else
			ent.SendNetEventUDP(m_netEvFireBullets,p);
	}

	CEOnBulletsFired evData {bulletInfo,outHitTargets};
	BroadcastEvent(EVENT_ON_BULLETS_FIRED,evData);
}