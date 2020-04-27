/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __C_SHADER_CUBEMAP_TO_EQUIRECTANGULAR_HPP__
#define __C_SHADER_CUBEMAP_TO_EQUIRECTANGULAR_HPP__

#include "pragma/rendering/shaders/c_shader_base_cubemap.hpp"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace prosper {class Texture;};
namespace pragma
{
	class DLLCLIENT ShaderCubemapToEquirectangular
		: public prosper::ShaderBaseImageProcessing
	{
	public:
		enum class Pipeline : uint32_t
		{
			RGBA16 = 0,
			RGBA8,

			Count
		};

		ShaderCubemapToEquirectangular(prosper::Context &context,const std::string &identifier);
		std::shared_ptr<prosper::Texture> CubemapToEquirectangularTexture(prosper::Texture &cubemap,uint32_t width=1'600,uint32_t height=800);
	protected:
		std::shared_ptr<prosper::IImage> CreateEquirectangularMap(uint32_t width,uint32_t height,prosper::util::ImageCreateInfo::Flags flags) const;
		std::shared_ptr<prosper::RenderTarget> CreateEquirectangularRenderTarget(uint32_t width,uint32_t height,prosper::util::ImageCreateInfo::Flags flags) const;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};
};

#endif
