#include "stdafx_server.h"
#include "pragma/game/s_game.h"
#include "pragma/entities/components/s_player_component.hpp"
#include "pragma/networking/wvserverclient.h"
#include "pragma/entities/player.h"
#include <pragma/entities/baseplayer.hpp>
#include <pragma/networking/snapshot_flags.hpp>
#include <pragma/entities/components/velocity_component.hpp>
#include <pragma/entities/components/base_transform_component.hpp>
#include <pragma/entities/components/base_physics_component.hpp>
#include <pragma/entities/components/base_character_component.hpp>

extern DLLSERVER ServerState *server;

void SGame::SendSnapshot(pragma::SPlayerComponent *pl)
{
	if(pl == nullptr)
		return;
	auto *session = pl->GetClientSession();
	NetPacket packet;
	packet->Write<uint8_t>(session->SwapSnapshotId());
	packet->Write<double>(CurTime());

	std::vector<SBaseEntity*> *entities;
	GetEntities(&entities);
	auto numEntities = entities->size();
	auto posNumEnts = packet->GetSize();
	packet->Write<unsigned int>((unsigned int)(0));
	size_t numEntitiesValid = 0;
	for(size_t i=0;i<numEntities;i++)
	{
		SBaseEntity *ent = (*entities)[i];
		if(ent != NULL && ent->IsShared() && ent->IsSynchronized() && ent->IsMarkedForSnapshot())
		{
			numEntitiesValid++;
			auto pTrComponent = ent->GetTransformComponent();
			auto pVelComponent = ent->GetComponent<pragma::VelocityComponent>();
			nwm::write_entity(packet,ent);
			nwm::write_vector(packet,pTrComponent.valid() ? pTrComponent->GetPosition() : Vector3{});
			nwm::write_vector(packet,pVelComponent.valid() ? pVelComponent->GetVelocity() : Vector3{});
			nwm::write_vector(packet,pVelComponent.valid() ? pVelComponent->GetAngularVelocity() : Vector3{});
			nwm::write_quat(packet,pTrComponent.valid() ? pTrComponent->GetOrientation() : uquat::identity());

			auto offsetEntData = packet->GetSize();
			packet->Write<UInt8>(UInt8(0));
			auto offset = packet->GetSize();
			ent->SendSnapshotData(packet,*pl);
			auto entDataSize = packet->GetSize() -offset;
#ifdef _DEBUG
			assert(entDataSize <= std::numeric_limits<UInt8>::max());
#endif
			packet->Write<UInt8>(CUInt8(entDataSize),&offsetEntData);

			auto flags = pragma::SnapshotFlags::None;
			auto offsetSnapshotFlags = packet->GetOffset();
			packet->Write<decltype(flags)>(flags);

			auto pPhysComponent = ent->GetPhysicsComponent();
			PhysObj *physObj = pPhysComponent.valid() ? pPhysComponent->GetPhysicsObject() : nullptr;
			if(physObj != NULL && !physObj->IsStatic())
			{
				flags |= pragma::SnapshotFlags::PhysicsData;
#ifdef PHYS_ENGINE_BULLET
				if(physObj->IsController())
				{
					packet->Write<uint8_t>(1u);
					auto *physController = static_cast<ControllerPhysObj*>(physObj);
					packet->Write<Vector3>(physController->GetPosition());
					packet->Write<Quat>(physController->GetOrientation());
					packet->Write<Vector3>(physController->GetLinearVelocity());
					packet->Write<Vector3>(physController->GetAngularVelocity());
				}
				else
				{
					auto colObjs = physObj->GetCollisionObjects();
					packet->Write<uint8_t>(static_cast<uint8_t>(colObjs.size()));
					//auto i = 0;
					for(auto &hObj : colObjs)
					{
						Vector3 pos {0.f,0.f,0.f};
						auto rot = uquat::identity();
						Vector3 vel {0.f,0.f,0.f};
						Vector3 angVel {0.f,0.f,0.f};
						if(hObj.IsValid())
						{
							auto *o = static_cast<PhysCollisionObject*>(hObj.get());
							pos = o->GetPos();
							rot = o->GetRotation();
							if(o->IsRigid())
							{
								auto *rigid = static_cast<PhysRigidBody*>(o);
								vel = rigid->GetLinearVelocity();
								angVel = rigid->GetAngularVelocity();
							}
						}
						packet->Write<Vector3>(pos);
						packet->Write<Quat>(rot);
						packet->Write<Vector3>(vel);
						packet->Write<Vector3>(angVel);
					}
				}
#elif PHYS_ENGINE_PHYSX
				std::vector<DynamicActorInfo> &actors = phys->GetActorInfo();
				packet.Write<unsigned char>(actors.size());
				for(unsigned char j=0;j<actors.size();j++)
				{
					physx::PxRigidDynamic *actor = actors[j].GetActor();
					physx::PxTransform t = actor->getGlobalPose();
					physx::PxVec3 vel = actor->getLinearVelocity();
					physx::PxVec3 angVel = actor->getAngularVelocity();
					for(char i=0;i<3;i++)
						packet->Write<float>(t.p[i]);
					for(char i=0;i<3;i++)
						packet->Write<float>(vel[i]);
					for(char i=0;i<3;i++)
						packet->Write<float>(angVel[i]);
					packet->Write<float>(t.q.w);
					packet->Write<float>(t.q.x);
					packet->Write<float>(t.q.y);
					packet->Write<float>(t.q.z);
				}
#endif
			}

			auto offsetNumComponents = 0u;
			auto numComponents = 0u;
			auto bFirst = true;
			for(auto &pComponent : ent->GetComponents())
			{
				if(pComponent->ShouldTransmitSnapshotData() == false)
					continue;
				auto *pSnapshotComponent = dynamic_cast<pragma::SBaseSnapshotComponent*>(pComponent.get());
				if(pSnapshotComponent == nullptr)
					throw std::logic_error("Component must be derived from SBaseSnapshotComponent if snapshot data is enabled!");
				if(bFirst)
				{
					bFirst = false;
					flags |= pragma::SnapshotFlags::ComponentData;
					offsetNumComponents = packet->GetOffset();
					packet->Write<uint8_t>(static_cast<uint8_t>(0u));
				}
				packet->Write<pragma::ComponentId>(pComponent->GetComponentId());
				auto offsetComponentSize = packet->GetOffset();
				packet->Write<uint8_t>(static_cast<uint8_t>(0u));

				auto offsetComponentDataStart = packet->GetOffset();
				pSnapshotComponent->SendSnapshotData(packet,*pl);
				auto szComponent = packet->GetOffset() -offsetComponentDataStart;
				if(szComponent > std::numeric_limits<uint8_t>::max())
					throw std::runtime_error("Component size mustn't exceed " +std::to_string(std::numeric_limits<uint8_t>::max()) +" bytes!");
				packet->Write<uint8_t>(szComponent,&offsetComponentSize);

				if(++numComponents == std::numeric_limits<uint8_t>::max())
				{
					Con::csv<<"WARNING: Attempted to send data for more than "<<std::numeric_limits<uint8_t>::max()<<" components for a single entity! This is not allowed!"<<Con::endl;
					break;
				}
			}
			packet->Write<decltype(flags)>(flags,&offsetSnapshotFlags);
			if((flags &pragma::SnapshotFlags::ComponentData) != pragma::SnapshotFlags::None)
				packet->Write<uint8_t>(numComponents,&offsetNumComponents);
		}
	}
	packet->Write<UInt32>(CUInt32(numEntitiesValid),&posNumEnts);

	//unsigned int numPlayers = Player::GetPlayerCount();
	auto posNumPls = packet->GetSize();
	packet->Write<unsigned char>((unsigned char)(0));
	auto &players = pragma::SPlayerComponent::GetAll();
	unsigned char numPlayersValid = 0;
	for(auto *plComponent : players)
	{
		if(plComponent != NULL && plComponent != pl)
		{
			auto *ent = static_cast<Player*>(plComponent->GetBasePlayer());
			if(ent != nullptr)
			{
				numPlayersValid++;
				nwm::write_player(packet,plComponent);
				auto charComponent = ent->GetCharacterComponent();
				nwm::write_quat(packet,charComponent.valid() ? charComponent->GetViewOrientation() : uquat::identity());
				std::vector<InputAction> &keyStack = pl->GetKeyStack();
				auto sz = CUChar(keyStack.size());
				packet->Write<UChar>(sz);
				for(UChar k=0;k<sz;k++) // TODO: Same as above
				{
					InputAction &ka = keyStack[k];
					packet->Write<unsigned short>(CUInt16(ka.action));
					packet->Write<char>(ka.task == GLFW_PRESS);
				}
			}
		}
	}
	packet->Write<unsigned char>(numPlayersValid,&posNumPls);
	server->SendPacketUDP("snapshot",packet,pl->GetClientSession());
}

void SGame::SendSnapshot()
{
	//Con::csv<<"[SERVER] Sending snapshot.."<<Con::endl;
	auto &players = pragma::SPlayerComponent::GetAll();
	//unsigned char numPlayersValid = 0;
	for(auto *plComponent : players)
	{
		if(plComponent != nullptr && plComponent->IsGameReady())
			SendSnapshot(plComponent);
	}
	std::vector<SBaseEntity*> *entities;
	GetEntities(&entities);
	for(unsigned int i=0;i<entities->size();i++)
	{
		SBaseEntity *ent = (*entities)[i];
		if(ent != NULL && ent->IsMarkedForSnapshot())
			ent->MarkForSnapshot(false);
	}
	for(auto *plComponent : players)
	{
		if(plComponent != NULL)
			plComponent->ClearKeyStack();
	}
}
