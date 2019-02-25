#ifndef __COLLISIONS_H__
#define __COLLISIONS_H__
#include "pragma/definitions.h"
#include <mathutil/uvec.h>
#include <pragma/math/plane.h>
#include "pragma/math/vector/util_winding_order.hpp"
#include <optional>

#define INTERSECT_OUTSIDE 0
#define INTERSECT_INSIDE 1
#define INTERSECT_OVERLAP 2

struct AABB;
class ModelMesh;
class ModelSubMesh;
class Model;
namespace Intersection
{
	enum class Result : uint32_t
	{
		NoIntersection = 0,
		Intersect,
		OutOfRange // There is an intersection, but not within the specified range (t will be < 0 or > 1)
	};
	DLLENGINE bool VectorInBounds(const Vector3 &vec,const Vector3 &min,const Vector3 &max,float EPSILON=0.f);

	// Return true if the first AABB is contained within the second AABB
	DLLENGINE bool AABBInAABB(const Vector3 &minA,const Vector3 &maxA,const Vector3 &minB,const Vector3 &maxB);

	DLLENGINE int AABBAABB(const Vector3 &minA,const Vector3 &maxA,const Vector3 &minB,const Vector3 &maxB);
	DLLENGINE bool AABBAABB(const ::AABB &a,const ::AABB &b);
	DLLENGINE bool AABBTriangle(const Vector3 &min,const Vector3 &max,const Vector3 &a,const Vector3 &b,const Vector3 &c);
	DLLENGINE bool AABBPlane(const Vector3 &min,const Vector3 &max,const Vector3 &n,double d);
	DLLENGINE bool OBBPlane(const Vector3 &min,const Vector3 &max,const Vector3 &origin,const Quat &rot,const Vector3 &n,double d);
	DLLENGINE bool SpherePlane(const Vector3 &sphereOrigin,float sphereRadius,const Vector3 &n,double d);
	DLLENGINE Result LineAABB(const Vector3 &o,const Vector3 &d,const Vector3 &min,const Vector3 &max,float *tMinRes,float *tMaxRes=NULL);
	DLLENGINE Result LinePlane(const Vector3 &o,const Vector3 &d,const Vector3 &nPlane,float distPlane,float *t=NULL);
	DLLENGINE bool LineOBB(const Vector3 &rayStart,const Vector3 &rayDir,const Vector3 &min,const Vector3 &max,float *dist=nullptr,const Vector3 &origin={},const Quat &rot={});
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,ModelMesh &mesh,const Vector3 *origin=nullptr,const Quat *rot=nullptr,float *t=nullptr);
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,ModelSubMesh &subMesh,const Vector3 *origin=nullptr,const Quat *rot=nullptr,float *t=nullptr);
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,Model &mdl,const std::vector<uint32_t> &bodyGroups,uint32_t lod,const Vector3 &origin,const Quat &rot,float *t=nullptr);
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,Model &mdl,uint32_t lod,const Vector3 &origin,const Quat &rot,float *t=nullptr);
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,Model &mdl,const std::vector<uint32_t> &bodyGroups,const Vector3 &origin,const Quat &rot,float *t=nullptr);
	DLLENGINE Result LineMesh(const Vector3 &start,const Vector3 &dir,Model &mdl,const Vector3 &origin,const Quat &rot,float *t=nullptr);
	DLLENGINE bool SphereSphere(const Vector3 &originA,float rA,const Vector3 &originB,float rB);
	DLLENGINE bool AABBSphere(const Vector3 &min,const Vector3 &max,const Vector3 &origin,float r);
	DLLENGINE bool PointInPlaneMesh(const Vector3 &vec,const std::vector<Plane> &planes);
	DLLENGINE int SphereInPlaneMesh(const Vector3 &vec,float radius,const std::vector<Plane> &planes,bool skipInsideTest=false);
	DLLENGINE int AABBInPlaneMesh(const Vector3 &min,const Vector3 &max,const std::vector<Plane> &planes);
	DLLENGINE bool SphereCone(const Vector3 &sphereOrigin,float radius,const Vector3 &coneOrigin,const Vector3 &coneDir,float coneAngle);
	DLLENGINE bool LineTriangle(const Vector3 &lineOrigin,const Vector3 &lineDir,const Vector3 &v0,const Vector3 &v1,const Vector3 &v2,double &t,double &u,double &v,bool bCull=false);
};

namespace Geometry
{
	DLLENGINE void ClosestPointOnAABBToPoint(const Vector3 &min,const Vector3 &max,const Vector3 &point,Vector3 *res);
	DLLENGINE void ClosestPointOnPlaneToPoint(const Vector3 &n,float d,const Vector3 &p,Vector3 *res);
	DLLENGINE void ClosestPointOnTriangleToPoint(const Vector3 &a,const Vector3 &b,const Vector3 &c,const Vector3 &p,Vector3 *res);
	DLLENGINE float ClosestPointsBetweenLines(const Vector3 &pA,const Vector3 &qA,const Vector3 &pB,const Vector3 &qB,float *s,float *t,Vector3 *cA,Vector3 *cB);
	DLLENGINE Vector3 ClosestPointOnLineToPoint(const Vector3 &start,const Vector3 &end,const Vector3 &p,bool bClampResultToSegment=true);
	DLLENGINE Vector3 ClosestPointOnSphereToLine(const Vector3 &origin,float radius,const Vector3 &start,const Vector3 &end,bool bClampResultToSegment=true);

	DLLENGINE Vector3 CalcFaceNormal(const Vector3 &v0,const Vector3 &v1,const Vector3 &v2);
	DLLENGINE void GenerateTruncatedConeMesh(const Vector3 &origin,float startRadius,const Vector3 &dir,float dist,float endRadius,std::vector<Vector3> &verts,std::vector<uint16_t> *triangles=nullptr,std::vector<Vector3> *normals=nullptr,uint32_t segmentCount=12,bool bAddCaps=true);
	
	DLLENGINE double calc_volume_of_triangle(const Vector3 &v0,const Vector3 &v1,const Vector3 &v2);
	DLLENGINE double calc_volume_of_polyhedron(const std::function<bool(const Vector3**,const Vector3**,const Vector3**)> &fGetNextTriangle,Vector3 *centerOfMass=nullptr);
	DLLENGINE double calc_volume_of_polyhedron(const std::vector<Vector3> &verts,const std::vector<uint16_t> &triangles,Vector3 *centerOfMass=nullptr);
	DLLENGINE Vector3 calc_center_of_mass(const std::vector<Vector3> &verts,const std::vector<uint16_t> &triangles,double *volume=nullptr);

	DLLENGINE bool calc_barycentric_coordinates(const Vector3 &p0,const Vector3 &p1,const Vector3 &p2,const Vector3 &hitPoint,float &b1,float &b2);
	DLLENGINE bool calc_barycentric_coordinates(const Vector3 &p0,const Vector2 &uv0,const Vector3 &p1,const Vector2 &uv1,const Vector3 &p2,const Vector2 &uv2,const Vector3 &hitPoint,float &u,float &v);
	DLLENGINE Quat calc_rotation_between_planes(const Vector3 &n0,const Vector3 &n1);

	enum class LineSide : uint8_t
	{
		Left = 0u,
		Right,
		OnLine
	};
	DLLENGINE LineSide get_side_of_point_to_line(const Vector2 &lineStart,const Vector2 &lineEnd,const Vector2 &point);

	enum class PlaneSide : uint8_t
	{
		Front = 0u,
		Back,
		OnPlane
	};
	DLLENGINE PlaneSide get_side_of_point_to_plane(const Vector3 &n,double d,const Vector3 &p);

	DLLENGINE std::optional<std::vector<uint32_t>> get_outline_vertices(const std::vector<Vector2> &polygons);
	DLLENGINE WindingOrder get_triangle_winding_order(const Vector3 &v0,const Vector3 &v1,const Vector3 &v2,const Vector3 &n);
	DLLENGINE WindingOrder get_triangle_winding_order(const Vector2 &v0,const Vector2 &v1,const Vector2 &v2);
};

struct DLLENGINE AABB
{
public:
	AABB(Vector3 ppos,Vector3 pextents)
		: pos(ppos), extents(pextents)
	{}
	Vector3 pos;
	Vector3 extents;
	bool Intersect(AABB &b) {return Intersection::AABBAABB(*this,b);}
	float min(int i) {return pos[i] -extents[i];}
	float max(int i) {return pos[i] +extents[i];}
	static void GetRotatedBounds(const Vector3 &min,const Vector3 &max,Mat4 rot,Vector3 *rmin,Vector3 *rmax)
	{
		rot = glm::inverse(rot);
		uvec::zero(rmin);
		uvec::zero(rmax);
		for(int i=0;i<3;i++)
		{
			for(int j=0;j<3;j++)
			{
				float e = rot[i][j] *min[j];
				float f = rot[i][j] *max[j];
				if(e < f)
				{
					(*rmin)[i] += e;
					(*rmax)[i] += f;
				}
				else
				{
					(*rmin)[i] += f;
					(*rmax)[i] += e;
				}
			}
		}
	}
};

namespace Sweep
{
	DLLENGINE bool AABBWithAABB(const Vector3 &aa,const Vector3 &ab,const Vector3 &extA,
		const Vector3 &ba,const Vector3 &bb,const Vector3 &extB,
		float *entryTime,float *exitTime,Vector3 *normal);
	DLLENGINE bool AABBWithPlane(const Vector3 &origin,const Vector3 &dir,const Vector3 &ext,const Vector3 &planeNormal,float planeDistance,float *t);
};

#endif // __COLLISIONS_H__