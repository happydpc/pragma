#ifndef __RAYTRACING_CYCLES_HPP__
#define __RAYTRACING_CYCLES_HPP__

#include "pragma/clientdefinitions.h"
#include <pragma/entities/environment/env_camera.h>
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/util_parallel_job.hpp>
#include <sharedutils/functioncallback.h>
#include <memory>
#include <functional>

class ClientState;
class BaseEntity;
namespace util {class ImageBuffer;};
namespace pragma::rendering::cycles
{
	struct DLLCLIENT SceneInfo
	{
		uint32_t width = 1'024;
		uint32_t height = 768;
		uint32_t samples = 1'024;
		bool denoise = true;
		bool hdrOutput = false;
	};
	struct DLLCLIENT RenderImageInfo
	{
		Vector3 cameraPosition = {};
		Quat cameraRotation = {};
		float nearZ = pragma::BaseEnvCameraComponent::DEFAULT_NEAR_Z;
		float farZ = pragma::BaseEnvCameraComponent::DEFAULT_FAR_Z;
		umath::Degree fov = BaseEnvCameraComponent::DEFAULT_FOV;

		std::function<bool(BaseEntity&)> entityFilter = nullptr;
	};
	util::ParallelJob<std::shared_ptr<util::ImageBuffer>> render_image(ClientState &client,const SceneInfo &sceneInfo,const RenderImageInfo &renderImageInfo);
	util::ParallelJob<std::shared_ptr<util::ImageBuffer>> bake_ambient_occlusion(ClientState &client,const SceneInfo &sceneInfo,Model &mdl,uint32_t materialIndex);
	util::ParallelJob<std::shared_ptr<util::ImageBuffer>> bake_lightmaps(ClientState &client,const SceneInfo &sceneInfo,BaseEntity &entTarget);
};

#endif
