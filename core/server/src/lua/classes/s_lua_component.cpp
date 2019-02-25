#include "stdafx_server.h"
#include "pragma/lua/s_lua_component.hpp"
#include <servermanager/interface/sv_nwm_manager.hpp>
#include <pragma/entities/components/base_player_component.hpp>

using namespace pragma;

SLuaBaseEntityComponent::SLuaBaseEntityComponent(BaseEntity &ent,luabind::object &o)
	: BaseLuaBaseEntityComponent(ent,o),SBaseSnapshotComponent()
{}
void SLuaBaseEntityComponent::OnMemberValueChanged(uint32_t memberIdx)
{
	BaseLuaBaseEntityComponent::OnMemberValueChanged(memberIdx);
	if(m_networkedMemberInfo == nullptr)
		return;
	auto &members = GetMembers();
	if(memberIdx >= members.size())
		return;
	auto &member = members.at(memberIdx);
	if((member.flags &MemberFlags::TransmitOnChange) != MemberFlags::TransmitOnChange)
	{
		if((member.flags &MemberFlags::SnapshotData) == MemberFlags::SnapshotData)
			static_cast<SBaseEntity&>(GetEntity()).MarkForSnapshot(true);
		return;
	}
	auto itNwIndex = m_networkedMemberInfo->memberIndexToNetworkedIndex.find(memberIdx);
	if(itNwIndex == m_networkedMemberInfo->memberIndexToNetworkedIndex.end())
	{
		// This should be unreachable!
		throw std::logic_error("Invalid networked variable '" +member.name +"'!");
		return;
	}
	auto nwIndex = itNwIndex->second;
	const auto maxNwVars = std::numeric_limits<uint8_t>::max();
	if(nwIndex > maxNwVars)
	{
		Con::cwar<<"WARNING: Networked member index of '"<<member.name<<"' exceeds maximum allowed number of networked variables ("<<maxNwVars<<")!"<<Con::endl;
		return;
	}
	auto value = GetMemberValue(member);
	NetPacket p {};
	p->Write<uint8_t>(nwIndex);
	Lua::WriteAny(p,member.type,value);
	static_cast<SBaseEntity&>(GetEntity()).SendNetEventTCP(m_networkedMemberInfo->netEvSetMember,p);
}
void SLuaBaseEntityComponent::SendData(NetPacket &packet,nwm::RecipientFilter &rp)
{
	if(m_networkedMemberInfo != nullptr)
	{
		auto &members = GetMembers();
		for(auto idx : m_networkedMemberInfo->networkedMembers)
		{
			auto &member = members.at(idx);
			auto value = GetMemberValue(member);
			Lua::WriteAny(packet,member.type,value);
		}
	}

	CallLuaMember<void,NetPacket,nwm::RecipientFilter>("SendData",packet,rp);
}
Bool SLuaBaseEntityComponent::ReceiveNetEvent(pragma::BasePlayerComponent &pl,pragma::NetEventId evId,NetPacket &packet)
{
	auto it = m_boundNetEvents.find(evId);
	if(it != m_boundNetEvents.end())
	{
		it->second.Call<void,std::reference_wrapper<NetPacket>,pragma::BasePlayerComponent*>(std::reference_wrapper<NetPacket>{packet},&pl);
		return true;
	}

	auto handled = static_cast<uint32_t>(util::EventReply::Unhandled);
	CallLuaMember<uint32_t,luabind::object,uint32_t,NetPacket>("ReceiveNetEvent",&handled,pl.GetLuaObject(),evId,packet);
	return static_cast<util::EventReply>(handled) == util::EventReply::Handled;
}
void SLuaBaseEntityComponent::SendSnapshotData(NetPacket &packet,pragma::BasePlayerComponent &pl)
{
	auto &members = GetMembers();
	if(m_networkedMemberInfo != nullptr)
	{
		for(auto idx : m_networkedMemberInfo->snapshotMembers)
		{
			auto &member = members.at(idx);
			auto value = GetMemberValue(member);
			Lua::WriteAny(packet,member.type,value);
		}
	}
	CallLuaMember<void,NetPacket,luabind::object>("SendSnapshotData",packet,pl.GetLuaObject());
}
bool SLuaBaseEntityComponent::ShouldTransmitNetData() const {return IsNetworked();}
bool SLuaBaseEntityComponent::ShouldTransmitSnapshotData() const {return BaseLuaBaseEntityComponent::ShouldTransmitSnapshotData();}
void SLuaBaseEntityComponent::InvokeNetEventHandle(const std::string &methodName,NetPacket &packet,pragma::BasePlayerComponent *pl)
{
	CallLuaMember<void,luabind::object,NetPacket>(methodName,pl->GetLuaObject(),packet);
}