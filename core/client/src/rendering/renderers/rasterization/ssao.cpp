/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#include "pragma/rendering/renderers/rasterization_renderer.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao_blur.hpp"
#include "pragma/rendering/occlusion_culling/c_occlusion_octree_impl.hpp"
#include "pragma/rendering/c_ssao.hpp"
#include "pragma/rendering/scene/util_draw_scene_info.hpp"
#include "pragma/game/c_game.h"
#include <prosper_command_buffer.hpp>
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <image/prosper_render_target.hpp>
#include <image/prosper_msaa_texture.hpp>

using namespace pragma::rendering;

extern DLLCLIENT CGame *c_game;

void RasterizationRenderer::RenderSSAO(const util::DrawSceneInfo &drawSceneInfo)
{
	auto &ssaoInfo = GetSSAOInfo();
	auto *shaderSSAO = static_cast<pragma::ShaderSSAO*>(ssaoInfo.GetSSAOShader());
	auto *shaderSSAOBlur = static_cast<pragma::ShaderSSAOBlur*>(ssaoInfo.GetSSAOBlurShader());
	if(IsSSAOEnabled() == false || shaderSSAO == nullptr || shaderSSAOBlur == nullptr)
		return;
	auto &scene = GetScene();
	c_game->StartProfilingStage(CGame::CPUProfilingPhase::SSAO);
	c_game->StartProfilingStage(CGame::GPUProfilingPhase::SSAO);
	// Pre-render depths, positions and normals (Required for SSAO)
	auto *renderInfo  = GetRenderInfo(RenderMode::World);
	auto &drawCmd = drawSceneInfo.commandBuffer;
	if(renderInfo != nullptr)
	{
		// SSAO
		auto &prepass = GetPrepass();
		auto &ssaoImg = ssaoInfo.renderTarget->GetTexture().GetImage();

		auto texNormals = prepass.textureNormals;
		auto bNormalsMultiSampled = texNormals->IsMSAATexture();
		if(bNormalsMultiSampled)
		{
			texNormals = static_cast<prosper::MSAATexture&>(*texNormals).Resolve(
				*drawCmd,prosper::ImageLayout::ColorAttachmentOptimal,prosper::ImageLayout::ColorAttachmentOptimal,
				prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal
			);
		}
		else
			drawCmd->RecordImageBarrier(texNormals->GetImage(),prosper::ImageLayout::ColorAttachmentOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);
		auto texDepth = prepass.textureDepth;
		if(texDepth->IsMSAATexture())
		{
			texDepth = static_cast<prosper::MSAATexture&>(*texDepth).Resolve(
				*drawCmd,prosper::ImageLayout::DepthStencilAttachmentOptimal,prosper::ImageLayout::DepthStencilAttachmentOptimal,
				prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal
			);
		}
		else
			drawCmd->RecordImageBarrier(texDepth->GetImage(),prosper::ImageLayout::DepthStencilAttachmentOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);

		drawCmd->RecordBeginRenderPass(*ssaoInfo.renderTarget);
		auto &renderImage = ssaoInfo.renderTarget->GetTexture().GetImage();
		auto extents = renderImage.GetExtents();

		if(shaderSSAO->BeginDraw(drawCmd) == true)
		{
			shaderSSAO->Draw(scene,*ssaoInfo.descSetGroupPrepass->GetDescriptorSet(),{extents.width,extents.height});
			shaderSSAO->EndDraw();
		}

		drawCmd->RecordEndRenderPass();

		drawCmd->RecordImageBarrier(texNormals->GetImage(),prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::ColorAttachmentOptimal);
		drawCmd->RecordImageBarrier(texDepth->GetImage(),prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::DepthStencilAttachmentOptimal);

		// Blur SSAO
		drawCmd->RecordImageBarrier(ssaoInfo.renderTargetBlur->GetTexture().GetImage(),prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::ColorAttachmentOptimal);
		drawCmd->RecordBeginRenderPass(*ssaoInfo.renderTargetBlur);

		if(shaderSSAOBlur->BeginDraw(drawCmd) == true)
		{
			shaderSSAOBlur->Draw(*ssaoInfo.descSetGroupOcclusion->GetDescriptorSet());
			shaderSSAOBlur->EndDraw();
		}

		drawCmd->RecordEndRenderPass();
		//

		if(bNormalsMultiSampled)
		{
			drawCmd->RecordImageBarrier(texNormals->GetImage(),prosper::ImageLayout::ColorAttachmentOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);
			drawCmd->RecordImageBarrier(texDepth->GetImage(),prosper::ImageLayout::DepthStencilAttachmentOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);
		}

		drawCmd->RecordImageBarrier(ssaoInfo.renderTarget->GetTexture().GetImage(),prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::ColorAttachmentOptimal);
	}
	c_game->StopProfilingStage(CGame::GPUProfilingPhase::SSAO);
	c_game->StopProfilingStage(CGame::CPUProfilingPhase::SSAO);
}
