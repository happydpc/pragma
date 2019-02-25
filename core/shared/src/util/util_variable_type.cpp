#include "stdafx_shared.h"
#include "pragma/util/util_variable_type.hpp"
#include "pragma/lua/classes/lproperty.hpp"
#include "pragma/lua/classes/lproperty_generic.hpp"
#include "pragma/networking/nwm_util.h"
#include <networkmanager/nwm_packet.h>
#include <sharedutils/datastream.h>
#include <mathutil/color.h>

struct IAnyHandler
{
	virtual std::any GetValue(lua_State *l,int32_t idx) const=0;
	virtual void Push(lua_State *l,const std::any &value) const=0;
	virtual void PushNewProperty(lua_State *l,const std::any &value) const=0;

	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const=0;
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const=0;

	virtual void Write(DataStream &ds,const std::any &value,uint32_t *pos=nullptr) const=0;
	virtual void Write(NetPacket &ds,const std::any &value,uint32_t *pos=nullptr) const=0;
	virtual void Read(Game &game,DataStream &ds,std::any &outValue) const=0;
	virtual void Read(NetPacket &ds,std::any &outValue) const=0;
	template<typename T>
		const T &Get(const std::any &value) const {return std::any_cast<const T&>(value);}
};

template<class T,class TProperty,class TLuaProperty,bool(*TIs)(lua_State*,int32_t),T(*TCheck)(lua_State*,int32_t),void(*TPush)(lua_State*,T)>
	struct TGenericHandler
		: public IAnyHandler
{
	virtual std::any GetValue(lua_State *l,int32_t idx) const override
	{
		if(TIs(l,idx) == false)
			return T{};
		return {static_cast<T>(TCheck(l,idx))};
	}
	virtual void Push(lua_State *l,const std::any &value) const override
	{
		TPush(l,std::any_cast<T>(value));
	}
	virtual void PushNewProperty(lua_State *l,const std::any &value) const override
	{
		Lua::Property::push(l,*TProperty::Create(Get<T>(value)));
	}
	virtual void Write(DataStream &ds,const std::any &value,uint32_t *pos=nullptr) const override
	{
		ds->Write<T>(Get<T>(value),pos);
	}
	virtual void Write(NetPacket &ds,const std::any &value,uint32_t *pos=nullptr) const override
	{
		ds->Write<T>(Get<T>(value),pos);
	}
	virtual void Read(Game &game,DataStream &ds,std::any &outValue) const override
	{
		outValue = ds->Read<T>();
	}
	virtual void Read(NetPacket &ds,std::any &outValue) const override
	{
		outValue = ds->Read<T>();
	}
};

template<class T>
	T check_user_class(lua_State *l,int32_t index)
{
	return Lua::Check<T>(l,index);
}

template<class T>
	void push_user_class(lua_State *l,T value)
{
	Lua::Push<T>(l,value);
}

template<class T,class TProperty,class TLuaProperty,bool(*TIs)(lua_State*,int32_t),T(*TCheck)(lua_State*,int32_t),void(*TPush)(lua_State*,T)>
	struct TGenericBasePropertyUserClassHandler
		: public TGenericHandler<T,TProperty,TLuaProperty,TIs,TCheck,TPush>
{
	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const override
	{
		if(Lua::IsType<TLuaProperty>(l,indexProperty) == false)
			return;
		*Lua::Check<TLuaProperty>(l,indexProperty) = std::any_cast<T>(value);
	}
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const override
	{
		if(Lua::IsType<TLuaProperty>(l,indexProperty) == false)
			return T{};
		return static_cast<T>(Lua::Check<TLuaProperty>(l,indexProperty)->GetValue());
	}
};

template<class T,class TProperty,class TLuaProperty,T(*TCheck)(lua_State*,int32_t),void(*TPush)(lua_State*,T)>
	struct TGenericPropertyUserClassHandler
		: public TGenericBasePropertyUserClassHandler<T,TProperty,TLuaProperty,Lua::IsType<T>,TCheck,TPush>
{};

// Note: This would be 'prettier' than creating a class, but causes compiler errors for some unknown reason
//template<class T,class TProperty,class TLuaProperty>
//	using TGenericUserClassHandler = TGenericPropertyUserClassHandler<T,TProperty,TLuaProperty,check_user_class<T>,push_user_class<T>>;
template<class T,class TProperty,class TLuaProperty>
	struct TGenericUserClassHandler
		: public TGenericPropertyUserClassHandler<T,TProperty,TLuaProperty,check_user_class<T>,push_user_class<T>>
{};

template<class T,class TProperty,class TLuaProperty>
	struct TGenericIntegerHandler
		: public TGenericHandler<T,TProperty,TLuaProperty,Lua::IsNumber,Lua::CheckInt,Lua::PushInt>
{
	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const override
	{
		if(Lua::IsType<LGenericIntPropertyWrapper>(l,indexProperty) == false)
			return;
		Lua::Check<LGenericIntPropertyWrapper>(l,indexProperty) = std::any_cast<T>(value);
	}
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const override
	{
		if(Lua::IsType<LGenericIntPropertyWrapper>(l,indexProperty) == false)
			return T{};
		return static_cast<T>(Lua::Check<LGenericIntPropertyWrapper>(l,indexProperty)->GetValue());
	}
};

template<class T,class TProperty,class TLuaProperty>
	struct TGenericFloatHandler
		: public TGenericHandler<T,TProperty,TLuaProperty,Lua::IsNumber,Lua::CheckNumber,Lua::PushNumber>
{
	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const override
	{
		if(Lua::IsType<LGenericIntPropertyWrapper>(l,indexProperty) == false)
			return;
		Lua::Check<LGenericFloatPropertyWrapper>(l,indexProperty) = std::any_cast<T>(value);
	}
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const override
	{
		if(Lua::IsType<LGenericIntPropertyWrapper>(l,indexProperty) == false)
			return T{};
		return static_cast<T>(Lua::Check<LGenericFloatPropertyWrapper>(l,indexProperty)->GetValue());
	}
};

static TGenericBasePropertyUserClassHandler<bool,util::BoolProperty,LBoolProperty,Lua::IsBool,Lua::CheckBool,Lua::PushBool> s_boolHandler;

static TGenericFloatHandler<double,util::DoubleProperty,LGenericFloatPropertyWrapper> s_doubleHandler;
static TGenericFloatHandler<float,util::FloatProperty,LGenericFloatPropertyWrapper> s_floatHandler;
static TGenericIntegerHandler<int8_t,util::Int8Property,LGenericIntPropertyWrapper> s_int8Handler;
static TGenericIntegerHandler<int16_t,util::Int16Property,LGenericIntPropertyWrapper> s_int16Handler;
static TGenericIntegerHandler<int32_t,util::Int32Property,LGenericIntPropertyWrapper> s_int32Handler;
static TGenericIntegerHandler<int64_t,util::Int64Property,LGenericIntPropertyWrapper> s_int64Handler;
static TGenericIntegerHandler<long double,util::LongDoubleProperty,LGenericFloatPropertyWrapper> s_longDoubleHandler;

static TGenericPropertyUserClassHandler<
	std::string,util::StringProperty,LStringProperty,
	static_cast<std::string(*)(lua_State*,int32_t)>([](lua_State *l,int32_t index) -> std::string {return Lua::CheckString(l,index);}),
	static_cast<void(*)(lua_State*,std::string)>([](lua_State *l,std::string value) {Lua::PushString(l,value);})
> s_stringHandler;

static TGenericIntegerHandler<uint8_t,util::UInt8Property,LGenericIntPropertyWrapper> s_uint8Handler;
static TGenericIntegerHandler<uint16_t,util::UInt16Property,LGenericIntPropertyWrapper> s_uint16Handler;
static TGenericIntegerHandler<uint32_t,util::UInt32Property,LGenericIntPropertyWrapper> s_uint32Handler;
static TGenericIntegerHandler<uint64_t,util::UInt64Property,LGenericIntPropertyWrapper> s_uint64Handler;

static TGenericUserClassHandler<EulerAngles,util::EulerAnglesProperty,LEulerAnglesProperty> s_eulerAnglesHandler;
static TGenericUserClassHandler<Color,util::ColorProperty,LColorProperty> s_colorHandler;
static TGenericUserClassHandler<Vector3,util::Vector3Property,LVector3Property> s_vector3Handler;
static TGenericUserClassHandler<Vector2,util::Vector2Property,LVector2Property> s_vector2Handler;
static TGenericUserClassHandler<Vector4,util::Vector4Property,LVector4Property> s_vector4Handler;
static TGenericUserClassHandler<Quat,util::QuatProperty,LQuatProperty> s_quatHandler;

struct EntityHandler
	: public IAnyHandler
{
	virtual std::any GetValue(lua_State *l,int32_t idx) const override
	{
		if(Lua::IsType<EntityHandle>(l,idx) == false)
			return EntityHandle{};
		return Lua::Check<EntityHandle>(l,idx);
	}
	virtual void Push(lua_State *l,const std::any &value) const override
	{
		auto hEnt = Get<EntityHandle>(value);
		if(hEnt.IsValid())
		{
			lua_pushentity(l,hEnt);
			return;
		}
		Lua::Push<EntityHandle>(l,hEnt);
	}
	virtual void PushNewProperty(lua_State *l,const std::any &value) const override
	{
		Lua::Property::push(l,*pragma::EntityProperty::Create(Get<EntityHandle>(value)));
	}
	virtual void Write(DataStream &ds,const std::any &value,uint32_t *pos=nullptr) const override
	{
		auto hEnt = Get<EntityHandle>(value);
		auto idx = hEnt.IsValid() ? hEnt->GetIndex() : std::numeric_limits<uint32_t>::max();
		ds->Write<uint32_t>(idx);
	}
	virtual void Write(NetPacket &ds,const std::any &value,uint32_t *pos=nullptr) const override
	{
		auto hEnt = Get<EntityHandle>(value);
		nwm::write_entity(ds,hEnt);
	}
	virtual void Read(Game &game,DataStream &ds,std::any &outValue) const override
	{
		auto idx = ds->Read<uint32_t>();
		auto *ent = game.GetEntity(idx);
		outValue = (ent != nullptr) ? ent->GetHandle() : EntityHandle{};
	}
	virtual void Read(NetPacket &ds,std::any &outValue) const override
	{
		auto *ent = nwm::read_entity(ds);
		outValue = (ent != nullptr) ? ent->GetHandle() : EntityHandle{};
	}
	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const override
	{
		if(Lua::IsType<LEntityProperty>(l,indexProperty) == false)
			return;
		*Lua::Check<LEntityProperty>(l,indexProperty) = std::any_cast<EntityHandle>(value);
	}
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const override
	{
		if(Lua::IsType<LEntityProperty>(l,indexProperty) == false)
			return EntityHandle{};
		return static_cast<EntityHandle>(Lua::Check<LEntityProperty>(l,indexProperty)->GetValue());
	}
} static s_entityHandler;

// If this assert fails, it means a new variable type as been added to the enum list which hasn't been included in this list yet
static_assert(umath::to_integral(util::VarType::Count) == 21u);

struct NilHandler
	: public IAnyHandler
{
	virtual std::any GetValue(lua_State *l,int32_t idx) const override {return {};}
	virtual void Push(lua_State *l,const std::any &value) const override {Lua::PushNil(l);}
	virtual void PushNewProperty(lua_State *l,const std::any &value) const override {Lua::PushNil(l);}
	virtual void SetPropertyValue(lua_State *l,int32_t indexProperty,const std::any &value) const override {}
	virtual std::any GetPropertyValue(lua_State *l,int32_t indexProperty) const override {return {};}
	virtual void Write(DataStream &ds,const std::any &value,uint32_t *pos=nullptr) const override {}
	virtual void Write(NetPacket &ds,const std::any &value,uint32_t *pos=nullptr) const override {}
	virtual void Read(Game &game,DataStream &ds,std::any &outValue) const override {outValue = {};}
	virtual void Read(NetPacket &ds,std::any &outValue) const override {outValue = {};}
} static s_nilHandler;

static constexpr const IAnyHandler &get_any_handler(util::VarType varType)
{
	switch(varType)
	{
		case util::VarType::Bool:
			return s_boolHandler;
		case util::VarType::Double:
			return s_doubleHandler;
		case util::VarType::Float:
			return s_floatHandler;
		case util::VarType::Int8:
			return s_int8Handler;
		case util::VarType::Int16:
			return s_int16Handler;
		case util::VarType::Int32:
			return s_int32Handler;
		case util::VarType::Int64:
			return s_int64Handler;
		case util::VarType::LongDouble:
			return s_longDoubleHandler;
		case util::VarType::String:
			return s_stringHandler;
		case util::VarType::UInt8:
			return s_uint8Handler;
		case util::VarType::UInt16:
			return s_uint16Handler;
		case util::VarType::UInt32:
			return s_uint32Handler;
		case util::VarType::UInt64:
			return s_uint64Handler;
		case util::VarType::EulerAngles:
			return s_eulerAnglesHandler;
		case util::VarType::Color:
			return s_colorHandler;
		case util::VarType::Vector:
			return s_vector3Handler;
		case util::VarType::Vector2:
			return s_vector2Handler;
		case util::VarType::Vector4:
			return s_vector4Handler;
		case util::VarType::Entity:
			return s_entityHandler;
		case util::VarType::Quaternion:
			return s_quatHandler;
	}
	return s_nilHandler;
}

std::any Lua::GetAnyValue(lua_State *l,util::VarType varType,int32_t idx)
{
	return get_any_handler(varType).GetValue(l,idx);
}
std::any Lua::GetAnyPropertyValue(lua_State *l,int32_t indexProperty,::util::VarType varType)
{
	return get_any_handler(varType).GetPropertyValue(l,indexProperty);
}
void Lua::SetAnyPropertyValue(lua_State *l,int32_t indexProperty,::util::VarType varType,const std::any &value)
{
	get_any_handler(varType).SetPropertyValue(l,indexProperty,value);
}
void Lua::PushAny(lua_State *l,util::VarType varType,const std::any &value)
{
	get_any_handler(varType).Push(l,value);
}
void Lua::PushNewAnyProperty(lua_State *l,::util::VarType varType,const std::any &value)
{
	get_any_handler(varType).PushNewProperty(l,value);
}
void Lua::WriteAny(DataStream &ds,::util::VarType varType,const std::any &value,uint32_t *pos)
{
	get_any_handler(varType).Write(ds,value,pos);
}
void Lua::WriteAny(NetPacket &ds,::util::VarType varType,const std::any &value,uint32_t *pos)
{
	get_any_handler(varType).Write(ds,value,pos);
}
void Lua::ReadAny(Game &game,DataStream &ds,::util::VarType varType,std::any &outValue)
{
	get_any_handler(varType).Read(game,ds,outValue);
}
void Lua::ReadAny(NetPacket &ds,::util::VarType varType,std::any &outValue)
{
	get_any_handler(varType).Read(ds,outValue);
}