#ifndef __C_SHADER_DEPTH_TO_RGB_H__
#define __C_SHADER_DEPTH_TO_RGB_H__

#include "pragma/clientdefinitions.h"
#include <shader/prosper_shader.hpp>

namespace pragma
{
	class DLLCLIENT ShaderDepthToRGB
		: public prosper::ShaderGraphics
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::Shader::DescriptorSetInfo DESCRIPTOR_SET;

#pragma pack(push,1)
		struct PushConstants
		{
			float nearZ;
			float farZ;
		};
#pragma pack(pop)

		ShaderDepthToRGB(prosper::Context &context,const std::string &identifier,const std::string &fsShader);
		ShaderDepthToRGB(prosper::Context &context,const std::string &identifier);
		bool Draw(Anvil::DescriptorSet &descSetDepthTex,float nearZ,float farZ);
	protected:
		template<class TPushConstants>
			bool Draw(Anvil::DescriptorSet &descSetDepthTex,const TPushConstants &pushConstants);
		virtual uint32_t GetPushConstantSize() const;
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	//////////////////////

	class DLLCLIENT ShaderCubeDepthToRGB
		: public ShaderDepthToRGB
	{
	public:
#pragma pack(push,1)
		struct PushConstants
		{
			ShaderDepthToRGB::PushConstants basePushConstants;
			int32_t cubeSide;
		};
#pragma pack(pop)

		ShaderCubeDepthToRGB(prosper::Context &context,const std::string &identifier);
		bool Draw(Anvil::DescriptorSet &descSetDepthTex,float nearZ,float farZ,uint32_t cubeSide);
	protected:
		virtual uint32_t GetPushConstantSize() const override;
	};

	//////////////////////

	class DLLCLIENT ShaderCSMDepthToRGB
		: public ShaderDepthToRGB
	{
	public:
#pragma pack(push,1)
		struct PushConstants
		{
			ShaderDepthToRGB::PushConstants basePushConstants;
			int32_t layer;
		};
#pragma pack(pop)

		ShaderCSMDepthToRGB(prosper::Context &context,const std::string &identifier);
		bool Draw(Anvil::DescriptorSet &descSetDepthTex,float nearZ,float farZ,uint32_t layer);
	protected:
		virtual uint32_t GetPushConstantSize() const override;
	};
};
/*
namespace Shader
{
	class DLLCLIENT DepthToRGB
		: public Screen
	{
	protected:
		DepthToRGB(const std::string &fs);
		virtual void InitializePipelineLayout(const Vulkan::Context &context,std::vector<Vulkan::DescriptorSetLayout> &setLayouts,std::vector<Vulkan::PushConstantRange> &pushConstants) override;
		virtual bool BeginDraw(Vulkan::CommandBufferObject *cmdBuffer,Vulkan::ShaderPipeline *shaderPipeline) override;
	public:
		DepthToRGB();
		bool BeginDraw(Vulkan::CommandBufferObject *cmdBuffer,Vulkan::ShaderPipeline *shaderPipeline,float nearZ,float farZ);
	};
	class DLLCLIENT CubeDepthToRGB
		: public DepthToRGB
	{
	protected:
		virtual void InitializePipelineLayout(const Vulkan::Context &context,std::vector<Vulkan::DescriptorSetLayout> &setLayouts,std::vector<Vulkan::PushConstantRange> &pushConstants) override;
		using DepthToRGB::BeginDraw;
	public:
		CubeDepthToRGB();
		bool BeginDraw(Vulkan::CommandBufferObject *cmdBuffer,Vulkan::ShaderPipeline *shaderPipeline,float nearZ,float farZ,int32_t cubeSide);
	};
	class DLLCLIENT CSMDepthToRGB
		: public DepthToRGB
	{
	protected:
		virtual void InitializePipelineLayout(const Vulkan::Context &context,std::vector<Vulkan::DescriptorSetLayout> &setLayouts,std::vector<Vulkan::PushConstantRange> &pushConstants) override;
	public:
		CSMDepthToRGB();
		bool BeginDraw(Vulkan::CommandBufferObject *cmdBuffer,Vulkan::ShaderPipeline *shaderPipeline,float nearZ,float farZ,int32_t layer);
	};
};
*/
#endif