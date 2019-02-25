#include "stdafx_shared.h"
#include "pragma/entities/components/base_animated_component.hpp"
#include "pragma/entities/components/base_model_component.hpp"
#include "pragma/entities/components/base_time_scale_component.hpp"
#include "pragma/entities/components/base_physics_component.hpp"
#include "pragma/entities/components/base_sound_emitter_component.hpp"
#include "pragma/entities/components/logic_component.hpp"
#include "pragma/model/model.h"
#include "pragma/audio/alsound_type.h"
#include <sharedutils/datastream.h>

#define DEBUG_VERBOSE_ANIMATION 0

using namespace pragma;

ComponentEventId BaseAnimatedComponent::EVENT_HANDLE_ANIMATION_EVENT = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_PLAY_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_PLAY_LAYERED_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_PLAY_LAYERED_ACTIVITY = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_ANIMATION_COMPLETE = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_LAYERED_ANIMATION_START = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_LAYERED_ANIMATION_COMPLETE = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_ANIMATION_START = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_TRANSLATE_LAYERED_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_TRANSLATE_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_TRANSLATE_ACTIVITY = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_MAINTAIN_ANIMATIONS = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_MAINTAIN_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_MAINTAIN_ANIMATION_MOVEMENT = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_SHOULD_UPDATE_BONES = pragma::INVALID_COMPONENT_ID;

ComponentEventId BaseAnimatedComponent::EVENT_ON_PLAY_ACTIVITY = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_STOP_LAYERED_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_BONE_TRANSFORM_CHANGED = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_ANIMATIONS_UPDATED = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_ON_BLEND_ANIMATION = pragma::INVALID_COMPONENT_ID;
ComponentEventId BaseAnimatedComponent::EVENT_PLAY_ANIMATION = pragma::INVALID_COMPONENT_ID;
void BaseAnimatedComponent::RegisterEvents(pragma::EntityComponentManager &componentManager)
{
	auto componentType = std::type_index(typeid(BaseAnimatedComponent));
	EVENT_HANDLE_ANIMATION_EVENT = componentManager.RegisterEvent("HANDLE_ANIMATION_EVENT",componentType);
	EVENT_ON_PLAY_ANIMATION = componentManager.RegisterEvent("ON_PLAY_ANIMATION",componentType);
	EVENT_ON_PLAY_LAYERED_ANIMATION = componentManager.RegisterEvent("ON_PLAY_LAYERED_ANIMATION",componentType);
	EVENT_ON_PLAY_LAYERED_ACTIVITY = componentManager.RegisterEvent("ON_PLAY_LAYERED_ACTIVITY",componentType);
	EVENT_ON_ANIMATION_COMPLETE = componentManager.RegisterEvent("ON_ANIMATION_COMPLETE",componentType);
	EVENT_ON_LAYERED_ANIMATION_START = componentManager.RegisterEvent("ON_LAYERED_ANIMATION_START",componentType);
	EVENT_ON_LAYERED_ANIMATION_COMPLETE = componentManager.RegisterEvent("ON_LAYERED_ANIMATION_COMPLETE",componentType);
	EVENT_ON_ANIMATION_START = componentManager.RegisterEvent("ON_ANIMATION_START",componentType);
	EVENT_TRANSLATE_LAYERED_ANIMATION = componentManager.RegisterEvent("TRANSLATE_LAYERED_ANIMATION",componentType);
	EVENT_TRANSLATE_ANIMATION = componentManager.RegisterEvent("TRANSLATE_ANIMATION",componentType);
	EVENT_TRANSLATE_ACTIVITY = componentManager.RegisterEvent("TRANSLATE_ACTIVITY",componentType);
	EVENT_MAINTAIN_ANIMATIONS = componentManager.RegisterEvent("MAINTAIN_ANIMATIONS",componentType);
	EVENT_MAINTAIN_ANIMATION = componentManager.RegisterEvent("MAINTAIN_ANIMATION",componentType);
	EVENT_MAINTAIN_ANIMATION_MOVEMENT = componentManager.RegisterEvent("MAINTAIN_ANIMATION_MOVEMENT",componentType);
	EVENT_SHOULD_UPDATE_BONES = componentManager.RegisterEvent("SHOULD_UPDATE_BONES",componentType);

	EVENT_ON_PLAY_ACTIVITY = componentManager.RegisterEvent("ON_PLAY_ACTIVITY",componentType);
	EVENT_ON_STOP_LAYERED_ANIMATION = componentManager.RegisterEvent("ON_STOP_LAYERED_ANIMATION",componentType);
	EVENT_ON_BONE_TRANSFORM_CHANGED = componentManager.RegisterEvent("ON_BONE_TRANSFORM_CHANGED");
	EVENT_ON_ANIMATIONS_UPDATED = componentManager.RegisterEvent("ON_ANIMATIONS_UPDATED",componentType);
	EVENT_ON_BLEND_ANIMATION = componentManager.RegisterEvent("ON_BLEND_ANIMATION",componentType);
	EVENT_PLAY_ANIMATION = componentManager.RegisterEvent("PLAY_ANIMATION",componentType);
}

BaseAnimatedComponent::BaseAnimatedComponent(BaseEntity &ent)
	: BaseEntityComponent(ent),m_playbackRate(util::FloatProperty::Create(1.f))
{}

void BaseAnimatedComponent::Initialize()
{
	BaseEntityComponent::Initialize();

	BindEventUnhandled(BaseModelComponent::EVENT_ON_MODEL_CHANGED,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		OnModelChanged(static_cast<pragma::CEOnModelChanged&>(evData.get()).model);
		return util::EventReply::Unhandled;
	});

	BindEventUnhandled(LogicComponent::EVENT_ON_TICK,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		auto &ent = GetEntity();
		auto pLogicComponent = ent.GetComponent<pragma::LogicComponent>();
		if(pLogicComponent.expired())
			return;
		if(ShouldUpdateBones() == true)
		{
			auto pTimeScaleComponent = ent.GetTimeScaleComponent();
			MaintainAnimations(pLogicComponent->DeltaTime() *(pTimeScaleComponent.valid() ? pTimeScaleComponent->GetEffectiveTimeScale() : 1.f));
		}
	});

	BindEventUnhandled(BasePhysicsComponent::EVENT_ON_PRE_PHYSICS_SIMULATE,[this](std::reference_wrapper<pragma::ComponentEvent> evData) {
		if(IsPlayingAnimation() == false)
			return;
		auto pPhysComponent = GetEntity().GetPhysicsComponent();
		if(pPhysComponent.expired())
			return;
		auto *phys = pPhysComponent->GetPhysicsObject();
		if(phys != nullptr && phys->IsController() == true)
			MaintainAnimationMovement(m_animDisplacement);
	});

	auto &ent = GetEntity();
	auto *mdlComponent = static_cast<pragma::BaseModelComponent*>(ent.AddComponent("model").get());
	if(mdlComponent != nullptr)
	{
		auto &mdl = mdlComponent->GetModel();
		OnModelChanged(mdl);
	}
	ent.AddComponent<LogicComponent>(); // Required for animation updates
}

void BaseAnimatedComponent::OnModelChanged(const std::shared_ptr<Model> &mdl)
{
	m_animSlots.clear();
	m_baseAnim = {};
	m_blendControllers.clear();
	m_bones.clear();
	m_processedBones.clear();
	ApplyAnimationEventTemplates();
	if(mdl == nullptr)
		return;
	std::vector<BlendController> &blendControllers = mdl->GetBlendControllers();
	for(unsigned int i=0;i<blendControllers.size();i++)
	{
		BlendController &blend = blendControllers[i];
		int val;
		if(blend.max < 0)
			val = blend.max;
		else
			val = 0;
		m_blendControllers.insert(std::unordered_map<unsigned int,int>::value_type(i,val));
	}
	Skeleton &skeleton = mdl->GetSkeleton();
	for(unsigned int i=0;i<skeleton.GetBoneCount();i++)
		m_bones.push_back(Transform());
	std::unordered_map<std::string,unsigned int> *animations;
	mdl->GetAnimations(&animations);
	std::unordered_map<std::string,unsigned int>::iterator it;
	int autoplaySlot = 1'200; // Arbitrary start slot number for autoplay layered animations
	for(it=animations->begin();it!=animations->end();it++)
	{
		unsigned int animID = it->second;
		auto anim = mdl->GetAnimation(animID);
		if(anim->HasFlag(FAnim::Autoplay))
		{
			PlayLayeredAnimation(autoplaySlot,animID);
			autoplaySlot++;
		}
	}

	auto anim = mdl->GetAnimation(0);
	auto frame = (anim != nullptr) ? anim->GetFrame(0) : nullptr;
	if(frame != nullptr)
	{
		for(UInt32 i=0;i<anim->GetBoneCount();i++)
		{
			m_bones[i].SetPosition(*frame->GetBonePosition(i));
			m_bones[i].SetOrientation(*frame->GetBoneOrientation(i));
		}
	}
}

CallbackHandle BaseAnimatedComponent::BindAnimationEvent(AnimationEvent::Type eventId,const std::function<void(std::reference_wrapper<const AnimationEvent>)> &fCallback)
{
	auto it = m_boundAnimEvents.find(eventId);
	if(it != m_boundAnimEvents.end())
	{
		if(it->second.IsValid())
			it->second.Remove();
		m_boundAnimEvents.erase(it);
	}
	auto hCb = FunctionCallback<void,std::reference_wrapper<const AnimationEvent>>::Create(fCallback);
	m_boundAnimEvents.insert(std::make_pair(eventId,hCb));
	return hCb;
}

bool BaseAnimatedComponent::IsPlayingAnimation() const {return (GetAnimation() >= 0) ? true : false;}

float BaseAnimatedComponent::GetAnimationDuration() const
{
	int seq = GetAnimation();
	if(seq == -1)
		return 0.f;
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return 0.f;
	auto anim = hModel->GetAnimation(seq);
	if(anim == NULL)
		return 0.f;
	return (1.f -m_baseAnim.cycle) *anim->GetDuration();
}

int BaseAnimatedComponent::SelectWeightedAnimation(Activity activity,int animAvoid) const
{
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return -1;
	return hModel->SelectWeightedAnimation(activity,animAvoid);
}

void BaseAnimatedComponent::SetLastAnimationBlendScale(float scale)
{
	m_baseAnim.lastAnim.blendScale = scale;
}

void BaseAnimatedComponent::SetBlendController(unsigned int controller,float val)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return;
	auto it = m_blendControllers.find(controller);
	if(it == m_blendControllers.end())
		return;
	BlendController *blend = hModel->GetBlendController(it->first);
	if(blend == NULL)
		return;
	//if(it->second != val)
	//	std::cout<<"Changed from "<<it->second<<" to "<<val<<std::endl;
	auto min = static_cast<float>(blend->min);
	auto max = static_cast<float>(blend->max);
	if(val > max)
	{
		if(blend->loop == true)
			val = min +(val -max);
		else
			val = max;
	}
	else if(val < min)
	{
		if(blend->loop == true)
			val = max +val;
		else
			val = min;
	}
	it->second = val;
}
void BaseAnimatedComponent::SetBlendController(const std::string &controller,float val)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	if(mdlComponent.expired())
		return;
	int id = mdlComponent->LookupBlendController(controller);
	if(id == -1)
		return;
	SetBlendController(id,val);
}
const std::unordered_map<unsigned int,float> &BaseAnimatedComponent::GetBlendControllers() const {return m_blendControllers;}
float BaseAnimatedComponent::GetBlendController(const std::string &controller) const
{
	auto mdlComponent = GetEntity().GetModelComponent();
	if(mdlComponent.expired())
		return 0.f;
	int id = mdlComponent->LookupBlendController(controller);
	if(id == -1)
		return 0.f;
	return GetBlendController(id);
}
float BaseAnimatedComponent::GetBlendController(unsigned int controller) const
{
	auto it = m_blendControllers.find(controller);
	if(it == m_blendControllers.end())
		return 0;
	return it->second;
}
static Frame *get_frame_from_cycle(Animation &anim,float cycle,uint32_t frameOffset=0)
{
	return anim.GetFrame(static_cast<uint32_t>((anim.GetFrameCount() -1) *cycle) +frameOffset).get();
}
bool BaseAnimatedComponent::GetBlendFramesFromCycle(Animation &anim,float cycle,Frame **frameA,Frame **frameB,float &blendScale,int32_t frameOffset)
{
	auto frameVal = (anim.GetFrameCount() -1) *cycle;
	blendScale = frameVal -static_cast<float>(umath::floor(frameVal));
	*frameA = anim.GetFrame(umath::max(static_cast<int32_t>(frameVal) +frameOffset,0)).get();
	if(*frameA == nullptr)
		return false;
	auto *f = anim.GetFrame(umath::max(static_cast<int32_t>(frameVal) +1 +frameOffset,0)).get();
	if(f == *frameA) // No need to blend if both frames are the same
		blendScale = 0.f;
	else
		*frameB = f;
	return true;
}
void BaseAnimatedComponent::GetAnimationBlendController(Animation *anim,float cycle,std::array<AnimationBlendInfo,2> &bcFrames,float *blendScale) const
{
	struct BlendTransition
	{
		BlendTransition()
			: transition(NULL),value(0.f)
		{}
		AnimationBlendControllerTransition *transition;
		float value;
		void Set(AnimationBlendControllerTransition &tr)
		{
			transition = &tr;
			value = CFloat(tr.transition);
		}
		void Clear()
		{
			transition = NULL;
			value = 0.f;
		}
	};
	AnimationBlendController *controller = anim->GetBlendController();
	float blendControllerScale = 0.f;
	if(controller != NULL && !controller->transitions.empty())
	{
		auto mdlComponent = GetEntity().GetModelComponent();
		auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
		BlendController *blend = hModel->GetBlendController(controller->controller);
		if(blend != NULL)
		{
			auto val = GetBlendController(controller->controller);
			BlendTransition tSrc;
			BlendTransition tTgt;
			tSrc.Set(controller->transitions.front());
			tTgt.Set(controller->transitions.back());
			float scale = 0.f;
			for(unsigned int i=0;i<controller->transitions.size();i++)
			{
				AnimationBlendControllerTransition &tr = controller->transitions[i];
				if(tr.transition <= val && tr.transition > tSrc.value)
					tSrc.Set(tr);
				if(tr.transition >= val && tr.transition < tTgt.value)
					tTgt.Set(tr);
			}
			float offset = (tTgt.value -tSrc.value);
			if(offset > 0.f)
				scale = (val -tSrc.value) /offset;
			auto blendSrc = hModel->GetAnimation(tSrc.transition->animation);
			auto blendTgt = hModel->GetAnimation(tTgt.transition->animation);
			auto &srcBlend = bcFrames.at(0);
			auto &dstBlend = bcFrames.at(1);
			srcBlend.animation = blendSrc.get();
			dstBlend.animation = blendTgt.get();
			GetBlendFramesFromCycle(*blendSrc,cycle,&srcBlend.frameSrc,&srcBlend.frameDst,srcBlend.scale);
			GetBlendFramesFromCycle(*blendTgt,cycle,&dstBlend.frameSrc,&dstBlend.frameDst,dstBlend.scale);
			blendControllerScale = scale;
		}
	}
	*blendScale = blendControllerScale;
}
Frame *BaseAnimatedComponent::GetPreviousAnimationBlendFrame(AnimationSlotInfo &animInfo,double tDelta,float &blendScale)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return nullptr;
	Frame *frameLastAnim = nullptr;
	auto &lastAnim = animInfo.lastAnim;
	if(lastAnim.animation != -1)
	{
		lastAnim.blendTimeScale.second -= static_cast<float>(tDelta);
		if(lastAnim.blendTimeScale.second <= 0.f)
		{
			lastAnim.blendTimeScale = {0.f,0.f};
			lastAnim.animation = -1;
		}
		else
		{
			auto anim = hModel->GetAnimation(lastAnim.animation);
			if(anim != nullptr)
			{
				auto frameLast = umath::floor((anim->GetFrameCount() -1) *lastAnim.cycle);
				frameLastAnim = anim->GetFrame(frameLast).get();
			}
		}
		blendScale = ((lastAnim.blendTimeScale.first != 0.f) ? (lastAnim.blendTimeScale.second /lastAnim.blendTimeScale.first) : 0.f) *lastAnim.blendScale;
	}
	return frameLastAnim;
}
void BaseAnimatedComponent::ApplyAnimationBlending(AnimationSlotInfo &animInfo,double tDelta)
{
	auto blendScale = 0.f;
	auto *frameLastAnim = GetPreviousAnimationBlendFrame(animInfo,tDelta,blendScale);
	if(frameLastAnim == nullptr)
		return;
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return;
	auto anim = hModel->GetAnimation(animInfo.animation);
	if(anim == nullptr)
		return;
	BlendBoneFrames(animInfo.boneOrientations,!animInfo.boneScales.empty() ? &animInfo.boneScales : nullptr,*anim,frameLastAnim,blendScale);
}

bool BaseAnimatedComponent::MaintainAnimation(AnimationSlotInfo &animInfo,double dt,int32_t layeredSlot)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return false;
	CEMaintainAnimation evData{animInfo,dt};
	if(InvokeEventCallbacks(EVENT_MAINTAIN_ANIMATION,evData) == util::EventReply::Handled)
		return false;
	if(animInfo.animation == -1)
		return false;
	auto animId = animInfo.animation;
	auto anim = hModel->GetAnimation(animId);
	if(anim == nullptr)
		return false;
	auto act = anim->GetActivity();
	auto animSpeed = GetPlaybackRate();
	auto numFrames = anim->GetFrameCount();
	if(numFrames > 0)
		animSpeed *= static_cast<float>(anim->GetFPS()) /static_cast<float>(numFrames);

	auto &cycle = animInfo.cycle;
	auto cycleLast = cycle;
	auto cycleNew = cycle +static_cast<float>(dt) *animSpeed;
	auto bLoop = anim->HasFlag(FAnim::Loop);
	auto bComplete = (cycleNew >= 1.f) ? true : false;
	if(bComplete == true)
	{
		cycle = 1.f;
		if(&animInfo == &m_baseAnim) // Only if this is the main animation
		{
			CEOnAnimationComplete evData{animId,act};
			InvokeEventCallbacks(EVENT_ON_ANIMATION_COMPLETE,evData);
		}
		else
		{
			CELayeredAnimationInfo evData{layeredSlot,animId,act};
			InvokeEventCallbacks(EVENT_ON_LAYERED_ANIMATION_COMPLETE,evData);
		}
		if(cycleLast > 0.f) // If current cycle is 0 but we're also complete, that means the animation was started and finished within a single frame. Calling the block below may result in endless recursion, so we need to make sure the animation stays for this frame.
		{
			if(cycle != 1.f || animId != animInfo.animation)
				return MaintainAnimation(animInfo,dt);
			if(bLoop == true)
			{
				cycleNew -= floor(cycleNew);
				if(anim->HasFlag(FAnim::NoRepeat))
				{
					animId = SelectWeightedAnimation(act,animId);
					cycle = cycleNew;
					return MaintainAnimation(animInfo,dt);
				}
			}
			else
				cycleNew = 1.f;
		}
		else
			cycleNew = 1.f;
		cycle = cycleNew;
	}
	else
		cycle = cycleNew;

#if DEBUG_VERBOSE_ANIMATION == 1
	if(&animInfo == &m_baseAnim)
	{
		Con::cout<<
			GetEntity().GetClass()<<" is playing base animation '"<<hModel->GetAnimationName(animId)<<"'"
			<<": Cycle "<<cycle<<" => "<<cycleNew
			<<"; Looping: "<<(bLoop ? "true" : "false")
			<<"; Frame Count: "<<numFrames
			<<"; Speed: "<<animSpeed<<Con::endl;
	}
#endif

	// Apply animation blending
	auto bcBlendScale = 0.f;
	// First element contains the previous and current frame of the main animation.
	// Second element contains previous and current frame of animation that needs to be blended to due
	// to a blend controller (if the main animation has one). This is usually only used for movement animations
	// and will otherwise be null.
	// 'bcBlendScale' is the blend controller lerp scale that needs to be applied between the two.
	std::array<AnimationBlendInfo,2> blendInfo;
	GetAnimationBlendController(anim.get(),cycle,blendInfo,&bcBlendScale); // Blend scale?

	if(blendInfo.front().frameSrc == nullptr) // TODO: If this makes trouble, try changing to blendInfo.front().frameDst
	{
		auto &bi = blendInfo.front();
		if(GetBlendFramesFromCycle(*anim,cycle,&bi.frameSrc,&bi.frameDst,bi.scale) == true)
			blendInfo.front().animation = anim.get();
		else
			return false; // Animation doesn't have any frames?
	}

	for(auto i=blendInfo.size();i>0;)
	{
		--i;
		auto &bi = blendInfo.at(i);
		auto *anim = bi.animation;
		auto *frame = bi.frameSrc;
		auto *frameBlend = bi.frameDst;
		if(anim == nullptr || frame == nullptr)
			continue;
		auto blendScale = bi.scale;

		auto &bones = anim->GetBoneList();
		auto &boneOrientations = (i == 0) ? animInfo.boneOrientations : animInfo.boneOrientationsBc;
		auto &boneScales = (i == 0) ? animInfo.boneScales : animInfo.boneScalesBc;
		const auto defaultBoneScale = Vector3(1.f,1.f,1.f);
		if(bones.size() > boneOrientations.size())
		{
			boneOrientations.resize(bones.size());
			boneScales.resize(bones.size(),defaultBoneScale);
		}
		else
			std::fill(boneScales.begin(),boneScales.end(),defaultBoneScale);
		auto bUseScale = (frame->HasScaleTransforms() || (frameBlend != nullptr && frameBlend->HasScaleTransforms())) ? true : false;
		for(uint32_t i=0;i<static_cast<uint32_t>(boneOrientations.size());++i)
		{
			auto *ppos = frame->GetBonePosition(i);
			auto *prot = frame->GetBoneOrientation(i);
			auto weight = anim->GetBoneWeight(i);
			if(ppos != nullptr && prot != nullptr)
			{
				auto &orientation = boneOrientations[i] = Orientation(*ppos,*prot);
				if(weight != 1.f)
				{
					orientation.pos *= weight;
					orientation.rot = glm::slerp(uquat::identity(),orientation.rot,weight);
				}
			}
			if(bUseScale == true)
			{
				auto *scale = frame->GetBoneScale(i);
				if(scale != nullptr)
					boneScales.at(i) = *scale *weight;
			}
		}

		// Blend between the previous and the current frame
		if(frameBlend != nullptr)
			BlendBoneFrames(boneOrientations,(bUseScale == true) ? &boneScales : nullptr,*anim,frameBlend,blendScale);

		if(i == 1)
			continue;
		BlendBoneFrames(animInfo.boneOrientations,&animInfo.boneScales,animInfo.boneOrientationsBc,&animInfo.boneScalesBc,bcBlendScale);

		// Apply blending from previous animations which are still being faded out, or movement animations, etc.
		ApplyAnimationBlending(animInfo,dt);

		CEOnBlendAnimation evData{animInfo,act,boneOrientations,(bUseScale == true) ? &boneScales : nullptr};
		InvokeEventCallbacks(EVENT_ON_BLEND_ANIMATION,evData);
	}

	// Deprecated?
	/*Frame *frame = nullptr;
	Frame *frameBlend = nullptr;
	auto blendScale = 0.f;
	GetAnimationBlendController(anim.get(),cycle,&frame,&frameBlend,&blendScale);

	if(frame == nullptr && GetBlendFramesFromCycle(*anim,cycle,&frame,&frameBlend,blendScale) == false)
		return false; // Animation doesn't have any frames?
		
	auto &bones = anim->GetBoneList();
	auto &boneOrientations = animInfo.boneOrientations;
	auto &boneScales = animInfo.boneScales;
	const auto defaultBoneScale = Vector3(1.f,1.f,1.f);
	if(bones.size() > boneOrientations.size())
	{
		boneOrientations.resize(bones.size());
		boneScales.resize(bones.size(),defaultBoneScale);
	}
	else
		std::fill(boneScales.begin(),boneScales.end(),defaultBoneScale);
	auto bUseScale = (frame->HasScaleTransforms() || (frameBlend != nullptr && frameBlend->HasScaleTransforms())) ? true : false;
	for(uint32_t i=0;i<static_cast<uint32_t>(boneOrientations.size());++i)
	{
		auto *ppos = frame->GetBonePosition(i);
		auto *prot = frame->GetBoneOrientation(i);
		auto weight = anim->GetBoneWeight(i);
		if(ppos != nullptr && prot != nullptr)
		{
			auto &orientation = boneOrientations[i] = Orientation(*ppos,*prot);
			if(weight != 1.f)
			{
				orientation.pos *= weight;
				orientation.rot = glm::slerp(uquat::identity(),orientation.rot,weight);
			}
		}
		if(bUseScale == true)
		{
			auto *scale = frame->GetBoneScale(i);
			if(scale != nullptr)
				boneScales.at(i) = *scale *weight;
		}
	}

	// Blend between the previous and the current frame
	if(frameBlend != nullptr)
		BlendBoneFrames(boneOrientations,(bUseScale == true) ? &boneScales : nullptr,*anim,frameBlend,blendScale);

	// Apply blending from previous animations which are still being faded out, or movement animations, etc.
	ApplyAnimationBlending(animInfo,tDelta);

	BlendAnimation(animInfo,act,boneOrientations,(bUseScale == true) ? &boneScales : nullptr);*/

/*
	if(bMove == true)
	{
		if(boneID == 0)
		{
			Vector2 *move = frame->GetMoveOffset();
			float fScale = float(anim->GetFPS()) *GetPlaybackRate();
			m_animDisplacement.x = move->x *fScale;
			m_animDisplacement.z = move->y *fScale;
			std::cout<<move->y<<std::endl;
		}
	}
*/

	auto frameLast = (cycleLast != 0.f) ? static_cast<int32_t>((numFrames -1) *cycleLast) : -1;
	auto frameCycle = (numFrames -1) *cycle;
	auto frameID = umath::floor(frameCycle);

	if(frameID < frameLast)
		frameID = numFrames;

	m_animEventQueue.push({});
	auto &eventItem = m_animEventQueue.back();
	eventItem.animId = animId;
	eventItem.animation = anim;
	eventItem.frameId = frameID;
	eventItem.lastFrame = frameLast;
	return true;
}

bool BaseAnimatedComponent::MaintainAnimations(double dt)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return false;
	CEMaintainAnimations evData{dt};
	if(InvokeEventCallbacks(EVENT_MAINTAIN_ANIMATIONS,evData) == util::EventReply::Handled)
		return true;

	auto r = MaintainAnimation(m_baseAnim,dt);
	if(r == true)
	{
		auto &animInfo = m_baseAnim;
		auto anim = hModel->GetAnimation(animInfo.animation);
		auto &bones = anim->GetBoneList();
		auto &boneOrientations = animInfo.boneOrientations;
		auto &boneScales = animInfo.boneScales;

		// Update gestures
		for(auto it=m_animSlots.begin();it!=m_animSlots.end();)
		{
			auto &animInfo = it->second;
			if(MaintainAnimation(animInfo,dt,it->first) == true)
			{
				auto anim = hModel->GetAnimation(animInfo.animation);
				TransformBoneFrames(boneOrientations,&boneScales,anim,animInfo.boneOrientations,!animInfo.boneScales.empty() ? &animInfo.boneScales : nullptr,anim->HasFlag(FAnim::Gesture));
				if(animInfo.cycle >= 1.f)
				{
					if(anim->HasFlag(FAnim::Loop) == false)
					{
						it = m_animSlots.erase(it); // No need to keep the gesture information around anymore
						continue;
					}
				}
			}
			++it;
		}

		// Apply animation to skeleton
		for(auto i=decltype(boneOrientations.size()){0};i<boneOrientations.size();++i)
		{
			auto boneId = bones[i];
			auto &orientation = boneOrientations[i];
			SetBonePosition(boneId,orientation.pos,orientation.rot,nullptr,false);
			if(boneScales.empty() == false)
				SetBoneScale(boneId,boneScales.at(i));
		}
	}

	InvokeEventCallbacks(EVENT_ON_ANIMATIONS_UPDATED);

	// It's now safe to execute animation events
	const auto fHandleAnimationEvents = [this](uint32_t animId,const std::shared_ptr<Animation> &anim,int32_t frameId) {
		auto *events = anim->GetEvents(frameId);
		if(events)
		{
			for(auto &ev : *events)
				HandleAnimationEvent(*ev);
		}
		auto *customEvents = GetAnimationEvents(animId,frameId);
		if(customEvents != nullptr)
		{
			for(auto &ev : *customEvents)
			{
				if(ev.callback.first == true) // Is it a callback event?
				{
					if(ev.callback.second.IsValid())
					{
						auto *f = ev.callback.second.get();
						if(typeid(*f) == typeid(LuaCallback))
						{
							auto *lf = static_cast<LuaCallback*>(f);
							lf->Call<void>();
						}
						else
							(*f)();
					}
				}
				else
					HandleAnimationEvent(ev);
			}
		}
	};
	while(!m_animEventQueue.empty())
	{
		auto &eventItem = m_animEventQueue.front();

		for(auto i=eventItem.lastFrame +1;i<=eventItem.frameId;++i)
			fHandleAnimationEvents(eventItem.animId,eventItem.animation,i);

		if(static_cast<int32_t>(eventItem.frameId) < eventItem.lastFrame)
		{
			for(auto i=decltype(eventItem.frameId){0};i<=eventItem.frameId;++i)
				fHandleAnimationEvents(eventItem.animId,eventItem.animation,i);
		}

		m_animEventQueue.pop();
	}
	return r;
}
Activity BaseAnimatedComponent::TranslateActivity(Activity act) {return act;}

float BaseAnimatedComponent::GetCycle() const {return m_baseAnim.cycle;}
void BaseAnimatedComponent::SetCycle(float cycle) {m_baseAnim.cycle = cycle;}

int BaseAnimatedComponent::GetAnimation() const {return m_baseAnim.animation;}
Animation *BaseAnimatedComponent::GetAnimationObject() const
{
	auto animId = GetAnimation();
	if(animId == -1)
		return nullptr;
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return nullptr;
	auto anim = hModel->GetAnimation(animId);
	if(anim == nullptr)
		return nullptr;
	return anim.get();
}
int32_t BaseAnimatedComponent::GetLayeredAnimation(uint32_t slot) const
{
	auto it = m_animSlots.find(slot);
	if(it == m_animSlots.end())
		return -1;
	return it->second.animation;
}
Activity BaseAnimatedComponent::GetLayeredActivity(uint32_t slot) const
{
	auto it = m_animSlots.find(slot);
	if(it == m_animSlots.end())
		return Activity::Invalid;
	return it->second.activity;
}

void BaseAnimatedComponent::PlayAnimation(int animation,FPlayAnim flags)
{
	auto bSkipAnim = false;
	CEOnPlayAnimation evData{m_baseAnim.animation,animation,flags};
	if(InvokeEventCallbacks(EVENT_PLAY_ANIMATION,evData) == util::EventReply::Handled)
		return;
	if(m_baseAnim.animation == animation && (flags &FPlayAnim::Reset) == FPlayAnim::None)
	{
		auto mdlComponent = GetEntity().GetModelComponent();
		auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
		if(hModel != nullptr)
		{
			auto anim = hModel->GetAnimation(animation);
			if(anim != NULL && anim->HasFlag(FAnim::Loop))
				return;
		}
	}
	if(animation < -1)
		animation = -1;

	CETranslateAnimation evTranslateAnimData {animation,flags};
	InvokeEventCallbacks(EVENT_TRANSLATE_ANIMATION,evTranslateAnimData);

	if(animation == m_baseAnim.animation && m_baseAnim.cycle == 0.f && m_baseAnim.flags == flags)
		return; // No change
	auto &lastAnim = m_baseAnim.lastAnim;
	if(m_baseAnim.animation != -1 && m_baseAnim.animation != animation && m_baseAnim.cycle > 0.f)
	{
		lastAnim.animation = m_baseAnim.animation;
		lastAnim.cycle = m_baseAnim.cycle;
		lastAnim.flags = m_baseAnim.flags;
		lastAnim.blendTimeScale = {0.f,0.f};
		lastAnim.blendScale = 1.f;

		// Update animation fade time
		auto mdlComponent = GetEntity().GetModelComponent();
		auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
		if(hModel != nullptr)
		{
			auto anim = hModel->GetAnimation(animation);
			auto animLast = hModel->GetAnimation(m_baseAnim.animation);
			if(anim != nullptr && animLast != nullptr)
			{
				auto bAnimFadeIn = anim->HasFadeInTime();
				auto bAnimLastFadeOut = animLast->HasFadeOutTime();
				auto animFadeIn = anim->GetFadeInTime();
				auto animFadeOut = anim->GetFadeOutTime();
				UNUSED(animFadeOut);
				auto animLastFadeOut = animLast->GetFadeOutTime();
				const auto defaultFadeOutTime = 0.2f;
				if(bAnimFadeIn == true)
				{
					if(bAnimLastFadeOut == true)
						lastAnim.blendTimeScale.first = (animFadeIn > animLastFadeOut) ? animFadeIn : animLastFadeOut;
					else
						lastAnim.blendTimeScale.first = animFadeIn;
				}
				else if(bAnimLastFadeOut == true)
					lastAnim.blendTimeScale.first = animLastFadeOut;
				else
					lastAnim.blendTimeScale.first = defaultFadeOutTime;
				lastAnim.blendTimeScale.second = lastAnim.blendTimeScale.first;
			}
		}
		//
	}
	else
		lastAnim.animation = -1;
	m_baseAnim.animation = animation;
	m_baseAnim.cycle = 0;
	m_baseAnim.flags = flags;
	m_baseAnim.activity = Activity::Invalid;
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel != nullptr)
	{
		auto anim = hModel->GetAnimation(animation);
		if(anim != nullptr)
			m_baseAnim.activity = anim->GetActivity();
	}

	CEOnAnimationStart evAnimStartData {m_baseAnim.animation,m_baseAnim.activity,m_baseAnim.flags};
	InvokeEventCallbacks(EVENT_ON_ANIMATION_START,evAnimStartData);
}

int32_t BaseAnimatedComponent::SelectTranslatedAnimation(Activity activity) const
{
	activity = const_cast<BaseAnimatedComponent*>(this)->TranslateActivity(activity);
	return SelectWeightedAnimation(activity,m_baseAnim.animation);
}

bool BaseAnimatedComponent::PlayActivity(Activity activity,FPlayAnim flags)
{
	if(GetActivity() == activity && (flags &FPlayAnim::Reset) == FPlayAnim::None)
		return true;
	CETranslateActivity evTranslateActivityData {activity,flags};
	InvokeEventCallbacks(EVENT_TRANSLATE_ACTIVITY,evTranslateActivityData);

	CEOnPlayActivity evDataActivity {activity,flags};
	InvokeEventCallbacks(EVENT_ON_PLAY_ACTIVITY,evDataActivity);

	auto seq = SelectTranslatedAnimation(activity);
	PlayAnimation(seq,flags);
	//m_baseAnim.activity = activity;
	return (seq == -1) ? false : true;
}

Activity BaseAnimatedComponent::GetActivity() const
{
	if(m_baseAnim.animation == -1)
		return Activity::Invalid;
	if(m_baseAnim.activity != Activity::Invalid)
		return m_baseAnim.activity;
	auto mdlComponent = GetEntity().GetModelComponent();
	auto hModel = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	if(hModel == nullptr)
		return Activity::Invalid;
	auto anim = hModel->GetAnimation(m_baseAnim.animation);
	if(anim == NULL)
		return Activity::Invalid;
	return anim->GetActivity();
}

void BaseAnimatedComponent::SetPlaybackRate(float rate) {*m_playbackRate = rate;}
float BaseAnimatedComponent::GetPlaybackRate() const {return *m_playbackRate;}
const util::PFloatProperty &BaseAnimatedComponent::GetPlaybackRateProperty() const {return m_playbackRate;}

void BaseAnimatedComponent::HandleAnimationEvent(const AnimationEvent &ev)
{
	auto bHandled = false;
	CEHandleAnimationEvent evData{ev};
	if(InvokeEventCallbacks(EVENT_HANDLE_ANIMATION_EVENT,evData) == util::EventReply::Handled)
		return;
	auto it = m_boundAnimEvents.find(ev.eventID);
	if(it != m_boundAnimEvents.end())
	{
		it->second.Call<void,std::reference_wrapper<const AnimationEvent>>(ev);
		return;
	}
	switch(ev.eventID)
	{
		case AnimationEvent::Type::EmitSound:
		{
			if(ev.arguments.size() > 0)
			{
				auto pSoundEmitterComponent = static_cast<pragma::BaseSoundEmitterComponent*>(GetEntity().FindComponent("sound_emitter").get());
				if(pSoundEmitterComponent != nullptr)
					pSoundEmitterComponent->EmitSharedSound(ev.arguments.front(),ALSoundType::Generic);
			}
			break;
		}
		default:
			;//Con::cout<<"WARNING: Unhandled animation event "<<ev->eventID<<Con::endl;
	}
}

const BaseAnimatedComponent::AnimationSlotInfo &BaseAnimatedComponent::GetBaseAnimationInfo() const {return const_cast<BaseAnimatedComponent&>(*this).GetBaseAnimationInfo();}
BaseAnimatedComponent::AnimationSlotInfo &BaseAnimatedComponent::GetBaseAnimationInfo() {return m_baseAnim;}
const std::unordered_map<uint32_t,BaseAnimatedComponent::AnimationSlotInfo> &BaseAnimatedComponent::GetAnimationSlotInfos() const {return const_cast<BaseAnimatedComponent&>(*this).GetAnimationSlotInfos();}
std::unordered_map<uint32_t,BaseAnimatedComponent::AnimationSlotInfo> &BaseAnimatedComponent::GetAnimationSlotInfos() {return m_animSlots;}

bool BaseAnimatedComponent::PlayAnimation(const std::string &name,FPlayAnim flags)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	if(mdlComponent.expired())
		return false;
	auto prevAnim = GetAnimation();
	int anim = mdlComponent->LookupAnimation(name);

	CEOnPlayAnimation evData {prevAnim,anim,flags};
	if(InvokeEventCallbacks(EVENT_ON_PLAY_ANIMATION,evData) == util::EventReply::Handled)
		return false;

	PlayAnimation(anim,flags);
	return true;
}

void BaseAnimatedComponent::PlayLayeredAnimation(int slot,int animation,FPlayAnim flags,AnimationSlotInfo **animInfo)
{
	auto prevAnim = GetLayeredAnimation(slot);

	CETranslateLayeredAnimation evData {slot,animation,flags};
	InvokeEventCallbacks(EVENT_TRANSLATE_LAYERED_ANIMATION,evData);

	CEOnPlayLayeredAnimation evDataPlay {slot,prevAnim,animation,flags};
	InvokeEventCallbacks(EVENT_ON_PLAY_LAYERED_ANIMATION,evDataPlay);

	auto &slotInfo = m_animSlots[slot] = {animation};
	slotInfo.flags = flags;
	if(animInfo != nullptr)
		*animInfo = &slotInfo;

	CELayeredAnimationInfo evDataStart {slot,slotInfo.animation,slotInfo.activity};
	InvokeEventCallbacks(EVENT_ON_LAYERED_ANIMATION_START,evDataStart);
}
void BaseAnimatedComponent::PlayLayeredAnimation(int slot,int animation,FPlayAnim flags)
{
	PlayLayeredAnimation(slot,animation,flags,nullptr);
}
bool BaseAnimatedComponent::PlayLayeredAnimation(int slot,std::string animation,FPlayAnim flags)
{
	auto mdlComponent = GetEntity().GetModelComponent();
	if(mdlComponent.expired())
		return false;
	auto anim = mdlComponent->LookupAnimation(animation);
	if(anim == -1)
		return false;
	PlayLayeredAnimation(slot,anim,flags);
	return true;
}
bool BaseAnimatedComponent::PlayLayeredActivity(int slot,Activity activity,FPlayAnim flags)
{
	CEOnPlayLayeredActivity evData{slot,activity,flags};
	InvokeEventCallbacks(EVENT_ON_PLAY_LAYERED_ACTIVITY,evData);

	int32_t animAvoid = -1;
	auto it = m_animSlots.find(slot);
	if(it != m_animSlots.end())
		animAvoid = it->second.animation;
	auto seq = SelectWeightedAnimation(activity,animAvoid);
	AnimationSlotInfo *animInfo = nullptr;
	PlayLayeredAnimation(slot,seq,flags,&animInfo);
	if(animInfo != nullptr)
		animInfo->activity = activity;
	return (seq == -1) ? false : true;
}
void BaseAnimatedComponent::StopLayeredAnimation(int slot)
{
	auto it = m_animSlots.find(slot);
	if(it == m_animSlots.end())
		return;
	CEOnStopLayeredAnimation evData{slot,it->second};
	InvokeEventCallbacks(EVENT_ON_STOP_LAYERED_ANIMATION,evData);
	m_animSlots.erase(it);
}
/*
static void SetupMatrices(std::vector<Orientation> &boneOrientations,std::unordered_map<unsigned int,Bone*> *bones,Vector3 pos=Vector3(0,0,0),Quat rot=uquat::identity())
{
	// Grab ID from GetBoneList(anim)
	std::unordered_map<unsigned int,Bone*>::iterator it;
	for(it=bones->begin();it!=bones->end();it++)
	{
		Vector3 posParent = pos;
		Quat rotParent = rot;
		if(it->first < boneOrientations.size())
		{
			Orientation &orientation = boneOrientations[it->first];
			Vector3 lpos = orientation.pos;
			lpos = rotParent *lpos;
			posParent += lpos;
			rotParent = orientation.rot *rotParent;
			orientation.pos = posParent;
			orientation.rot = rotParent;
		}
		Bone *bone = it->second;
		SetupMatrices(boneOrientations,&bone->children,posParent,rotParent);
	}
}
*/

const std::vector<Transform> &BaseAnimatedComponent::GetProcessedBones() const {return const_cast<BaseAnimatedComponent*>(this)->GetProcessedBones();}
std::vector<Transform> &BaseAnimatedComponent::GetProcessedBones() {return m_processedBones;}

bool BaseAnimatedComponent::CalcAnimationMovementSpeed(float *x,float *z,int32_t frameOffset) const
{
	auto &ent = GetEntity();
	auto mdlComponent = ent.GetModelComponent();
	auto hMdl = mdlComponent.valid() ? mdlComponent->GetModel() : nullptr;
	auto animId = GetAnimation();
	if(hMdl == nullptr || animId == -1)
		return false;
	auto anim = hMdl->GetAnimation(animId);
	if(anim == nullptr || (((x != nullptr && anim->HasFlag(FAnim::MoveX) == false) || x == nullptr) && ((z != nullptr && anim->HasFlag(FAnim::MoveZ) == false) || z == nullptr)))
		return false;

	std::array<Frame*,2> frames = {nullptr,nullptr};
	auto blendScale = 0.f;
	if(GetBlendFramesFromCycle(*anim,GetCycle(),&frames[0],&frames[1],blendScale,frameOffset) == false)
		return false; // Animation doesn't have any frames?
	auto animSpeed = GetPlaybackRate();
	std::array<float,2> blendScales = {1.f -blendScale,blendScale};
	Vector2 mvOffset {0.f,0.f};
	for(auto i=decltype(frames.size()){0};i<frames.size();++i)
	{
		auto *frame = frames[i];
		if(frame == nullptr)
			continue;
		auto *moveOffset = frame->GetMoveOffset();
		if(moveOffset == nullptr)
			continue;
		mvOffset += *moveOffset *blendScales[i] *animSpeed;
	}
	if(x != nullptr)
		*x = mvOffset.x;
	if(z != nullptr)
		*z = mvOffset.y;
	return true;
}

static void write_animation_slot_info(DataStream &ds,const BaseAnimatedComponent::AnimationSlotInfo &slotInfo)
{
	ds->Write<Activity>(slotInfo.activity);
	ds->Write<int32_t>(slotInfo.animation);
	ds->Write<float>(slotInfo.cycle);
	ds->Write<FPlayAnim>(slotInfo.flags);

	ds->Write<uint32_t>(slotInfo.boneOrientations.size());
	ds->Write(reinterpret_cast<const uint8_t*>(slotInfo.boneOrientations.data()),slotInfo.boneOrientations.size() *sizeof(slotInfo.boneOrientations.front()));
		
	ds->Write<uint32_t>(slotInfo.boneScales.size());
	ds->Write(reinterpret_cast<const uint8_t*>(slotInfo.boneScales.data()),slotInfo.boneScales.size() *sizeof(slotInfo.boneScales.front()));
		
	ds->Write<uint32_t>(slotInfo.boneOrientationsBc.size());
	ds->Write(reinterpret_cast<const uint8_t*>(slotInfo.boneOrientationsBc.data()),slotInfo.boneOrientationsBc.size() *sizeof(slotInfo.boneOrientationsBc.front()));
		
	ds->Write<uint32_t>(slotInfo.boneScalesBc.size());
	ds->Write(reinterpret_cast<const uint8_t*>(slotInfo.boneScalesBc.data()),slotInfo.boneScalesBc.size() *sizeof(slotInfo.boneScalesBc.front()));
		
	ds->Write<int32_t>(slotInfo.lastAnim.animation);
	ds->Write<float>(slotInfo.lastAnim.cycle);
	ds->Write<FPlayAnim>(slotInfo.lastAnim.flags);
	ds->Write<std::pair<float,float>>(slotInfo.lastAnim.blendTimeScale);
	ds->Write<float>(slotInfo.lastAnim.blendScale);
}
void BaseAnimatedComponent::Save(DataStream &ds)
{
	BaseEntityComponent::Save(ds);
	ds->Write<float>(GetPlaybackRate());

	// Write blend controllers
	auto &blendControllers = GetBlendControllers();
	ds->Write<std::size_t>(blendControllers.size());
	for(auto &pair : blendControllers)
	{
		ds->Write<uint32_t>(pair.first);
		ds->Write<float>(pair.second);
	}

	// Write animations
	write_animation_slot_info(ds,GetBaseAnimationInfo());
	auto &animSlotInfos = GetAnimationSlotInfos();
	ds->Write<std::size_t>(animSlotInfos.size());
	for(auto &pair : animSlotInfos)
	{
		ds->Write<uint32_t>(pair.first);
		write_animation_slot_info(ds,pair.second);
	}

	ds->Write<Vector3>(m_animDisplacement);
}
static void read_animation_slot_info(DataStream &ds,BaseAnimatedComponent::AnimationSlotInfo &slotInfo)
{
	slotInfo.activity = ds->Read<Activity>();
	slotInfo.animation = ds->Read<int32_t>();
	slotInfo.cycle = ds->Read<float>();
	slotInfo.flags = ds->Read<FPlayAnim>();

	auto numBoneOrientations = ds->Read<uint32_t>();
	slotInfo.boneOrientations.resize(numBoneOrientations);
	ds->Read(reinterpret_cast<uint8_t*>(slotInfo.boneOrientations.data()),slotInfo.boneOrientations.size() *sizeof(slotInfo.boneOrientations.front()));
		
	auto numBoneScales = ds->Read<uint32_t>();
	slotInfo.boneScales.resize(numBoneScales);
	ds->Read(reinterpret_cast<uint8_t*>(slotInfo.boneScales.data()),slotInfo.boneScales.size() *sizeof(slotInfo.boneScales.front()));
		
	auto numBoneOrientationsBc = ds->Read<uint32_t>();
	slotInfo.boneOrientationsBc.resize(numBoneOrientationsBc);
	ds->Read(reinterpret_cast<uint8_t*>(slotInfo.boneOrientationsBc.data()),slotInfo.boneOrientationsBc.size() *sizeof(slotInfo.boneOrientationsBc.front()));

	auto numBoneScalesBc = ds->Read<uint32_t>();
	slotInfo.boneScalesBc.resize(numBoneScalesBc);
	ds->Read(reinterpret_cast<uint8_t*>(slotInfo.boneScalesBc.data()),slotInfo.boneScalesBc.size() *sizeof(slotInfo.boneScalesBc.front()));
		
	slotInfo.lastAnim.animation = ds->Read<int32_t>();
	slotInfo.lastAnim.cycle = ds->Read<float>();
	slotInfo.lastAnim.flags = ds->Read<FPlayAnim>();
	slotInfo.lastAnim.blendTimeScale = ds->Read<std::pair<float,float>>();
	slotInfo.lastAnim.blendScale = ds->Read<float>();
}
void BaseAnimatedComponent::Load(DataStream &ds,uint32_t version)
{
	BaseEntityComponent::Load(ds,version);
	auto playbackRate = ds->Read<float>();
	SetPlaybackRate(playbackRate);

	// Read blend controllers
	auto numBlendControllers = ds->Read<std::size_t>();
	for(auto i=decltype(numBlendControllers){0};i<numBlendControllers;++i)
	{
		auto id = ds->Read<uint32_t>();
		auto val = ds->Read<float>();
		SetBlendController(id,val);
	}

	// Read animations
	read_animation_slot_info(ds,GetBaseAnimationInfo());
	auto &animSlots = GetAnimationSlotInfos();
	auto numAnims = ds->Read<std::size_t>();
	animSlots.reserve(numAnims);
	for(auto i=decltype(numAnims){0};i<numAnims;++i)
	{
		auto id = ds->Read<uint32_t>();
		auto it = animSlots.insert(std::make_pair(id,AnimationSlotInfo{})).first;
		read_animation_slot_info(ds,it->second);
	}

	auto animDisp = ds->Read<Vector3>();
	m_animDisplacement = animDisp;
}

/////////////////

CEHandleAnimationEvent::CEHandleAnimationEvent(const AnimationEvent &animationEvent)
	: animationEvent(animationEvent)
{}
void CEHandleAnimationEvent::PushArguments(lua_State *l)
{
	Lua::PushInt(l,static_cast<int32_t>(animationEvent.eventID));

	auto tArgs = Lua::CreateTable(l);
	auto &args = animationEvent.arguments;
	for(auto i=decltype(args.size()){0};i<args.size();++i)
	{
		Lua::PushInt(l,i +1);
		Lua::PushString(l,args.at(i));
		Lua::SetTableValue(l,tArgs);
	}
}
void CEHandleAnimationEvent::PushArgumentVariadic(lua_State *l)
{
	auto &args = animationEvent.arguments;
	for(auto &arg : args)
		Lua::PushString(l,arg);
}

/////////////////

CEOnPlayAnimation::CEOnPlayAnimation(int32_t previousAnimation,int32_t animation,pragma::FPlayAnim flags)
	: previousAnimation(previousAnimation),animation(animation),flags(flags)
{}
void CEOnPlayAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,previousAnimation);
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(flags));
}

/////////////////

CEOnPlayLayeredAnimation::CEOnPlayLayeredAnimation(int32_t slot,int32_t previousAnimation,int32_t animation,pragma::FPlayAnim flags)
	: CEOnPlayAnimation(previousAnimation,animation,flags),slot(slot)
{}
void CEOnPlayLayeredAnimation::PushArguments(lua_State *l)
{
	CEOnPlayAnimation::PushArguments(l);
	Lua::PushInt(l,slot);
}

/////////////////

CETranslateLayeredActivity::CETranslateLayeredActivity(int32_t &slot,Activity &activity,pragma::FPlayAnim &flags)
	: slot(slot),activity(activity),flags(flags)
{}
void CETranslateLayeredActivity::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slot);
	Lua::PushInt(l,umath::to_integral(activity));
	Lua::PushInt(l,umath::to_integral(flags));
}
uint32_t CETranslateLayeredActivity::GetReturnCount() {return 3;}
void CETranslateLayeredActivity::HandleReturnValues(lua_State *l)
{
	if(Lua::IsSet(l,-3))
		slot = Lua::CheckInt(l,-3);
	if(Lua::IsSet(l,-2))
		activity = static_cast<Activity>(Lua::CheckInt(l,-2));
	if(Lua::IsSet(l,-1))
		flags = static_cast<pragma::FPlayAnim>(Lua::CheckInt(l,-1));
}

/////////////////

CEOnAnimationComplete::CEOnAnimationComplete(int32_t animation,Activity activity)
	: animation(animation),activity(activity)
{}
void CEOnAnimationComplete::PushArguments(lua_State *l)
{
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(activity));
}

/////////////////

CELayeredAnimationInfo::CELayeredAnimationInfo(int32_t slot,int32_t animation,Activity activity)
	: slot(slot),animation(animation),activity(activity)
{}
void CELayeredAnimationInfo::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slot);
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(activity));
}

/////////////////

CEOnAnimationStart::CEOnAnimationStart(int32_t animation,Activity activity,pragma::FPlayAnim flags)
	: animation(animation),activity(activity),flags(flags)
{}
void CEOnAnimationStart::PushArguments(lua_State *l)
{
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(activity));
	Lua::PushInt(l,umath::to_integral(flags));
}

/////////////////

CETranslateLayeredAnimation::CETranslateLayeredAnimation(int32_t &slot,int32_t &animation,pragma::FPlayAnim &flags)
	: slot(slot),animation(animation),flags(flags)
{}
void CETranslateLayeredAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slot);
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(flags));
}
uint32_t CETranslateLayeredAnimation::GetReturnCount() {return 3;}
void CETranslateLayeredAnimation::HandleReturnValues(lua_State *l)
{
	if(Lua::IsSet(l,-3))
		slot = Lua::CheckInt(l,-3);
	if(Lua::IsSet(l,-2))
		animation = Lua::CheckInt(l,-2);
	if(Lua::IsSet(l,-1))
		flags = static_cast<pragma::FPlayAnim>(Lua::CheckInt(l,-1));
}

/////////////////

CETranslateAnimation::CETranslateAnimation(int32_t &animation,pragma::FPlayAnim &flags)
	: animation(animation),flags(flags)
{}
void CETranslateAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,animation);
	Lua::PushInt(l,umath::to_integral(flags));
}
uint32_t CETranslateAnimation::GetReturnCount() {return 2;}
void CETranslateAnimation::HandleReturnValues(lua_State *l)
{
	if(Lua::IsSet(l,-2))
		animation = Lua::CheckInt(l,-2);
	if(Lua::IsSet(l,-1))
		flags = static_cast<pragma::FPlayAnim>(Lua::CheckInt(l,-1));
}

/////////////////

CETranslateActivity::CETranslateActivity(Activity &activity,pragma::FPlayAnim &flags)
	: activity(activity),flags(flags)
{}
void CETranslateActivity::PushArguments(lua_State *l)
{
	Lua::PushInt(l,umath::to_integral(activity));
	Lua::PushInt(l,umath::to_integral(flags));
}
uint32_t CETranslateActivity::GetReturnCount() {return 2;}
void CETranslateActivity::HandleReturnValues(lua_State *l)
{
	if(Lua::IsSet(l,-2))
		activity = static_cast<Activity>(Lua::CheckInt(l,-2));
	if(Lua::IsSet(l,-1))
		flags = static_cast<pragma::FPlayAnim>(Lua::CheckInt(l,-1));
}

/////////////////

CEOnBoneTransformChanged::CEOnBoneTransformChanged(UInt32 boneId,const Vector3 *pos,const Quat *rot,const Vector3 *scale)
	: boneId{boneId},pos{pos},rot{rot},scale{scale}
{}
void CEOnBoneTransformChanged::CEOnBoneTransformChanged::PushArguments(lua_State *l)
{
	Lua::PushInt(l,boneId);
	if(pos != nullptr)
		Lua::Push<Vector3>(l,*pos);
	else
		Lua::PushNil(l);

	if(rot != nullptr)
		Lua::Push<Quat>(l,*rot);
	else
		Lua::PushNil(l);

	if(scale != nullptr)
		Lua::Push<Vector3>(l,*scale);
	else
		Lua::PushNil(l);
}

/////////////////

CEOnPlayActivity::CEOnPlayActivity(Activity activity,FPlayAnim flags)
	: activity{activity},flags{flags}
{}
void CEOnPlayActivity::PushArguments(lua_State *l)
{
	Lua::PushInt(l,umath::to_integral(activity));
	Lua::PushInt(l,umath::to_integral(flags));
}

/////////////////

CEOnPlayLayeredActivity::CEOnPlayLayeredActivity(int slot,Activity activity,FPlayAnim flags)
	: slot{slot},activity{activity},flags{flags}
{}
void CEOnPlayLayeredActivity::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slot);
	Lua::PushInt(l,umath::to_integral(activity));
	Lua::PushInt(l,umath::to_integral(flags));
}

/////////////////

CEOnStopLayeredAnimation::CEOnStopLayeredAnimation(int32_t slot,BaseAnimatedComponent::AnimationSlotInfo &slotInfo)
	: slot{slot},slotInfo{slotInfo}
{}
void CEOnStopLayeredAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slot);
	Lua::PushInt(l,slotInfo.animation);
	Lua::PushInt(l,umath::to_integral(slotInfo.activity));
}

/////////////////

CEOnBlendAnimation::CEOnBlendAnimation(BaseAnimatedComponent::AnimationSlotInfo &slotInfo,Activity activity,std::vector<Orientation> &boneOrientations,std::vector<Vector3> *boneScales)
	: slotInfo{slotInfo},activity{activity},boneOrientations{boneOrientations},boneScales{boneScales}
{}
void CEOnBlendAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slotInfo.animation);
	Lua::PushInt(l,umath::to_integral(activity));
}

/////////////////

CEMaintainAnimations::CEMaintainAnimations(double deltaTime)
	: deltaTime{deltaTime}
{}
void CEMaintainAnimations::PushArguments(lua_State *l)
{
	Lua::PushNumber(l,deltaTime);
}

/////////////////

CEMaintainAnimation::CEMaintainAnimation(BaseAnimatedComponent::AnimationSlotInfo &slotInfo,double deltaTime)
	: slotInfo{slotInfo},deltaTime{deltaTime}
{}
void CEMaintainAnimation::PushArguments(lua_State *l)
{
	Lua::PushInt(l,slotInfo.animation);
	Lua::PushInt(l,umath::to_integral(slotInfo.activity));
	Lua::PushNumber(l,deltaTime);
}

/////////////////

CEMaintainAnimationMovement::CEMaintainAnimationMovement(const Vector3 &displacement)
	: displacement{displacement}
{}
void CEMaintainAnimationMovement::PushArguments(lua_State *l)
{
	Lua::Push<Vector3>(l,displacement);
}

/////////////////

CEShouldUpdateBones::CEShouldUpdateBones()
{}
void CEShouldUpdateBones::PushArguments(lua_State *l) {}