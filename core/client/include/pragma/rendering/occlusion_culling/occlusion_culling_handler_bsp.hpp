#ifndef __OCCLUSION_CULLING_HANDLER_BSP_HPP__
#define __OCCLUSION_CULLING_HANDLER_BSP_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/rendering/occlusion_culling/occlusion_culling_handler_octtree.hpp"
#include <pragma/util/util_bsp_tree.hpp>

namespace pragma
{
	class DLLCLIENT OcclusionCullingHandlerBSP
		: public OcclusionCullingHandlerOctTree
	{
	public:
		OcclusionCullingHandlerBSP(const std::shared_ptr<util::BSPTree> &bspTree);
		const util::BSPTree::Node *FindLeafNode(const Vector3 &point) const;
		const util::BSPTree &GetBSPTree() const;

		bool ShouldPass(CBaseEntity &ent) const;
		bool ShouldPass(CModelMesh &modelMesh,const Vector3 &entityPos) const;
		bool ShouldPass(CModelSubMesh &subMesh,const Vector3 &entityPos) const;
		const util::BSPTree::Node *FindLeafNode(const util::BSPTree::Node &node,const Vector3 &point) const;
		const util::BSPTree::Node *GetCurrentNode() const;
		virtual void Update(const Vector3 &camPos) override;
		virtual void PerformCulling(const Scene &scene,std::vector<pragma::OcclusionMeshInfo> &culledMeshesOut) override;

		void SetCurrentNodeLocked(bool bLocked);
		bool IsCurrentNodeLocked() const;
	protected:
		virtual bool ShouldExamine(CModelMesh &mesh,const Vector3 &pos,bool bViewModel,std::size_t numMeshes,const std::vector<Plane> &planes) const override;
		virtual bool ShouldExamine(const Scene &scene,CBaseEntity &cent,bool &outViewModel,std::vector<Plane> **outPlanes) const override;
		std::shared_ptr<util::BSPTree> m_bspTree = nullptr;
		bool m_bLockCurrentNode = false;

		const util::BSPTree::Node *m_pCurrentNode = nullptr;
	};
};

#endif