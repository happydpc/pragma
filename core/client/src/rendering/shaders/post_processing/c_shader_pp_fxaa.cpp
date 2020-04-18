#include "stdafx_client.h"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_fxaa.hpp"
#include <prosper_util.hpp>
#include <shader/prosper_shader_copy_image.hpp>

using namespace pragma;

decltype(ShaderPPFXAA::DESCRIPTOR_SET_TEXTURE) ShaderPPFXAA::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding {
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		},
		prosper::Shader::DescriptorSetInfo::Binding { // Pre-tonemapped image (required for correct transparency)
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
ShaderPPFXAA::ShaderPPFXAA(prosper::Context &context,const std::string &identifier)
	: ShaderPPBase(context,identifier,"screen/fxaa/fs_pp_fxaa")
{
	SetBaseShader<prosper::ShaderCopyImage>();
}

void ShaderPPFXAA::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}
bool ShaderPPFXAA::Draw(Anvil::DescriptorSet &descSetTexture,const PushConstants &pushConstants)
{
	return RecordPushConstants(pushConstants) &&
		ShaderPPBase::Draw(descSetTexture);
}
void ShaderPPFXAA::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderPPFXAA>({{{
		Anvil::Format::R8G8B8A8_UNORM,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::AttachmentLoadOp::DONT_CARE,
		Anvil::AttachmentStoreOp::STORE,Anvil::SampleCountFlagBits::_1_BIT,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
	}}},outRenderPass,pipelineIdx);
}
