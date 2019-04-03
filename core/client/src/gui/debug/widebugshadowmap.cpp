#include "stdafx_client.h"
#include "pragma/clientstate/clientstate.h"
#include "pragma/gui/debug/widebugshadowmap.hpp"
#include "pragma/rendering/lighting/shadows/c_shadowmap.h"
#include "pragma/rendering/lighting/shadows/c_shadowmapcasc.h"
#include "pragma/gui/widebugdepthtexture.h"
#include "pragma/rendering/c_cubemapside.h"
#include <image/prosper_render_target.hpp>
#include <sharedutils/util_string.h>

extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
extern DLLCLIENT CGame *c_game;

LINK_WGUI_TO_CLASS(WIDebugShadowMap,WIDebugShadowMap);

WIDebugShadowMap::WIDebugShadowMap()
	: WIBase()
{}

void WIDebugShadowMap::SetContrastFactor(float contrastFactor)
{
	m_contrastFactor = contrastFactor;
	for(auto &hEl : m_shadowMapImages)
	{
		auto *el = dynamic_cast<WIDebugDepthTexture*>(hEl.get());
		if(el == nullptr)
			continue;
		el->SetContrastFactor(contrastFactor);
	}
}
float WIDebugShadowMap::GetContrastFactor() const {return m_contrastFactor;}

void WIDebugShadowMap::SetShadowMapSize(uint32_t w,uint32_t h) {m_shadowMapSize = {w,h};}

void WIDebugShadowMap::SetLightSource(pragma::CLightComponent &lightSource)
{
	m_lightHandle = lightSource.GetHandle<pragma::CLightComponent>();
}

void WIDebugShadowMap::Update()
{
	WIBase::Update();
	for(auto &hEl : m_shadowMapImages)
	{
		if(hEl.IsValid() == false)
			continue;
		hEl->Remove();
	}
	if(c_game == nullptr || m_lightHandle.expired())
		return;
	auto &lightSource = *m_lightHandle;
	auto type = LightType::Invalid;
	auto *pLight = lightSource.GetLight(type);
	auto *shadow = lightSource.GetShadowMap();
	if(pLight == nullptr || shadow == nullptr)
		return;
	auto &depthTexture = (type != LightType::Directional) ? shadow->GetDepthTexture() : static_cast<ShadowMapCasc*>(shadow)->GetDepthTexture(pragma::CLightComponent::RenderPass::Dynamic);
	if(depthTexture == nullptr)
		return;
	auto &depthImage = depthTexture->GetImage();
	auto numLayers = depthImage->GetLayerCount();
	auto wLayer = m_shadowMapSize.x;
	auto hLayer = m_shadowMapSize.y;
	prosper::util::BarrierImageLayout barrierImageLayout {
		Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::AccessFlagBits::SHADER_READ_BIT
	};
	auto pRadiusComponent = lightSource.GetEntity().GetComponent<pragma::CRadiusComponent>();
	auto &wgui = WGUI::GetInstance();
	switch(type)
	{
		case LightType::Point:
		{
			for(auto i=decltype(numLayers){0};i<numLayers;++i)
			{
				auto *dt = wgui.Create<WIDebugDepthTexture>(this);
				dt->SetTexture(*depthTexture,barrierImageLayout,barrierImageLayout,i);
				dt->SetSize(wLayer,hLayer);
				if(i == static_cast<uint32_t>(CubeMapSide::Left))
					dt->SetPos(0,hLayer); // Left
				else if(i == static_cast<uint32_t>(CubeMapSide::Right))
					dt->SetPos(wLayer *2,hLayer); // Right
				else if(i == static_cast<uint32_t>(CubeMapSide::Top))
					dt->SetPos(wLayer,0); // Up
				else if(i == static_cast<uint32_t>(CubeMapSide::Bottom))
					dt->SetPos(wLayer,hLayer *2); // Down
				else if(i == static_cast<uint32_t>(CubeMapSide::Front))
					dt->SetPos(wLayer,hLayer); // Front
				else
					dt->SetPos(wLayer *3,hLayer); // Back
				dt->Update(1.f,pRadiusComponent.valid() ? pRadiusComponent->GetRadius() : 0.f);
				dt->SetName("dbg_shadowmap" +std::to_string(i));
				dt->SetContrastFactor(GetContrastFactor());
				m_shadowMapImages.push_back(dt->GetHandle());
			}
			break;
		}
		case LightType::Spot:
		{
			auto *dt = wgui.Create<WIDebugDepthTexture>(this);
			dt->SetTexture(*depthTexture,barrierImageLayout,barrierImageLayout);
			dt->SetSize(wLayer,hLayer);
			dt->Update(1.f,pRadiusComponent.valid() ? pRadiusComponent->GetRadius() : 0.f);
			dt->SetName("dbg_shadowmap" +std::to_string(0));
			dt->SetContrastFactor(GetContrastFactor());
			m_shadowMapImages.push_back(dt->GetHandle());
			break;
		}
		case LightType::Directional:
		{
			wLayer *= 0.5f;
			hLayer *= 0.5f;
			for(auto i=decltype(numLayers){0};i<numLayers;++i)
			{
				auto *dt = wgui.Create<WIDebugDepthTexture>(this);
				dt->SetTexture(*depthTexture,barrierImageLayout,barrierImageLayout,i);
				dt->SetSize(wLayer,hLayer);
				dt->SetPos(i *wLayer,0);
				dt->Update(1.f,pRadiusComponent.valid() ? pRadiusComponent->GetRadius() : 0.f);
				dt->SetName("dbg_shadowmap" +std::to_string(i));
				dt->SetContrastFactor(GetContrastFactor());
				m_shadowMapImages.push_back(dt->GetHandle());
			}
			auto csmMap = static_cast<ShadowMapCasc*>(shadow);
			auto &staticDepthTex = csmMap->GetStaticPendingRenderTarget()->GetTexture();
			if(staticDepthTex != nullptr)
			{
				auto &staticDepthImg = staticDepthTex->GetImage();
				for(auto i=decltype(numLayers){0};i<numLayers;++i)
				{
					auto *dt = wgui.Create<WIDebugDepthTexture>(this);
					dt->SetTexture(*staticDepthTex,barrierImageLayout,barrierImageLayout,i);
					dt->SetSize(wLayer,hLayer);
					dt->SetPos(i *wLayer,hLayer);
					dt->Update(1.f,pRadiusComponent.valid() ? pRadiusComponent->GetRadius() : 0.f);
					dt->SetName("dbg_shadowmap_static" +std::to_string(i));
					dt->SetContrastFactor(GetContrastFactor());
					m_shadowMapImages.push_back(dt->GetHandle());
				}
			}
			hLayer *= 2.0;
			break;
		}
		default:
			break;
	}
	SizeToContents();
	auto w = GetWidth();
	auto h = GetHeight();
	for(auto &hEl : m_shadowMapImages)
	{
		if(hEl.IsValid() == false)
			continue;
		hEl->SetAnchor(
			hEl->GetLeft() /static_cast<float>(w),
			hEl->GetTop() /static_cast<float>(h),
			hEl->GetRight() /static_cast<float>(w),
			hEl->GetBottom() /static_cast<float>(h)
		);
	}
}