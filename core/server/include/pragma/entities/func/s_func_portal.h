#ifndef __S_FUNC_PORTAL_H__
#define __S_FUNC_PORTAL_H__
#include "pragma/serverdefinitions.h"
#include "pragma/entities/s_baseentity.h"
#include "pragma/entities/func/basefuncportal.h"

namespace pragma
{
	class DLLSERVER SFuncPortalComponent final
		: public BaseFuncPortalComponent
	{
	public:
		SFuncPortalComponent(BaseEntity &ent) : BaseFuncPortalComponent(ent) {}
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	};
};

class DLLSERVER FuncPortal
	: public SBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif