#include "stdafx_shared.h"
#include "pragma/lua/classes/ldef_angle.h"
#include "pragma/lua/classes/ldef_vector.h"
#include "pragma/lua/classes/lmodel.h"
#include "pragma/lua/classes/lanimation.h"
#include "pragma/lua/classes/lskeleton.h"
#include "pragma/lua/classes/lvector.h"
#include "pragma/physics/collisionmesh.h"
#include "pragma/lua/classes/lcollisionmesh.h"
#include "luasystem.h"
#include "pragma/model/model.h"
#include "pragma/physics/collisionmesh.h"
#include "pragma/model/vertex.h"
#include "pragma/physics/physsoftbodyinfo.hpp"

extern DLLENGINE Engine *engine;

void Lua::ModelMeshGroup::register_class(luabind::class_<std::shared_ptr<::ModelMeshGroup>> &classDef)
{
	classDef.scope[luabind::def("Create",&Create)];
	classDef.def(luabind::const_self == std::shared_ptr<::ModelMeshGroup>());
	classDef.def("GetName",&GetName);
	classDef.def("GetMeshes",&GetMeshes);
	classDef.def("AddMesh",&AddMesh);
	classDef.def("ClearMeshes",static_cast<void(*)(lua_State*,std::shared_ptr<::ModelMeshGroup>&)>([](lua_State *l,std::shared_ptr<::ModelMeshGroup> &meshGroup) {
		meshGroup->GetMeshes().clear();
	}));
	classDef.def("SetMeshes",static_cast<void(*)(lua_State*,std::shared_ptr<::ModelMeshGroup>&,luabind::object)>([](lua_State *l,std::shared_ptr<::ModelMeshGroup> &meshGroup,luabind::object tMeshes) {
		auto idxMeshes = 2;
		Lua::CheckTable(l,idxMeshes);
		auto &meshes = meshGroup->GetMeshes();
		meshes = {};
		auto numMeshes = Lua::GetObjectLength(l,idxMeshes);
		meshes.reserve(idxMeshes);
		for(auto i=decltype(numMeshes){0u};i<numMeshes;++i)
		{
			Lua::PushInt(l,i +1);
			Lua::GetTableValue(l,idxMeshes);
			auto &subMesh = Lua::Check<std::shared_ptr<::ModelMesh>>(l,-1);
			meshes.push_back(subMesh);
			Lua::Pop(l,1);
		}
	}));
}
void Lua::ModelMeshGroup::Create(lua_State *l,const std::string &name)
{
	Lua::Push<std::shared_ptr<::ModelMeshGroup>>(l,std::make_shared<::ModelMeshGroup>(name));
}
void Lua::ModelMeshGroup::GetName(lua_State *l,std::shared_ptr<::ModelMeshGroup> &meshGroup)
{
	Lua::PushString(l,meshGroup->GetName());
}
void Lua::ModelMeshGroup::GetMeshes(lua_State *l,std::shared_ptr<::ModelMeshGroup> &meshGroup)
{
	auto &meshes = meshGroup->GetMeshes();
	auto t = Lua::CreateTable(l);
	int32_t i = 1;
	for(auto &mesh : meshes)
	{
		Lua::PushInt(l,i);
		Lua::Push<std::shared_ptr<ModelMesh>>(l,mesh);
		Lua::SetTableValue(l,t);
		++i;
	}
}
void Lua::ModelMeshGroup::AddMesh(lua_State*,std::shared_ptr<::ModelMeshGroup> &meshGroup,std::shared_ptr<ModelMesh> &mesh)
{
	meshGroup->AddMesh(mesh);
}

//////////////////////////

void Lua::Joint::GetType(lua_State *l,JointInfo &joint)
{
	Lua::PushInt(l,joint.type);
}
void Lua::Joint::GetCollisionMeshId(lua_State *l,JointInfo &joint)
{
	Lua::PushInt(l,joint.src);
}
void Lua::Joint::GetParentCollisionMeshId(lua_State *l,JointInfo &joint)
{
	Lua::PushInt(l,joint.dest);
}
void Lua::Joint::GetCollisionsEnabled(lua_State *l,JointInfo &joint)
{
	Lua::PushBool(l,joint.collide);
}
void Lua::Joint::GetKeyValues(lua_State *l,JointInfo &joint)
{
	auto t = Lua::CreateTable(l);
	for(auto &pair : joint.args)
	{
		Lua::PushString(l,pair.first);
		Lua::PushString(l,pair.second);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Joint::SetType(lua_State *l,JointInfo &joint,uint32_t type) {joint.type = type;}
void Lua::Joint::SetCollisionMeshId(lua_State *l,JointInfo &joint,uint32_t meshId) {joint.src = meshId;}
void Lua::Joint::SetParentCollisionMeshId(lua_State *l,JointInfo &joint,uint32_t meshId) {joint.dest = meshId;}
void Lua::Joint::SetCollisionsEnabled(lua_State *l,JointInfo &joint,bool bEnabled) {joint.collide = bEnabled;}
void Lua::Joint::SetKeyValues(lua_State *l,JointInfo &joint,luabind::object keyValues)
{
	Lua::CheckTable(l,2);

	Lua::PushNil(l);
	joint.args.clear();
	while(Lua::GetNextPair(l,2) != 0)
	{
		auto *key = Lua::CheckString(l,-2);
		auto *val = Lua::CheckString(l,-1);
		joint.args[key] = val;
		Lua::Pop(l,1);
	}
}
void Lua::Joint::SetKeyValue(lua_State *l,JointInfo &joint,const std::string &key,const std::string &val) {joint.args[key] = val;}
void Lua::Joint::RemoveKeyValue(lua_State *l,JointInfo &joint,const std::string &key)
{
	auto it = joint.args.find(key);
	if(it == joint.args.end())
		return;
	joint.args.erase(it);
}

//////////////////////////

void Lua::Model::register_class(
	lua_State *l,
	luabind::class_<std::shared_ptr<::Model>> &classDef,
	luabind::class_<std::shared_ptr<::ModelMesh>> &classDefModelMesh,
	luabind::class_<std::shared_ptr<::ModelSubMesh>> &classDefModelSubMesh
)
{
	classDef.def(luabind::const_self == std::shared_ptr<::Model>());
	classDef.def("GetCollisionMeshes",&GetCollisionMeshes);
	classDef.def("ClearCollisionMeshes",&ClearCollisionMeshes);
	classDef.def("GetSkeleton",&GetSkeleton);
	classDef.def("GetAttachmentCount",&GetAttachmentCount);
	classDef.def("GetAttachments",&GetAttachments);
	classDef.def("GetAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&GetAttachment));
	classDef.def("GetAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,int32_t)>(&GetAttachment));
	classDef.def("LookupAttachment",&LookupAttachment);
	classDef.def("LookupBone",&LookupBone);
	classDef.def("LookupAnimation",&LookupAnimation);
	classDef.def("AddAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&,const std::string&,const Vector3&,const EulerAngles&)>(&AddAttachment));
	classDef.def("AddAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&,uint32_t,const Vector3&,const EulerAngles&)>(&AddAttachment));
	classDef.def("SetAttachmentData",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&,luabind::object)>(&SetAttachmentData));
	classDef.def("SetAttachmentData",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t,luabind::object)>(&SetAttachmentData));
	classDef.def("RemoveAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&RemoveAttachment));
	classDef.def("RemoveAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&RemoveAttachment));

	classDef.def("GetObjectAttachments",&GetObjectAttachments);
	classDef.def("AddObjectAttachment",&AddObjectAttachment);
	classDef.def("GetObjectAttachmentCount",&GetObjectAttachmentCount);
	classDef.def("GetObjectAttachment",&GetObjectAttachment);
	classDef.def("LookupObjectAttachment",&LookupObjectAttachment);
	classDef.def("RemoveObjectAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&RemoveObjectAttachment));
	classDef.def("RemoveObjectAttachment",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&RemoveObjectAttachment));

	classDef.def("GetBlendControllerCount",&GetBlendControllerCount);
	classDef.def("GetBlendControllers",&GetBlendControllers);
	classDef.def("GetBlendController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&GetBlendController));
	classDef.def("GetBlendController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,int32_t)>(&GetBlendController));
	classDef.def("LookupBlendController",&LookupBlendController);
	classDef.def("GetAnimationCount",&GetAnimationCount);
	classDef.def("GetAnimations",&GetAnimations);
	classDef.def("GetAnimationNames",&GetAnimationNames);
	classDef.def("GetAnimation",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const char*)>(&GetAnimation));
	classDef.def("GetAnimation",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,unsigned int)>(&GetAnimation));
	classDef.def("GetAnimationName",&GetAnimationName);

	classDef.def("PrecacheTextureGroup",&PrecacheTextureGroup);
	classDef.def("PrecacheTextureGroups",&PrecacheTextureGroups);
	classDef.def("GetReferencePose",&GetReference);
	//classDef.def("GetReferenceBoneMatrix",&GetReferenceBoneMatrix);
	//classDef.def("SetReferenceBoneMatrix",&SetReferenceBoneMatrix);
	classDef.def("GetLocalBoneTransform",&GetLocalBonePosition);
	classDef.def("LookupBodyGroup",&LookupBodyGroup);
	classDef.def("GetBaseMeshGroupIds",&GetBaseMeshGroupIds);
	classDef.def("SetBaseMeshGroupIds",&SetBaseMeshGroupIds);
	classDef.def("AddBaseMeshGroupId",&AddBaseMeshGroupId);
	classDef.def("GetMeshGroupId",&GetMeshGroupId);
	classDef.def("GetMeshGroup",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&GetMeshGroup));
	classDef.def("GetMeshGroup",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&GetMeshGroup));
	classDef.def("GetMeshes",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&Lua::Model::GetMeshes));
	classDef.def("GetMeshes",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,luabind::object)>(&Lua::Model::GetMeshes));
	classDef.def("GetMeshGroups",&Lua::Model::GetMeshGroups);
	classDef.def("AddMeshGroup",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&Lua::Model::AddMeshGroup));
	classDef.def("AddMeshGroup",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,std::shared_ptr<::ModelMeshGroup>&)>(&Lua::Model::AddMeshGroup));
	classDef.def("UpdateCollisionBounds",&Lua::Model::UpdateCollisionBounds);
	classDef.def("UpdateRenderBounds",&Lua::Model::UpdateRenderBounds);
	classDef.def("Update",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&)>(&Lua::Model::Update));
	classDef.def("Update",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::Update));
	classDef.def("GetName",&Lua::Model::GetName);
	classDef.def("GetMass",&Lua::Model::GetMass);
	classDef.def("SetMass",&Lua::Model::SetMass);
	classDef.def("GetBoneCount",&Lua::Model::GetBoneCount);
	classDef.def("GetCollisionBounds",&Lua::Model::GetCollisionBounds);
	classDef.def("GetRenderBounds",&Lua::Model::GetRenderBounds);
	classDef.def("SetCollisionBounds",&Lua::Model::SetCollisionBounds);
	classDef.def("SetRenderBounds",&Lua::Model::SetRenderBounds);
	classDef.def("AddCollisionMesh",&Lua::Model::AddCollisionMesh);
	classDef.def("AddMaterial",&Lua::Model::AddMaterial);
	classDef.def("SetMaterial",&Lua::Model::SetMaterial);
	classDef.def("GetMaterials",&Lua::Model::GetMaterials);
	classDef.def("GetMaterialCount",&Lua::Model::GetMaterialCount);
	classDef.def("GetMeshGroupCount",&Lua::Model::GetMeshGroupCount);
	classDef.def("GetMeshCount",&Lua::Model::GetMeshCount);
	classDef.def("GetSubMeshCount",&Lua::Model::GetSubMeshCount);
	classDef.def("GetCollisionMeshCount",&Lua::Model::GetCollisionMeshCount);
	classDef.def("GetBodyGroupId",&Lua::Model::GetBodyGroupId);
	classDef.def("GetBodyGroupCount",&Lua::Model::GetBodyGroupCount);
	classDef.def("AddHitbox",&Lua::Model::AddHitbox);
	classDef.def("GetHitboxCount",&Lua::Model::GetHitboxCount);
	classDef.def("GetHitboxGroup",&Lua::Model::GetHitboxGroup);
	classDef.def("GetHitboxBounds",&Lua::Model::GetHitboxBounds);
	classDef.def("GetHitboxBones",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::GetHitboxBones));
	classDef.def("GetHitboxBones",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&)>(&Lua::Model::GetHitboxBones));
	classDef.def("SetHitboxGroup",&Lua::Model::SetHitboxGroup);
	classDef.def("SetHitboxBounds",&Lua::Model::SetHitboxBounds);
	classDef.def("RemoveHitbox",&Lua::Model::RemoveHitbox);
	classDef.def("GetBodyGroups",&Lua::Model::GetBodyGroups);
	classDef.def("GetBodyGroup",&Lua::Model::GetBodyGroup);

	classDef.def("GetTextureGroupCount",&Lua::Model::GetTextureGroupCount);
	classDef.def("GetTextureGroups",&Lua::Model::GetTextureGroups);
	classDef.def("GetTextureGroup",&Lua::Model::GetTextureGroup);
	classDef.def("Save",&Lua::Model::Save);
	classDef.def("Copy",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&)>(&Lua::Model::Copy));
	classDef.def("Copy",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::Copy));
	classDef.def("GetVertexCount",&Lua::Model::GetVertexCount);
	classDef.def("GetTriangleCount",&Lua::Model::GetTriangleCount);
	classDef.def("GetMaterialNames",&Lua::Model::GetTextures);
	classDef.def("GetMaterialPaths",&Lua::Model::GetTexturePaths);
	classDef.def("LoadMaterials",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&)>(&Lua::Model::LoadMaterials));
	classDef.def("LoadMaterials",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,bool)>(&Lua::Model::LoadMaterials));
	classDef.def("AddMaterialPath",&Lua::Model::AddTexturePath);
	classDef.def("RemoveMaterialPath",&Lua::Model::RemoveTexturePath);
	classDef.def("SetMaterialPaths",&Lua::Model::SetTexturePaths);
	classDef.def("RemoveMaterial",&Lua::Model::RemoveTexture);
	classDef.def("ClearMaterials",&Lua::Model::ClearTextures);
	classDef.def("Rotate",&Lua::Model::Rotate);
	classDef.def("Translate",&Lua::Model::Translate);
	classDef.def("GetEyeOffset",&Lua::Model::GetEyeOffset);
	classDef.def("SetEyeOffset",&Lua::Model::SetEyeOffset);
	classDef.def("AddAnimation",&Lua::Model::AddAnimation);
	classDef.def("RemoveAnimation",&Lua::Model::RemoveAnimation);
	classDef.def("ClearAnimations",&Lua::Model::ClearAnimations);
	//classDef.def("ClipAgainstPlane",&Lua::Model::ClipAgainstPlane);
	classDef.def("ClearMeshGroups",&Lua::Model::ClearMeshGroups);
	classDef.def("RemoveMeshGroup",&Lua::Model::RemoveMeshGroup);
	classDef.def("ClearBaseMeshGroupIds",&Lua::Model::ClearBaseMeshGroupIds);
	classDef.def("AddTextureGroup",&Lua::Model::AddTextureGroup);
	classDef.def("Merge",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&,std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::Merge));
	classDef.def("Merge",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&,std::shared_ptr<::Model>&)>(&Lua::Model::Merge));
	classDef.def("GetLODCount",&Lua::Model::GetLODCount);
	classDef.def("GetLODData",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::GetLODData));
	classDef.def("GetLODData",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&)>(&Lua::Model::GetLODData));
	classDef.def("GetLOD",&Lua::Model::GetLOD);
	classDef.def("TranslateLODMeshes",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&,uint32_t,luabind::object)>(&Lua::Model::TranslateLODMeshes));
	classDef.def("TranslateLODMeshes",static_cast<void(*)(lua_State*,std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::TranslateLODMeshes));
	classDef.def("GetJoints",&Lua::Model::GetJoints);
	classDef.def("GetVertexAnimations",&Lua::Model::GetVertexAnimations);
	classDef.def("GetVertexAnimation",&Lua::Model::GetVertexAnimation);
	classDef.def("AddVertexAnimation",&Lua::Model::AddVertexAnimation);
	classDef.def("RemoveVertexAnimation",&Lua::Model::RemoveVertexAnimation);
	classDef.def("GetFlexControllers",&Lua::Model::GetFlexControllers);
	classDef.def("LookupFlexController",&Lua::Model::GetFlexControllerId);
	classDef.def("GetFlexController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&Lua::Model::GetFlexController));
	classDef.def("GetFlexController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::GetFlexController));
	classDef.def("GetFlexes",&Lua::Model::GetFlexes);
	classDef.def("LookupFlex",&Lua::Model::GetFlexId);
	classDef.def("GetFlexFormula",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&Lua::Model::GetFlexFormula));
	classDef.def("GetFlexFormula",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::GetFlexFormula));

	classDef.def("GetIKControllers",&Lua::Model::GetIKControllers);
	classDef.def("GetIKController",&Lua::Model::GetIKController);
	classDef.def("LookupIKController",&Lua::Model::LookupIKController);
	classDef.def("AddIKController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&,uint32_t,const std::string&,uint32_t)>(&Lua::Model::AddIKController));
	classDef.def("AddIKController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&,uint32_t,const std::string&)>(&Lua::Model::AddIKController));
	classDef.def("RemoveIKController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,uint32_t)>(&Lua::Model::RemoveIKController));
	classDef.def("RemoveIKController",static_cast<void(*)(lua_State*,const std::shared_ptr<::Model>&,const std::string&)>(&Lua::Model::RemoveIKController));

	classDef.add_static_constant("FMERGE_NONE",umath::to_integral(::Model::MergeFlags::None));
	classDef.add_static_constant("FMERGE_ANIMATIONS",umath::to_integral(::Model::MergeFlags::Animations));
	classDef.add_static_constant("FMERGE_ATTACHMENTS",umath::to_integral(::Model::MergeFlags::Attachments));
	classDef.add_static_constant("FMERGE_BLEND_CONTROLLERS",umath::to_integral(::Model::MergeFlags::BlendControllers));
	classDef.add_static_constant("FMERGE_HITBOXES",umath::to_integral(::Model::MergeFlags::Hitboxes));
	classDef.add_static_constant("FMERGE_JOINTS",umath::to_integral(::Model::MergeFlags::Joints));
	classDef.add_static_constant("FMERGE_COLLISION_MESHES",umath::to_integral(::Model::MergeFlags::CollisionMeshes));
	classDef.add_static_constant("FMERGE_MESHES",umath::to_integral(::Model::MergeFlags::Meshes));
	classDef.add_static_constant("FMERGE_ALL",umath::to_integral(::Model::MergeFlags::All));

	classDef.add_static_constant("FCOPY_NONE",umath::to_integral(::Model::CopyFlags::None));
	classDef.add_static_constant("FCOPY_SHALLOW",umath::to_integral(::Model::CopyFlags::ShallowCopy));
	classDef.add_static_constant("FCOPY_BIT_MESHES",umath::to_integral(::Model::CopyFlags::CopyMeshesBit));
	classDef.add_static_constant("FCOPY_BIT_ANIMATIONS",umath::to_integral(::Model::CopyFlags::CopyAnimationsBit));
	classDef.add_static_constant("FCOPY_BIT_VERTEX_ANIMATIONS",umath::to_integral(::Model::CopyFlags::CopyVertexAnimationsBit));
	classDef.add_static_constant("FCOPY_BIT_COLLISION_MESHES",umath::to_integral(::Model::CopyFlags::CopyCollisionMeshes));
	classDef.add_static_constant("FCOPY_DEEP",umath::to_integral(::Model::CopyFlags::DeepCopy));

	classDef.add_static_constant("FUPDATE_NONE",umath::to_integral(ModelUpdateFlags::None));
	classDef.add_static_constant("FUPDATE_BOUNDS",umath::to_integral(ModelUpdateFlags::UpdateBounds));
	classDef.add_static_constant("FUPDATE_PRIMITIVE_COUNTS",umath::to_integral(ModelUpdateFlags::UpdatePrimitiveCounts));
	classDef.add_static_constant("FUPDATE_COLLISION_SHAPES",umath::to_integral(ModelUpdateFlags::UpdateCollisionShapes));
	classDef.add_static_constant("FUPDATE_TANGENTS",umath::to_integral(ModelUpdateFlags::UpdateTangents));
	classDef.add_static_constant("FUPDATE_VERTEX_BUFFER",umath::to_integral(ModelUpdateFlags::UpdateVertexBuffer));
	classDef.add_static_constant("FUPDATE_INDEX_BUFFER",umath::to_integral(ModelUpdateFlags::UpdateIndexBuffer));
	classDef.add_static_constant("FUPDATE_WEIGHT_BUFFER",umath::to_integral(ModelUpdateFlags::UpdateWeightBuffer));
	classDef.add_static_constant("FUPDATE_ALPHA_BUFFER",umath::to_integral(ModelUpdateFlags::UpdateAlphaBuffer));
	classDef.add_static_constant("FUPDATE_VERTEX_ANIMATION_BUFFER",umath::to_integral(ModelUpdateFlags::UpdateVertexAnimationBuffer));
	classDef.add_static_constant("FUPDATE_CHILDREN",umath::to_integral(ModelUpdateFlags::UpdateChildren));
	classDef.add_static_constant("FUPDATE_BUFFERS",umath::to_integral(ModelUpdateFlags::UpdateBuffers));
	classDef.add_static_constant("FUPDATE_ALL",umath::to_integral(ModelUpdateFlags::All));
	classDef.add_static_constant("FUPDATE_ALL_DATA",umath::to_integral(ModelUpdateFlags::AllData));

	classDef.add_static_constant("OBJECT_ATTACHMENT_TYPE_MODEL",umath::to_integral(ObjectAttachment::Type::Model));
	classDef.add_static_constant("OBJECT_ATTACHMENT_TYPE_PARTICLE_SYSTEM",umath::to_integral(ObjectAttachment::Type::ParticleSystem));

	// Frame
	auto classDefFrame = luabind::class_<std::shared_ptr<::Frame>>("Frame")
		.def("GetBoneMatrix",&Lua::Frame::GetBoneMatrix)
		.def("GetBoneTransform",&Lua::Frame::GetBonePosition)
		.def("GetBoneRotation",&Lua::Frame::GetBoneOrientation)
		.def("SetBonePosition",&Lua::Frame::SetBonePosition)
		.def("SetBoneRotation",&Lua::Frame::SetBoneOrientation)
		.def("Localize",&Lua::Frame::Localize)
		.def("Globalize",&Lua::Frame::Globalize)
		.def("CalcRenderBounds",&Lua::Frame::CalcRenderBounds)
		.def("Rotate",&Lua::Frame::Rotate)
		.def("Translate",&Lua::Frame::Translate)
		.def("GetMoveTranslation",&Lua::Frame::GetMoveTranslation)
		.def("GetMoveTranslationX",&Lua::Frame::GetMoveTranslationX)
		.def("GetMoveTranslationZ",&Lua::Frame::GetMoveTranslationZ)
		.def("SetMoveTranslation",&Lua::Frame::SetMoveTranslation)
		.def("SetMoveTranslationX",&Lua::Frame::SetMoveTranslationX)
		.def("SetMoveTranslationZ",&Lua::Frame::SetMoveTranslationZ)
		.def("SetBoneScale",&Lua::Frame::SetBoneScale)
		.def("GetBoneScale",&Lua::Frame::GetBoneScale)
		.def("SetBoneTransform",static_cast<void(*)(lua_State*,const std::shared_ptr<::Frame>&,unsigned int,const Vector3&,const Quat&,const Vector3&)>(&Lua::Frame::SetBoneTransform))
		.def("SetBoneTransform",static_cast<void(*)(lua_State*,const std::shared_ptr<::Frame>&,unsigned int,const Vector3&,const Quat&)>(&Lua::Frame::SetBoneTransform))
		.def("GetLocalBoneTransform",&Lua::Frame::GetLocalBoneTransform)
		.def("GetBoneCount",&Lua::Frame::GetBoneCount)
		.def("SetBoneCount",&Lua::Frame::SetBoneCount)
	;
	classDefFrame.scope[luabind::def("Create",&Lua::Frame::Create)];

	// Animation
	auto classDefAnimation = luabind::class_<std::shared_ptr<::Animation>>("Animation")
		.def("GetFrame",&Lua::Animation::GetFrame)
		.def("GetBoneList",&Lua::Animation::GetBoneList)
		.def("GetActivity",&Lua::Animation::GetActivity)
		.def("SetActivity",&Lua::Animation::SetActivity)
		.def("GetActivityWeight",&Lua::Animation::GetActivityWeight)
		.def("SetActivityWeight",&Lua::Animation::SetActivityWeight)
		.def("GetFPS",&Lua::Animation::GetFPS)
		.def("SetFPS",&Lua::Animation::SetFPS)
		.def("GetFlags",&Lua::Animation::GetFlags)
		.def("SetFlags",&Lua::Animation::SetFlags)
		.def("AddFlags",&Lua::Animation::AddFlags)
		.def("RemoveFlags",&Lua::Animation::RemoveFlags)
		.def("AddFrame",&Lua::Animation::AddFrame)
		.def("GetFrames",&Lua::Animation::GetFrames)
		.def("GetDuration",&Lua::Animation::GetDuration)
		.def("GetBoneCount",&Lua::Animation::GetBoneCount)
		.def("GetFrameCount",&Lua::Animation::GetFrameCount)
		.def("AddEvent",&Lua::Animation::AddEvent)
		.def("GetEvents",static_cast<void(*)(lua_State*,std::shared_ptr<::Animation>&,uint32_t)>(&Lua::Animation::GetEvents))
		.def("GetEvents",static_cast<void(*)(lua_State*,std::shared_ptr<::Animation>&)>(&Lua::Animation::GetEvents))
		.def("GetEventCount",static_cast<void(*)(lua_State*,std::shared_ptr<::Animation>&,uint32_t)>(&Lua::Animation::GetEventCount))
		.def("GetEventCount",static_cast<void(*)(lua_State*,std::shared_ptr<::Animation>&)>(&Lua::Animation::GetEventCount))
		.def("GetFadeInTime",&Lua::Animation::GetFadeInTime)
		.def("GetFadeOutTime",&Lua::Animation::GetFadeOutTime)
		.def("GetBlendController",&Lua::Animation::GetBlendController)
		.def("CalcRenderBounds",&Lua::Animation::CalcRenderBounds)
		.def("GetRenderBounds",&Lua::Animation::GetRenderBounds)
		.def("Rotate",&Lua::Animation::Rotate)
		.def("Translate",&Lua::Animation::Translate)
		.def("Reverse",&Lua::Animation::Reverse)
		.def("RemoveEvent",&Lua::Animation::RemoveEvent)
		.def("SetEventData",&Lua::Animation::SetEventData)
		.def("SetEventType",&Lua::Animation::SetEventType)
		.def("SetEventArgs",&Lua::Animation::SetEventArgs)
		.def("LookupBone",&Lua::Animation::LookupBone)
		.def("SetBoneList",&Lua::Animation::SetBoneList)
		.def("AddBoneId",&Lua::Animation::AddBoneId)
		.def("SetFadeInTime",&Lua::Animation::SetFadeInTime)
		.def("SetFadeOutTime",&Lua::Animation::SetFadeOutTime)
		.def("SetBoneWeight",&Lua::Animation::SetBoneWeight)
		.def("GetBoneWeight",&Lua::Animation::GetBoneWeight)
		.def("GetBoneWeights",&Lua::Animation::GetBoneWeights);
	classDefAnimation.scope[
		luabind::def("Create",&Lua::Animation::Create),
		luabind::def("RegisterActivity",&Lua::Animation::RegisterActivityEnum),
		luabind::def("RegisterEvent",&Lua::Animation::RegisterEventEnum),
		luabind::def("GetActivityEnums",&Lua::Animation::GetActivityEnums),
		luabind::def("GetEventEnums",&Lua::Animation::GetEventEnums),
		luabind::def("GetActivityEnumName",&Lua::Animation::GetActivityEnumName),
		luabind::def("GetEventEnumName",&Lua::Animation::GetEventEnumName),
		classDefFrame
	];
	//for(auto &pair : ANIMATION_EVENT_NAMES)
	//	classDefAnimation.add_static_constant(pair.second.c_str(),pair.first);

	classDefAnimation.add_static_constant("FLAG_LOOP",umath::to_integral(FAnim::Loop));
	classDefAnimation.add_static_constant("FLAG_NOREPEAT",umath::to_integral(FAnim::NoRepeat));
	classDefAnimation.add_static_constant("FLAG_MOVEX",umath::to_integral(FAnim::MoveX));
	classDefAnimation.add_static_constant("FLAG_MOVEZ",umath::to_integral(FAnim::MoveZ));
	classDefAnimation.add_static_constant("FLAG_AUTOPLAY",umath::to_integral(FAnim::Autoplay));
	classDefAnimation.add_static_constant("FLAG_GESTURE",umath::to_integral(FAnim::Gesture));

	//for(auto &pair : ACTIVITY_NAMES)
	//	classDefAnimation.add_static_constant(pair.second.c_str(),pair.first);

	// Vertex Animation
	auto classDefVertexAnimation = luabind::class_<std::shared_ptr<::VertexAnimation>>("VertexAnimation")
		.def("GetMeshAnimations",&Lua::VertexAnimation::GetMeshAnimations)
		.def("GetName",&Lua::VertexAnimation::GetName);

	auto classDefMeshVertexFrame = luabind::class_<std::shared_ptr<::MeshVertexFrame>>("Frame")
		.def("GetVertices",&Lua::MeshVertexFrame::GetVertices)
		.def("SetVertexCount",&Lua::MeshVertexFrame::SetVertexCount)
		.def("SetVertexPosition",&Lua::MeshVertexFrame::SetVertexPosition)
		.def("GetVertexPosition",&Lua::MeshVertexFrame::GetVertexPosition);
	auto classDefMeshVertexAnimation = luabind::class_<std::shared_ptr<::MeshVertexAnimation>>("MeshAnimation")
		.def("GetFrames",&Lua::MeshVertexAnimation::GetFrames)
		.def("GetMesh",&Lua::MeshVertexAnimation::GetMesh);
	classDefMeshVertexAnimation.scope[
		classDefMeshVertexFrame
	];
	classDefVertexAnimation.scope[
		classDefMeshVertexAnimation
	];

	auto classDefSkeleton = luabind::class_<::Skeleton>("Skeleton");
	classDefSkeleton.def("GetBone",&Lua::Skeleton::GetBone);
	classDefSkeleton.def("GetRootBones",&Lua::Skeleton::GetRootBones);
	classDefSkeleton.def("GetBones",&Lua::Skeleton::GetBones);
	classDefSkeleton.def("LookupBone",&Lua::Skeleton::LookupBone);
	classDefSkeleton.def("AddBone",static_cast<void(*)(lua_State*,::Skeleton&,const std::string&,std::shared_ptr<::Bone>&)>(&Lua::Skeleton::AddBone));
	classDefSkeleton.def("AddBone",static_cast<void(*)(lua_State*,::Skeleton&,const std::string&)>(&Lua::Skeleton::AddBone));
	classDefSkeleton.def("GetBoneCount",&Lua::Skeleton::GetBoneCount);
	classDefSkeleton.def("Merge",&Lua::Skeleton::Merge);
	classDefSkeleton.def("ClearBones",&Lua::Skeleton::ClearBones);
	classDefSkeleton.def("MakeRootBone",Lua::Skeleton::MakeRootBone);
	Lua::Bone::register_class(l,classDefSkeleton);

	auto modelMeshGroupClassDef = luabind::class_<std::shared_ptr<::ModelMeshGroup>>("MeshGroup");
	Lua::ModelMeshGroup::register_class(modelMeshGroupClassDef);

	auto collisionMeshClassDef = luabind::class_<std::shared_ptr<::CollisionMesh>>("CollisionMesh");
	Lua::CollisionMesh::register_class(collisionMeshClassDef);

	// Vertex
	auto defVertex = luabind::class_<::Vertex>("Vertex");
	defVertex.def(luabind::constructor<const Vector3&,const ::Vector2&,const Vector3&,const Vector3&,const Vector3&>());
	defVertex.def(luabind::constructor<const Vector3&,const ::Vector2&,const Vector3&>());
	defVertex.def(luabind::constructor<const Vector3&,const Vector3&>());
	defVertex.def(luabind::constructor<>());
	defVertex.def(luabind::tostring(luabind::self));
	defVertex.def(luabind::const_self ==::Vertex());
	defVertex.def_readwrite("position",&::Vertex::position);
	defVertex.def_readwrite("uv",&::Vertex::uv);
	defVertex.def_readwrite("normal",&::Vertex::normal);
	defVertex.def_readwrite("tangent",&::Vertex::tangent);
	defVertex.def_readwrite("bitangent",&::Vertex::biTangent);
	defVertex.def("Copy",&Lua::Vertex::Copy);
	classDef.scope[defVertex];

	auto defVertWeight = luabind::class_<::VertexWeight>("VertexWeight");
	defVertWeight.def(luabind::constructor<const ::Vector4i&,const ::Vector4&>());
	defVertWeight.def(luabind::constructor<>());
	defVertWeight.def(luabind::tostring(luabind::self));
	defVertWeight.def(luabind::const_self ==::VertexWeight());
	defVertWeight.def_readwrite("boneIds",&::VertexWeight::boneIds);
	defVertWeight.def_readwrite("weights",&::VertexWeight::weights);
	defVertWeight.def("Copy",&Lua::VertexWeight::Copy);
	classDef.scope[defVertWeight];

	// Joint
	auto defJoint = luabind::class_<JointInfo>("Joint");
	defJoint.def("GetType",&Lua::Joint::GetType);
	defJoint.def("GetCollisionMeshId",&Lua::Joint::GetCollisionMeshId);
	defJoint.def("GetParentCollisionMeshId",&Lua::Joint::GetParentCollisionMeshId);
	defJoint.def("GetCollisionsEnabled",&Lua::Joint::GetCollisionsEnabled);
	defJoint.def("GetKeyValues",&Lua::Joint::GetKeyValues);

	defJoint.def("SetType",&Lua::Joint::SetType);
	defJoint.def("SetCollisionMeshId",&Lua::Joint::SetCollisionMeshId);
	defJoint.def("SetParentCollisionMeshId",&Lua::Joint::SetParentCollisionMeshId);
	defJoint.def("SetCollisionsEnabled",&Lua::Joint::SetCollisionsEnabled);
	defJoint.def("SetKeyValues",&Lua::Joint::SetKeyValues);
	defJoint.def("SetKeyValue",&Lua::Joint::SetKeyValue);
	defJoint.def("RemoveKeyValue",&Lua::Joint::RemoveKeyValue);

	defJoint.add_static_constant("TYPE_NONE",JOINT_TYPE_NONE);
	defJoint.add_static_constant("TYPE_FIXED",JOINT_TYPE_FIXED);
	defJoint.add_static_constant("TYPE_BALLSOCKET",JOINT_TYPE_BALLSOCKET);
	defJoint.add_static_constant("TYPE_HINGE",JOINT_TYPE_HINGE);
	defJoint.add_static_constant("TYPE_SLIDER",JOINT_TYPE_SLIDER);
	defJoint.add_static_constant("TYPE_CONETWIST",JOINT_TYPE_CONETWIST);
	defJoint.add_static_constant("TYPE_DOF",JOINT_TYPE_DOF);
	classDef.scope[defJoint];

	// Assign definitions
	classDef.scope[classDefSkeleton];
	classDef.scope[modelMeshGroupClassDef];
	classDef.scope[collisionMeshClassDef];
	classDef.scope[classDefAnimation];
	classDef.scope[classDefVertexAnimation];
	classDefModelMesh.scope[classDefModelSubMesh];
	classDef.scope[classDefModelMesh];
}

void Lua::Model::GetCollisionMeshes(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &meshes = mdl->GetCollisionMeshes();
	lua_newtable(l);
	int top = lua_gettop(l);
	for(auto i=decltype(meshes.size()){0};i<meshes.size();++i)
	{
		Lua::Push<std::shared_ptr<::CollisionMesh>>(l,meshes[i]);
		lua_rawseti(l,top,i +1);
	}
}

void Lua::Model::ClearCollisionMeshes(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->GetCollisionMeshes().clear();
}

void Lua::Model::GetSkeleton(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &skeleton = mdl->GetSkeleton();
	luabind::object(l,&skeleton).push(l);
}

static void push_attachment(lua_State *l,const Attachment &att)
{
	auto tAtt = Lua::CreateTable(l);

	Lua::PushString(l,"angles");
	Lua::Push<EulerAngles>(l,att.angles);
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"bone");
	Lua::PushInt(l,att.bone);
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"name");
	Lua::PushString(l,att.name);
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"offset");
	Lua::Push<Vector3>(l,att.offset);
	Lua::SetTableValue(l,tAtt);
}
void Lua::Model::GetAttachmentCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetAttachments().size());
}
void Lua::Model::GetAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,int32_t attId)
{
	//Lua::CheckModel(l,1);
	auto *att = mdl->GetAttachment(attId);
	if(att == nullptr)
		return;
	push_attachment(l,*att);
}
void Lua::Model::GetAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto *att = mdl->GetAttachment(name);
	if(att == nullptr)
		return;
	push_attachment(l,*att);
}
void Lua::Model::GetAttachments(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &attachments = mdl->GetAttachments();
	auto t = Lua::CreateTable(l);
	for(auto i=decltype(attachments.size()){0};i<attachments.size();++i)
	{
		auto &att = attachments[i];
		att.angles;
		att.bone;
		att.name;
		att.offset;

		Lua::PushInt(l,i +1);
		push_attachment(l,att);

		Lua::SetTableValue(l,t);
	}
}

static void push_blend_controller(lua_State *l,const BlendController &blendController)
{
	auto tController = Lua::CreateTable(l);
	
	Lua::PushString(l,"loop");
	Lua::PushBool(l,blendController.loop);
	Lua::SetTableValue(l,tController);

	Lua::PushString(l,"min");
	Lua::PushInt(l,blendController.min);
	Lua::SetTableValue(l,tController);

	Lua::PushString(l,"max");
	Lua::PushInt(l,blendController.max);
	Lua::SetTableValue(l,tController);

	Lua::PushString(l,"name");
	Lua::PushString(l,blendController.name);
	Lua::SetTableValue(l,tController);
}
void Lua::Model::LookupAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto id = mdl->LookupAnimation(name);
	Lua::PushInt(l,id);
}
void Lua::Model::LookupAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto attId = mdl->LookupAttachment(name);
	Lua::PushInt(l,attId);
}
void Lua::Model::LookupBone(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto boneId = mdl->LookupBone(name);
	Lua::PushInt(l,boneId);
}
void Lua::Model::AddAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,const std::string &boneName,const Vector3 &offset,const EulerAngles &ang)
{
	//Lua::CheckModel(l,1);
	auto boneId = mdl->LookupBone(boneName);
	if(boneId < 0)
		return;
	AddAttachment(l,mdl,name,boneId,offset,ang);
}
void Lua::Model::AddAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,uint32_t boneId,const Vector3 &offset,const EulerAngles &ang)
{
	//Lua::CheckModel(l,1);
	mdl->AddAttachment(name,boneId,offset,ang);
}
struct LuaAttachmentData
{
	EulerAngles *angles = nullptr;
	const char *bone = nullptr;
	const char *name = nullptr;
	Vector3 *offset = nullptr;
};
static void get_attachment(lua_State *l,LuaAttachmentData &att,int32_t t)
{
	Lua::PushString(l,"angles");
	Lua::GetTableValue(l,t);
	if(Lua::IsNil(l,-1) == false)
		att.angles = Lua::CheckEulerAngles(l,-1);
	Lua::Pop(l,1);

	Lua::PushString(l,"bone");
	Lua::GetTableValue(l,t);
	if(Lua::IsNil(l,-1) == false)
		att.bone = Lua::CheckString(l,-1);
	Lua::Pop(l,1);

	Lua::PushString(l,"name");
	Lua::GetTableValue(l,t);
	if(Lua::IsNil(l,-1) == false)
		att.name = Lua::CheckString(l,-1);
	Lua::Pop(l,1);

	Lua::PushString(l,"offset");
	Lua::GetTableValue(l,t);
	if(Lua::IsNil(l,-1) == false)
		att.offset = Lua::CheckVector(l,-1);
	Lua::Pop(l,1);
}
void Lua::Model::SetAttachmentData(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,luabind::object data)
{
	//Lua::CheckModel(l,1);
	int32_t t = 3;
	Lua::CheckTable(l,t);
	LuaAttachmentData attNew {};
	get_attachment(l,attNew,t);
	auto *att = mdl->GetAttachment(name);
	if(att == nullptr)
		return;
	if(attNew.angles != nullptr)
		att->angles = *attNew.angles;
	if(attNew.bone != nullptr)
		att->bone = mdl->LookupBone(attNew.bone);
	if(attNew.name != nullptr)
		att->name = attNew.name;
	if(attNew.offset != nullptr)
		att->offset = *attNew.offset;
}
void Lua::Model::SetAttachmentData(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t attId,luabind::object data)
{
	//Lua::CheckModel(l,1);
	int32_t t = 3;
	Lua::CheckTable(l,t);
	LuaAttachmentData attNew {};
	get_attachment(l,attNew,t);
	auto *att = mdl->GetAttachment(attId);
	if(att == nullptr)
		return;
	if(attNew.angles != nullptr)
		att->angles = *attNew.angles;
	if(attNew.bone != nullptr)
		att->bone = mdl->LookupBone(attNew.bone);
	if(attNew.name != nullptr)
		att->name = attNew.name;
	if(attNew.offset != nullptr)
		att->offset = *attNew.offset;
}
void Lua::Model::RemoveAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveAttachment(name);
}
void Lua::Model::RemoveAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t attId)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveAttachment(attId);
}
void Lua::Model::GetBlendControllerCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetBlendControllers().size());
}
void Lua::Model::GetBlendController(lua_State *l,const std::shared_ptr<::Model> &mdl,int32_t blendControllerId)
{
	//Lua::CheckModel(l,1);
	auto *blendController = mdl->GetBlendController(blendControllerId);
	if(blendController == nullptr)
		return;
	push_blend_controller(l,*blendController);
}
void Lua::Model::GetBlendController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto *blendController = mdl->GetBlendController(name);
	if(blendController == nullptr)
		return;
	push_blend_controller(l,*blendController);
}
void Lua::Model::GetBlendControllers(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto t = Lua::CreateTable(l);
	auto &blendControllers = mdl->GetBlendControllers();
	for(auto i=decltype(blendControllers.size()){0};i<blendControllers.size();++i)
	{
		auto &blendController = blendControllers[i];

		Lua::PushInt(l,i +1);
		push_blend_controller(l,blendController);

		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::LookupBlendController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->LookupBlendController(name));
}

void Lua::Model::GetAnimationCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	std::unordered_map<std::string,uint32_t> *anims;
	mdl->GetAnimations(&anims);
	Lua::PushInt(l,anims->size());
}

void Lua::Model::GetAnimationNames(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	std::unordered_map<std::string,unsigned int> *anims;
	mdl->GetAnimations(&anims);
	std::unordered_map<std::string,unsigned int>::iterator i;
	lua_newtable(l);
	int top = lua_gettop(l);
	int n = 1;
	for(i=anims->begin();i!=anims->end();i++)
	{
		lua_pushstring(l,i->first.c_str());
		lua_rawseti(l,top,n);
		n++;
	}
}

void Lua::Model::GetAnimations(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto t = Lua::CreateTable(l);
	auto &anims = mdl->GetAnimations();
	int32_t idx = 1;
	for(auto &anim : anims)
	{
		Lua::PushInt(l,idx++);
		Lua::Push<std::shared_ptr<::Animation>>(l,anim);
		Lua::SetTableValue(l,t);
	}
}

void Lua::Model::GetAnimationName(lua_State *l,const std::shared_ptr<::Model> &mdl,unsigned int animID)
{
	//Lua::CheckModel(l,1);
	std::string name;
	if(mdl->GetAnimationName(animID,name) == false)
		return;
	Lua::PushString(l,name);
}

void Lua::Model::GetAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,unsigned int animID)
{
	//Lua::CheckModel(l,1);
	auto anim = mdl->GetAnimation(animID);
	if(anim == nullptr)
		return;
	Lua::Push<std::shared_ptr<::Animation>>(l,anim);
}

void Lua::Model::GetAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const char *name)
{
	//Lua::CheckModel(l,1);
	int animID = mdl->LookupAnimation(name);
	if(animID == -1)
		return;
	GetAnimation(l,mdl,animID);
}

void Lua::Model::PrecacheTextureGroup(lua_State*,const std::shared_ptr<::Model> &mdl,unsigned int group)
{
	//Lua::CheckModel(l,1);
	mdl->PrecacheTextureGroup(group);
}

void Lua::Model::PrecacheTextureGroups(lua_State*,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->PrecacheTextureGroups();
}

void Lua::Model::GetReference(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &ref = mdl->GetReference();
	Lua::Push<std::shared_ptr<::Frame>>(l,ref.shared_from_this());
}
/*void Lua::Model::GetReferenceBoneMatrix(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId)
{
	//Lua::CheckModel(l,1);
	auto *mat = mdl->GetBindPoseBoneMatrix(boneId);
	if(mat == nullptr)
		return;
	Lua::Push<Mat4>(l,*mat);
}
void Lua::Model::SetReferenceBoneMatrix(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId,const Mat4 &mat)
{
	//Lua::CheckModel(l,1);
	mdl->SetBindPoseBoneMatrix(boneId,mat);
}*/
void Lua::Model::GetLocalBonePosition(lua_State *l,const std::shared_ptr<::Model> &mdl,UInt32 animId,UInt32 frameId,UInt32 boneId)
{
	//Lua::CheckModel(l,1);
	Vector3 pos;
	Quat rot;
	Vector3 scale;
	mdl->GetLocalBonePosition(animId,frameId,boneId,pos,rot,&scale);
	Lua::Push<Vector3>(l,pos);
	Lua::Push<Quat>(l,rot);
	Lua::Push<Vector3>(l,scale);
}
void Lua::Model::LookupBodyGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto id = mdl->GetBodyGroupId(name);
	Lua::PushInt(l,id);
}
void Lua::Model::GetBaseMeshGroupIds(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &baseMeshes = mdl->GetBaseMeshes();
	auto t = Lua::CreateTable(l);
	int32_t n = 1;
	for(auto &meshId : baseMeshes)
	{
		Lua::PushInt(l,n);
		Lua::PushInt(l,meshId);
		Lua::SetTableValue(l,t);
		++n;
	}
}
void Lua::Model::SetBaseMeshGroupIds(lua_State *l,const std::shared_ptr<::Model> &mdl,luabind::object o)
{
	//Lua::CheckModel(l,1);
	int32_t tIdx = 2;
	Lua::CheckTable(l,tIdx);
	std::vector<uint32_t> ids;
	auto numIds = Lua::GetObjectLength(l,tIdx);
	ids.reserve(numIds);
	for(auto i=decltype(numIds){0};i<numIds;++i)
	{
		Lua::PushInt(l,i +1);
		Lua::GetTableValue(l,tIdx);
		auto groupId = Lua::CheckInt(l,-1);
		ids.push_back(groupId);

		Lua::Pop(l,1);
	}
	mdl->GetBaseMeshes() = ids;
}
void Lua::Model::AddBaseMeshGroupId(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto groupId = Lua::CheckInt(l,1);
	auto &ids = mdl->GetBaseMeshes();
	auto it = std::find(ids.begin(),ids.end(),groupId);
	if(it != ids.end())
		return;
	ids.push_back(groupId);
}
void Lua::Model::GetMeshGroupId(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t bodyGroupId,uint32_t groupId)
{
	//Lua::CheckModel(l,1);
	uint32_t meshId = uint32_t(-1);
	auto r = mdl->GetMesh(bodyGroupId,groupId,meshId);
	UNUSED(r);
	Lua::PushInt(l,static_cast<int32_t>(meshId));
}

void Lua::Model::GetMeshGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &meshGroupName)
{
	//Lua::CheckModel(l,1);
	auto meshGroup = mdl->GetMeshGroup(meshGroupName);
	if(meshGroup == nullptr)
		return;
	Lua::Push<std::shared_ptr<::ModelMeshGroup>>(l,meshGroup);
}

void Lua::Model::GetMeshGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t)
{
	//Lua::CheckModel(l,1);
	auto group = mdl->GetMeshGroup(0);
	if(group == nullptr)
		return;
	Lua::Push<decltype(group)>(l,group);
}

void Lua::Model::GetMeshes(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &meshGroup)
{
	//Lua::CheckModel(l,1);
	auto *meshes = mdl->GetMeshes(meshGroup);
	auto t = Lua::CreateTable(l);
	if(meshes != nullptr)
	{
		int32_t n = 1;
		for(auto &mesh : *meshes)
		{
			Lua::PushInt(l,n);
			Lua::Push<std::shared_ptr<ModelMesh>>(l,mesh);
			Lua::SetTableValue(l,t);
			++n;
		}
	}
}

void Lua::Model::GetMeshes(lua_State *l,const std::shared_ptr<::Model> &mdl,luabind::object o)
{
	//Lua::CheckModel(l,1);
	Lua::CheckTable(l,2);
	std::vector<uint32_t> meshIds;
	Lua::PushNil(l);
	while(Lua::GetNextPair(l,2) != 0)
	{
		auto meshId = Lua::CheckInt(l,-1);
		meshIds.push_back(static_cast<uint32_t>(meshId));
		Lua::Pop(l,1);
	}

	std::vector<std::shared_ptr<::ModelMesh>> meshes;
	mdl->GetMeshes(meshIds,meshes);

	auto t = Lua::CreateTable(l);
	int32_t n = 1;
	for(auto &mesh : meshes)
	{
		Lua::PushInt(l,n);
		Lua::Push<std::shared_ptr<ModelMesh>>(l,mesh);
		Lua::SetTableValue(l,t);
		++n;
	}
}

void Lua::Model::GetMeshGroups(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &meshGroups = mdl->GetMeshGroups();
	auto t = Lua::CreateTable(l);
	int32_t n = 1;
	for(auto &meshGroup : meshGroups)
	{
		Lua::PushInt(l,n);
		Lua::Push<std::shared_ptr<::ModelMeshGroup>>(l,meshGroup);
		Lua::SetTableValue(l,t);
		++n;
	}
}

void Lua::Model::AddMeshGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto meshGroup = mdl->AddMeshGroup(name);
	Lua::Push<std::shared_ptr<::ModelMeshGroup>>(l,meshGroup);
}

void Lua::Model::AddMeshGroup(lua_State*,const std::shared_ptr<::Model> &mdl,std::shared_ptr<::ModelMeshGroup> &meshGroup)
{
	//Lua::CheckModel(l,1);
	mdl->AddMeshGroup(meshGroup);
}

void Lua::Model::UpdateCollisionBounds(lua_State*,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->CalculateCollisionBounds();
}
void Lua::Model::UpdateRenderBounds(lua_State*,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->CalculateRenderBounds();
}
void Lua::Model::Update(lua_State*,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->Update();
}
void Lua::Model::Update(lua_State*,const std::shared_ptr<::Model> &mdl,uint32_t flags)
{
	//Lua::CheckModel(l,1);
	mdl->Update(static_cast<ModelUpdateFlags>(flags));
}
void Lua::Model::GetName(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushString(l,mdl->GetName());
}
void Lua::Model::GetMass(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushNumber(l,mdl->GetMass());
}
void Lua::Model::SetMass(lua_State*,const std::shared_ptr<::Model> &mdl,float mass)
{
	//Lua::CheckModel(l,1);
	mdl->SetMass(mass);
}
void Lua::Model::GetBoneCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetBoneCount());
}
void Lua::Model::GetCollisionBounds(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Vector3 min,max;
	mdl->GetCollisionBounds(min,max);
	Lua::Push<Vector3>(l,min);
	Lua::Push<Vector3>(l,max);
}
void Lua::Model::GetRenderBounds(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Vector3 min,max;
	mdl->GetRenderBounds(min,max);
	Lua::Push<Vector3>(l,min);
	Lua::Push<Vector3>(l,max);
}
void Lua::Model::SetCollisionBounds(lua_State*,const std::shared_ptr<::Model> &mdl,const Vector3 &min,const Vector3 &max)
{
	//Lua::CheckModel(l,1);
	mdl->SetCollisionBounds(min,max);
}
void Lua::Model::SetRenderBounds(lua_State*,const std::shared_ptr<::Model> &mdl,const Vector3 &min,const Vector3 &max)
{
	//Lua::CheckModel(l,1);
	mdl->SetRenderBounds(min,max);
}
void Lua::Model::AddCollisionMesh(lua_State*,const std::shared_ptr<::Model> &mdl,std::shared_ptr<::CollisionMesh> &colMesh)
{
	//Lua::CheckModel(l,1);
	mdl->AddCollisionMesh(colMesh);
}
void Lua::Model::AddMaterial(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t textureGroup,Material *mat)
{
	//Lua::CheckModel(l,1);
	auto r = mdl->AddMaterial(textureGroup,mat);
	Lua::PushInt(l,r);
}
void Lua::Model::SetMaterial(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t matId,::Material *mat)
{
	//Lua::CheckModel(l,1);
	mdl->SetMaterial(matId,mat);
}
void Lua::Model::GetMaterials(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto t = Lua::CreateTable(l);
	uint32_t idx = 1;
	auto &mats = mdl->GetMaterials();
	for(auto &mat : mats)
	{
		Lua::PushInt(l,idx++);
		Lua::Push<Material*>(l,mat.get());
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetMaterialCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetMaterials().size());
}
void Lua::Model::GetMeshGroupCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetMeshGroupCount());
}
void Lua::Model::GetMeshCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetMeshCount());
}
void Lua::Model::GetSubMeshCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetSubMeshCount());
}
void Lua::Model::GetCollisionMeshCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetCollisionMeshCount());
}
void Lua::Model::GetBodyGroupId(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &bodyGroupName)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetBodyGroupId(bodyGroupName));
}
void Lua::Model::GetBodyGroupCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetBodyGroupCount());
}
static void push_body_group(lua_State *l,const BodyGroup &bg)
{
	auto t = Lua::CreateTable(l);
	Lua::PushString(l,"name");
	Lua::PushString(l,bg.name);
	Lua::SetTableValue(l,t);

	Lua::PushString(l,"meshGroups");
	auto tMg = Lua::CreateTable(l);
	for(auto i=decltype(bg.meshGroups.size()){0u};i<bg.meshGroups.size();++i)
	{
		Lua::PushInt(l,i +1u);
		Lua::PushInt(l,bg.meshGroups.at(i));
		Lua::SetTableValue(l,tMg);
	}
	Lua::SetTableValue(l,t);
}
void Lua::Model::GetBodyGroups(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto t = Lua::CreateTable(l);
	auto &bodyGroups = mdl->GetBodyGroups();
	for(auto i=decltype(bodyGroups.size()){0u};i<bodyGroups.size();++i)
	{
		auto &bg = bodyGroups.at(i);
		Lua::PushInt(l,i +1u);
		push_body_group(l,bg);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetBodyGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t bgId)
{
	//Lua::CheckModel(l,1);
	auto *bg = mdl->GetBodyGroup(bgId);
	if(bg == nullptr)
		return;
	push_body_group(l,*bg);
}
void Lua::Model::AddHitbox(lua_State*,const std::shared_ptr<::Model> &mdl,uint32_t boneId,uint32_t hitGroup,const Vector3 &min,const Vector3 &max)
{
	//Lua::CheckModel(l,1);
	mdl->AddHitbox(boneId,static_cast<HitGroup>(hitGroup),min,max);
}
void Lua::Model::GetHitboxCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetHitboxCount());
}
void Lua::Model::GetHitboxGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetHitboxGroup(boneId));
}
void Lua::Model::GetHitboxBounds(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId)
{
	//Lua::CheckModel(l,1);
	Vector3 min{0.f,0.f,0.f};
	Vector3 max{0.f,0.f,0.f};
	mdl->GetHitboxBounds(boneId,min,max);
	Lua::Push<Vector3>(l,min);
	Lua::Push<Vector3>(l,max);
}
void Lua::Model::GetHitboxBones(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t hitGroup)
{
	//Lua::CheckModel(l,1);
	auto boneIds = mdl->GetHitboxBones(static_cast<HitGroup>(hitGroup));
	auto t = Lua::CreateTable(l);
	for(auto i=decltype(boneIds.size()){0};i<boneIds.size();++i)
	{
		Lua::PushInt(l,i +1);
		Lua::PushInt(l,boneIds[i]);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetHitboxBones(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto boneIds = mdl->GetHitboxBones();
	auto t = Lua::CreateTable(l);
	for(auto i=decltype(boneIds.size()){0};i<boneIds.size();++i)
	{
		Lua::PushInt(l,i +1);
		Lua::PushInt(l,boneIds[i]);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::SetHitboxGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId,uint32_t hitGroup)
{
	//Lua::CheckModel(l,1);
	auto &hitboxes = mdl->GetHitboxes();
	auto it = hitboxes.find(boneId);
	if(it == hitboxes.end())
		return;
	it->second.group = static_cast<HitGroup>(hitGroup);
}
void Lua::Model::SetHitboxBounds(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId,const Vector3 &min,const Vector3 &max)
{
	//Lua::CheckModel(l,1);
	auto &hitboxes = mdl->GetHitboxes();
	auto it = hitboxes.find(boneId);
	if(it == hitboxes.end())
		return;
	it->second.min = min;
	it->second.max = max;
}
void Lua::Model::RemoveHitbox(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t boneId)
{
	//Lua::CheckModel(l,1);
	auto &hitboxes = mdl->GetHitboxes();
	auto it = hitboxes.find(boneId);
	if(it == hitboxes.end())
		return;
	hitboxes.erase(it);
}
void Lua::Model::GetTextureGroupCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetTextureGroups().size());
}
static void push_texture_group(lua_State *l,TextureGroup &group)
{
	auto tGroup = Lua::CreateTable(l);
	for(auto j=decltype(group.textures.size()){0};j<group.textures.size();++j)
	{
		Lua::PushInt(l,j +1);
		Lua::PushInt(l,group.textures[j]);
		Lua::SetTableValue(l,tGroup);
	}
}
void Lua::Model::GetTextureGroups(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &texGroups = mdl->GetTextureGroups();
	auto t = Lua::CreateTable(l);
	for(auto i=decltype(texGroups.size()){0};i<texGroups.size();++i)
	{
		auto &group = texGroups[i];

		Lua::PushInt(l,i +1);
		push_texture_group(l,group);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetTextureGroup(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t id)
{
	//Lua::CheckModel(l,1);
	auto *group = mdl->GetTextureGroup(id);
	if(group == nullptr)
		return;
	push_texture_group(l,*group);
}

void Lua::Model::Save(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto r = mdl->Save(engine->GetNetworkState(l)->GetGameState(),name);
	Lua::PushBool(l,r);
}

void Lua::Model::Copy(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto cpy = mdl->Copy(engine->GetNetworkState(l)->GetGameState());
	Lua::Push<decltype(cpy)>(l,cpy);
}

void Lua::Model::Copy(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t copyFlags)
{
	//Lua::CheckModel(l,1);
	auto cpy = mdl->Copy(engine->GetNetworkState(l)->GetGameState(),static_cast<::Model::CopyFlags>(copyFlags));
	Lua::Push<decltype(cpy)>(l,cpy);
}

void Lua::Model::GetVertexCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetVertexCount());
}
void Lua::Model::GetTriangleCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetTriangleCount());
}
void Lua::Model::GetTextures(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &meta = mdl->GetMetaInfo();
	auto tTextures = Lua::CreateTable(l);
	for(auto i=decltype(meta.textures.size()){0};i<meta.textures.size();++i)
	{
		Lua::PushInt(l,i +1);
		Lua::PushString(l,meta.textures[i]);
		Lua::SetTableValue(l,tTextures);
	}
}
void Lua::Model::GetTexturePaths(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &meta = mdl->GetMetaInfo();
	auto tTexturePaths = Lua::CreateTable(l);
	for(auto i=decltype(meta.texturePaths.size()){0};i<meta.texturePaths.size();++i)
	{
		Lua::PushInt(l,i +1);
		Lua::PushString(l,meta.texturePaths[i]);
		Lua::SetTableValue(l,tTexturePaths);
	}
}
void Lua::Model::LoadMaterials(lua_State *l,const std::shared_ptr<::Model> &mdl) {LoadMaterials(l,mdl,false);}
void Lua::Model::LoadMaterials(lua_State *l,const std::shared_ptr<::Model> &mdl,bool bReload)
{
	//Lua::CheckModel(l,1);
	auto *nw = engine->GetNetworkState(l);
	mdl->LoadMaterials([nw](const std::string &str,bool b) -> Material* {
		return nw->LoadMaterial(str,b);
	},bReload);
}
void Lua::Model::AddTexturePath(lua_State*,const std::shared_ptr<::Model> &mdl,const std::string &path)
{
	//Lua::CheckModel(l,1);
	mdl->AddTexturePath(path);
}

void Lua::Model::RemoveTexturePath(lua_State*,const std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveTexturePath(idx);
}
void Lua::Model::SetTexturePaths(lua_State *l,const std::shared_ptr<::Model> &mdl,luabind::object o)
{
	//Lua::CheckModel(l,1);
	Lua::CheckTable(l,2);
	std::vector<std::string> texturePaths;
	auto num = Lua::GetObjectLength(l,2);
	texturePaths.reserve(num);
	Lua::PushNil(l);
	while(Lua::GetNextPair(l,2) != 0)
	{
		auto *path = Lua::CheckString(l,-1);
		texturePaths.push_back(path);
		Lua::Pop(l,1);
	}
	mdl->SetTexturePaths(texturePaths);
}
void Lua::Model::RemoveTexture(lua_State*,const std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveTexture(idx);
}
void Lua::Model::ClearTextures(lua_State*,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->ClearTextures();
}
void Lua::Model::Rotate(lua_State*,const std::shared_ptr<::Model> &mdl,const Quat &rot)
{
	//Lua::CheckModel(l,1);
	mdl->Rotate(rot);
}
void Lua::Model::Translate(lua_State*,const std::shared_ptr<::Model> &mdl,const Vector3 &t)
{
	//Lua::CheckModel(l,1);
	mdl->Translate(t);
}
void Lua::Model::GetEyeOffset(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::Push<Vector3>(l,mdl->GetEyeOffset());
}
void Lua::Model::SetEyeOffset(lua_State *l,const std::shared_ptr<::Model> &mdl,const Vector3 &offset)
{
	//Lua::CheckModel(l,1);
	mdl->SetEyeOffset(offset);
}
void Lua::Model::AddAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,const std::shared_ptr<::Animation> &anim)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->AddAnimation(name,anim));
}
void Lua::Model::RemoveAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	auto &anims = mdl->GetAnimations();
	if(idx >= anims.size())
		return;
	anims.erase(anims.begin() +idx);
}
void Lua::Model::ClearAnimations(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->GetAnimations().clear();
}
void Lua::Model::ClipAgainstPlane(lua_State *l,std::shared_ptr<::Model> &mdl,const Vector3 &n,double d,const std::shared_ptr<::Model> &clippedMdlA,const std::shared_ptr<::Model> &clippedMdlB)
{
	//Lua::CheckModel(l,1);
	mdl->ClipAgainstPlane(n,d,*clippedMdlA,*clippedMdlB);
}
void Lua::Model::ClearMeshGroups(lua_State *l,std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->GetMeshGroups() = {};
	mdl->GetBaseMeshes() = {};
}
void Lua::Model::RemoveMeshGroup(lua_State *l,std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	auto &groups = mdl->GetMeshGroups();
	if(idx >= groups.size())
		return;
	groups.erase(groups.begin() +idx);
	auto &baseMeshes = mdl->GetBaseMeshes();
	for(auto it=baseMeshes.begin();it!=baseMeshes.end();)
	{
		auto &id = *it;
		if(id == idx)
			it = baseMeshes.erase(it);
		else
		{
			if(id > idx)
				--id;
			++it;
		}
	}
}
void Lua::Model::ClearBaseMeshGroupIds(lua_State *l,std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->GetBaseMeshes() = {};
}
void Lua::Model::AddTextureGroup(lua_State *l,std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	mdl->CreateTextureGroup();
	Lua::PushInt(l,mdl->GetTextureGroups().size() -1);
}
void Lua::Model::Merge(lua_State *l,std::shared_ptr<::Model> &mdl,std::shared_ptr<::Model> &mdlOther)
{
	//Lua::CheckModel(l,1);
	mdl->Merge(*mdlOther);
}
void Lua::Model::Merge(lua_State *l,std::shared_ptr<::Model> &mdl,std::shared_ptr<::Model> &mdlOther,uint32_t mergeFlags)
{
	//Lua::CheckModel(l,1);
	mdl->Merge(*mdlOther,static_cast<::Model::MergeFlags>(mergeFlags));
}
void Lua::Model::GetLODCount(lua_State *l,std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetLODCount());
}
static void push_lod(lua_State *l,const LODInfo &info)
{
	auto tLod = Lua::CreateTable(l);

	Lua::PushString(l,"lod");
	Lua::PushInt(l,info.lod);
	Lua::SetTableValue(l,tLod);

	Lua::PushString(l,"meshGroupReplacements");
	auto tGroupReplacements = Lua::CreateTable(l);
	for(auto &pair : info.meshReplacements)
	{
		Lua::PushInt(l,pair.first);
		int32_t meshId = (pair.second == MODEL_NO_MESH) ? -1 : pair.second;
		Lua::PushInt(l,meshId);
		Lua::SetTableValue(l,tGroupReplacements);
	}
	Lua::SetTableValue(l,tLod);
}
void Lua::Model::GetLODData(lua_State *l,std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &lods = mdl->GetLODs();
	auto tLods = Lua::CreateTable(l);
	for(auto i=decltype(lods.size()){0};i<lods.size();++i)
	{
		auto &lodInfo = lods.at(i);
		Lua::PushInt(l,i +1);
		push_lod(l,lodInfo);
		Lua::SetTableValue(l,tLods);
	}
}
void Lua::Model::GetLOD(lua_State *l,std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	auto lod = mdl->GetLOD(idx);
	Lua::PushInt(l,lod);
}
void Lua::Model::GetLODData(lua_State *l,std::shared_ptr<::Model> &mdl,uint32_t lod)
{
	//Lua::CheckModel(l,1);
	auto *lodInfo = mdl->GetLODInfo(lod);
	if(lodInfo == nullptr)
		return;
	push_lod(l,*lodInfo);
}
void Lua::Model::TranslateLODMeshes(lua_State *l,std::shared_ptr<::Model> &mdl,uint32_t lod)
{
	//Lua::CheckModel(l,1);
	std::vector<uint32_t> meshIds;
	auto numMeshGroups = mdl->GetMeshGroupCount();
	meshIds.reserve(numMeshGroups);
	for(auto i=decltype(numMeshGroups){0};i<numMeshGroups;++i)
		meshIds.push_back(i);
	auto r = mdl->TranslateLODMeshes(lod,meshIds);
	Lua::PushBool(l,r);
	if(r == false)
		return;
	auto t = Lua::CreateTable(l);
	for(auto i=decltype(meshIds.size()){0};i<meshIds.size();++i)
	{
		auto id = meshIds.at(i);
		Lua::PushInt(l,i +1);
		Lua::PushInt(l,(id == MODEL_NO_MESH) ? -1 : static_cast<int32_t>(id));
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::TranslateLODMeshes(lua_State *l,std::shared_ptr<::Model> &mdl,uint32_t lod,luabind::object o)
{
	//Lua::CheckModel(l,1);
	int32_t t = 3;
	Lua::CheckTable(l,t);

	std::vector<uint32_t> meshIds;
	auto numMeshGroups = Lua::GetObjectLength(l,t);
	meshIds.reserve(numMeshGroups);
	for(auto i=decltype(numMeshGroups){0};i<numMeshGroups;++i)
	{
		Lua::PushInt(l,i +1);
		Lua::GetTableValue(l,t);
		meshIds.push_back(Lua::CheckInt(l,-1));

		Lua::Pop(l,1);
	}

	auto r = mdl->TranslateLODMeshes(lod,meshIds);
	Lua::PushBool(l,r);
	if(r == false)
		return;
	auto tTranslated = Lua::CreateTable(l);
	for(auto i=decltype(meshIds.size()){0};i<meshIds.size();++i)
	{
		auto id = meshIds.at(i);
		Lua::PushInt(l,i +1);
		Lua::PushInt(l,(id == MODEL_NO_MESH) ? -1 : static_cast<int32_t>(id));
		Lua::SetTableValue(l,tTranslated);
	}
}
void Lua::Model::GetJoints(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	auto &joints = mdl->GetJoints();
	auto t = Lua::CreateTable(l);
	uint32_t idx = 1;
	for(auto &joint : joints)
	{
		Lua::PushInt(l,idx++);
		Lua::Push<JointInfo*>(l,&joint);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetVertexAnimations(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	auto &vertexAnims = mdl->GetVertexAnimations();
	auto t = Lua::CreateTable(l);
	auto animIdx = 1u;
	for(auto &anim : vertexAnims)
	{
		Lua::PushInt(l,animIdx++);
		Lua::Push<std::shared_ptr<::VertexAnimation>>(l,anim);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetVertexAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto *anim = mdl->GetVertexAnimation(name);
	if(anim == nullptr)
		return;
	Lua::Push<std::shared_ptr<::VertexAnimation>>(l,*anim);
}
void Lua::Model::AddVertexAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto anim = mdl->AddVertexAnimation(name);
	Lua::Push<std::shared_ptr<::VertexAnimation>>(l,anim);
}
void Lua::Model::RemoveVertexAnimation(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name) {mdl->RemoveVertexAnimation(name);}
static void push_flex_controller(lua_State *l,const FlexController &fc)
{
	auto tFc = Lua::CreateTable(l);

	Lua::PushString(l,"name");
	Lua::PushString(l,fc.name);
	Lua::SetTableValue(l,tFc);

	Lua::PushString(l,"min");
	Lua::PushNumber(l,fc.min);
	Lua::SetTableValue(l,tFc);

	Lua::PushString(l,"max");
	Lua::PushNumber(l,fc.max);
	Lua::SetTableValue(l,tFc);
}
void Lua::Model::GetFlexController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto *fc = mdl->GetFlexController(name);
	if(fc == nullptr)
		return;
	push_flex_controller(l,*fc);
}
void Lua::Model::GetFlexController(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t id)
{
	auto *fc = mdl->GetFlexController(id);
	if(fc == nullptr)
		return;
	push_flex_controller(l,*fc);
}
void Lua::Model::GetFlexControllers(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	auto t = Lua::CreateTable(l);
	auto &flexControllers = mdl->GetFlexControllers();
	auto fcId = 1u;
	for(auto &fc : flexControllers)
	{
		Lua::PushInt(l,fcId++);
		push_flex_controller(l,fc);

		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetFlexControllerId(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto id = 0u;
	if(mdl->GetFlexControllerId(name,id) == false)
		Lua::PushInt(l,-1);
	else
		Lua::PushInt(l,id);
}
void Lua::Model::GetFlexes(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	auto &flexes = mdl->GetFlexes();
	auto t = Lua::CreateTable(l);
	auto flexIdx = 1u;
	for(auto &flex : flexes)
	{
		Lua::PushInt(l,flexIdx++);
		Lua::PushString(l,flex.GetName());
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetFlexId(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto flexId = 0u;
	if(mdl->GetFlexId(name,flexId) == false)
		Lua::PushInt(l,-1);
	else
		Lua::PushInt(l,flexId);
}
void Lua::Model::GetFlexFormula(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t flexId)
{
	std::string formula;
	if(mdl->GetFlexFormula(flexId,formula) == false)
		return;
	Lua::PushString(l,formula);
}
void Lua::Model::GetFlexFormula(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &flexName)
{
	std::string formula;
	if(mdl->GetFlexFormula(flexName,formula) == false)
		return;
	Lua::PushString(l,formula);
}
void Lua::Model::GetIKControllers(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	auto &ikControllers = mdl->GetIKControllers();
	auto t = Lua::CreateTable(l);
	auto ikControllerIdx = 1u;
	for(auto &ikController : ikControllers)
	{
		Lua::PushInt(l,ikControllerIdx++);
		Lua::Push<std::shared_ptr<IKController>>(l,ikController);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::GetIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t id)
{
	auto *ikController = mdl->GetIKController(id);
	if(ikController == nullptr)
		return;
	Lua::Push<std::shared_ptr<IKController>>(l,ikController->shared_from_this());
}
void Lua::Model::LookupIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	auto ikControllerId = std::numeric_limits<uint32_t>::max();
	if(mdl->LookupIKController(name,ikControllerId) == false)
	{
		Lua::PushInt(l,-1);
		return;
	}
	Lua::PushInt(l,ikControllerId);
}
void Lua::Model::AddIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,uint32_t chainLength,const std::string &type,uint32_t method)
{
	auto *ikController = mdl->AddIKController(name,chainLength,type,static_cast<util::ik::Method>(method));
	if(ikController == nullptr)
		return;
	Lua::Push<std::shared_ptr<IKController>>(l,ikController->shared_from_this());
}
void Lua::Model::AddIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name,uint32_t chainLength,const std::string &type) {AddIKController(l,mdl,name,chainLength,type,umath::to_integral(util::ik::Method::Default));}
void Lua::Model::RemoveIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t id) {mdl->RemoveIKController(id);}
void Lua::Model::RemoveIKController(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name) {mdl->RemoveIKController(name);}
static void push_object_attachment(lua_State *l,const ObjectAttachment &att)
{
	auto tAtt = Lua::CreateTable(l);

	Lua::PushString(l,"name");
	Lua::PushString(l,att.name);
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"attachment");
	Lua::PushString(l,att.attachment);
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"type");
	Lua::PushInt(l,umath::to_integral(att.type));
	Lua::SetTableValue(l,tAtt);

	Lua::PushString(l,"keyvalues");
	auto t = Lua::CreateTable(l);
	for(auto &pair : att.keyValues)
	{
		Lua::PushString(l,pair.first);
		Lua::PushString(l,pair.second);
		Lua::SetTableValue(l,t);
	}
	Lua::SetTableValue(l,tAtt);
}
void Lua::Model::GetObjectAttachments(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	auto &objAttachments = mdl->GetObjectAttachments();
	auto t = Lua::CreateTable(l);
	auto idx = 1u;
	for(auto &objAttachment : objAttachments)
	{
		Lua::PushInt(l,idx++);
		push_object_attachment(l,objAttachment);
		Lua::SetTableValue(l,t);
	}
}
void Lua::Model::AddObjectAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t type,const std::string &name,const std::string &attachment,luabind::object oKeyValues)
{
	//Lua::CheckModel(l,1);
	std::unordered_map<std::string,std::string> keyValues;
	auto numKeyValues = Lua::GetObjectLength(l,5);

	Lua::PushNil(l);
	while(Lua::GetNextPair(l,5) != 0)
	{
		auto *key = Lua::CheckString(l,-2);
		auto *val = Lua::CheckString(l,-1);
		keyValues.insert(std::make_pair(key,val));
		Lua::Pop(l,1);
	}

	auto attId = mdl->AddObjectAttachment(static_cast<ObjectAttachment::Type>(type),name,attachment,keyValues);
	Lua::PushInt(l,attId);
}
void Lua::Model::GetObjectAttachmentCount(lua_State *l,const std::shared_ptr<::Model> &mdl)
{
	//Lua::CheckModel(l,1);
	Lua::PushInt(l,mdl->GetObjectAttachmentCount());
}
void Lua::Model::GetObjectAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	auto *att = mdl->GetObjectAttachment(idx);
	if(att == nullptr)
		return;
	push_object_attachment(l,*att);
}
void Lua::Model::LookupObjectAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	auto attId = 0u;
	auto rAttId = -1;
	if(mdl->LookupObjectAttachment(name,attId) == true)
		rAttId = attId;
	Lua::PushInt(l,rAttId);
}
void Lua::Model::RemoveObjectAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,const std::string &name)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveObjectAttachment(name);
}
void Lua::Model::RemoveObjectAttachment(lua_State *l,const std::shared_ptr<::Model> &mdl,uint32_t idx)
{
	//Lua::CheckModel(l,1);
	mdl->RemoveObjectAttachment(idx);
}