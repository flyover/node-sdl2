/**
 * Copyright (c) Flyover Games, LLC.  All rights reserved. 
 *  
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to 
 * whom the Software is furnished to do so, subject to the 
 * following conditions: 
 *  
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the 
 * Software. 
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */

#ifndef _NODE_SDL2_H_
#define _NODE_SDL2_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include <SDL.h>

// macros for modules

#define MODULE_CONSTANT(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_VALUE(target, constant, value) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew(value), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_NUMBER(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew<v8::Number>(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_STRING(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew<v8::String>(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_EXPORT_APPLY(target, name) NODE_SET_METHOD(target, #name, _native_##name)
#define MODULE_EXPORT_DECLARE(name) static NAN_METHOD(_native_##name);
#define MODULE_EXPORT_IMPLEMENT(name) static NAN_METHOD(_native_##name)
#define MODULE_EXPORT_IMPLEMENT_TODO(name) static NAN_METHOD(_native_##name) { return NanThrowError(NanNew<v8::String>("not implemented: " #name)); }

// macros for object wrapped classes

#define CLASS_METHOD_DECLARE(_name) \
	static NAN_METHOD(_name);

#define CLASS_METHOD_APPLY(_target, _name) \
	NODE_SET_PROTOTYPE_METHOD(_target, #_name, _name);

#define CLASS_METHOD_IMPLEMENT(_class, _name, _method) \
	NAN_METHOD(_class::_name)											\
	{                                                       			\
		NanScope();                                  					\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_method;														\
	}

#define CLASS_MEMBER_DECLARE(_name) \
	CLASS_MEMBER_DECLARE_GET(_name) \
	CLASS_MEMBER_DECLARE_SET(_name)

#define CLASS_MEMBER_DECLARE_GET(_name) \
	static NAN_GETTER(_get_ ## _name);

#define CLASS_MEMBER_DECLARE_SET(_name) \
	static NAN_SETTER(_set_ ## _name);

#define CLASS_MEMBER_APPLY(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, _set_ ## _name);

#define CLASS_MEMBER_APPLY_GET(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, NULL);

#define CLASS_MEMBER_APPLY_SET(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), NULL, _set_ ## _name);

#define CLASS_MEMBER_IMPLEMENT(_class, _name, _getter, _setter) \
	CLASS_MEMBER_IMPLEMENT_GET(_class, _name, _getter) \
	CLASS_MEMBER_IMPLEMENT_SET(_class, _name, _setter)

#define CLASS_MEMBER_IMPLEMENT_GET(_class, _name, _getter) \
	NAN_GETTER(_class::_get_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_IMPLEMENT_SET(_class, _name, _setter) \
	NAN_SETTER(_class::_set_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_IMPLEMENT_INT32(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(  NanNew<v8::Int32>((_cast) that->_m._name)), that->_m._name = (_cast) value->Int32Value()		)
#define CLASS_MEMBER_IMPLEMENT_UINT32(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue( NanNew<v8::Uint32>((_cast) that->_m._name)), that->_m._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_IMPLEMENT_INTEGER(_class, _m, _cast, _name)	CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew<v8::Integer>((_cast) that->_m._name)), that->_m._name = (_cast) value->IntegerValue() 	)
#define CLASS_MEMBER_IMPLEMENT_NUMBER(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue( NanNew<v8::Number>((_cast) that->_m._name)), that->_m._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_IMPLEMENT_BOOLEAN(_class, _m, _cast, _name)	CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew<v8::Boolean>((_cast) that->_m._name)), that->_m._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_IMPLEMENT_VALUE(_class, _m, _name)				CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, value))
#define CLASS_MEMBER_IMPLEMENT_STRING(_class, _m, _name)			CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_IMPLEMENT_OBJECT(_class, _m, _name)			CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Object>::Cast(value)))
#define CLASS_MEMBER_IMPLEMENT_ARRAY(_class, _m, _name)				CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Array>::Cast(value)))

#define CLASS_MEMBER_INLINE(_class, _name, _getter, _setter) \
	CLASS_MEMBER_INLINE_GET(_class, _name, _getter) \
	CLASS_MEMBER_INLINE_SET(_class, _name, _setter)

#define CLASS_MEMBER_INLINE_GET(_class, _name, _getter) \
	static NAN_GETTER(_get_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_INLINE_SET(_class, _name, _setter) \
	static NAN_SETTER(_set_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_INLINE_INT32(_class, _m, _cast, _name)		CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(  NanNew<v8::Int32>(that->_m._name)), that->_m._name = (_cast) value->Int32Value()	)
#define CLASS_MEMBER_INLINE_UINT32(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue( NanNew<v8::Uint32>(that->_m._name)), that->_m._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_INLINE_INTEGER(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew<v8::Integer>(that->_m._name)), that->_m._name = (_cast) value->IntegerValue()	)
#define CLASS_MEMBER_INLINE_NUMBER(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue( NanNew<v8::Number>(that->_m._name)), that->_m._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_INLINE_BOOLEAN(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew<v8::Boolean>(that->_m._name)), that->_m._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_INLINE_VALUE(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, value))
#define CLASS_MEMBER_INLINE_STRING(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_INLINE_OBJECT(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Object>::Cast(value)))

#define CLASS_MEMBER_UNION_APPLY(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), _get_ ## _u ## _name, _set_ ## _u ## _name);

#define CLASS_MEMBER_UNION_APPLY_GET(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), _get_ ## _u ## _name, NULL);

#define CLASS_MEMBER_UNION_APPLY_SET(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), NULL, _set_ ## _u ## _name);

#define CLASS_MEMBER_UNION_INLINE(_class, _u, _name, _getter, _setter) \
	CLASS_MEMBER_UNION_INLINE_GET(_class, _u, _name, _getter) \
	CLASS_MEMBER_UNION_INLINE_SET(_class, _u, _name, _setter)

#define CLASS_MEMBER_UNION_INLINE_GET(_class, _u, _name, _getter) \
	static NAN_GETTER(_get_ ## _u ## _name)								\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_UNION_INLINE_SET(_class, _u, _name, _setter) \
	static NAN_SETTER(_set_ ## _u ## _name)								\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_UNION_INLINE_INT32(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(  NanNew<v8::Int32>(that->_m._u._name)), that->_m._u._name = (_cast) value->Int32Value()	)
#define CLASS_MEMBER_UNION_INLINE_UINT32(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue( NanNew<v8::Uint32>(that->_m._u._name)), that->_m._u._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_UNION_INLINE_INTEGER(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew<v8::Integer>(that->_m._u._name)), that->_m._u._name = (_cast) value->IntegerValue()	)
#define CLASS_MEMBER_UNION_INLINE_NUMBER(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue( NanNew<v8::Number>(that->_m._u._name)), that->_m._u._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_UNION_INLINE_BOOLEAN(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew<v8::Boolean>(that->_m._u._name)), that->_m._u._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_UNION_INLINE_VALUE(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, value))
#define CLASS_MEMBER_UNION_INLINE_STRING(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_UNION_INLINE_OBJECT(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, v8::Handle<v8::Object>::Cast(value)))

namespace node_sdl2 {

// a simple pointer wrapper

template <class CLASS, typename TYPE> class SimpleWrap
{
	///	protected: class WeakCallbackInfo
	///	{
	///	public: _NanWeakCallbackInfo<v8::Object, WeakCallbackInfo>* cbinfo;
	///	};
	protected: NAN_WEAK_CALLBACK(WeakCallback)
	{
		NanScope();
		assert(data.GetCallbackInfo()->persistent.IsNearDeath());
		CLASS::Free(Drop(data.GetValue()));
		///	WeakCallbackInfo *info = data.GetParameter();
		///	delete info;
	}

	public: static v8::Handle<v8::Value> Hold(TYPE ptr)
	{
		NanEscapableScope();//NanScope();
		if (ptr == NULL) { return NanEscapeScope(NanNull()); }
		static v8::Persistent<v8::ObjectTemplate> s_tpl;
		if (s_tpl.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> tpl = NanNew<v8::ObjectTemplate>();
			NanAssignPersistent<v8::ObjectTemplate>(s_tpl, tpl);
			tpl->SetInternalFieldCount(1);
			CLASS::InitTemplate(tpl);
		}
		v8::Local<v8::Object> obj = NanNew<v8::ObjectTemplate>(s_tpl)->NewInstance();
		obj->SetInternalField(0, NanNew<v8::External>(ptr));
		///	WeakCallbackInfo* info = new WeakCallbackInfo();
		///	info->cbinfo = NanMakeWeakPersistent(obj, info, WeakCallback<v8::Object, WeakCallbackInfo>);
		NanMakeWeakPersistent(obj, (void*) NULL, WeakCallback<v8::Object, void>);
		return NanEscapeScope(obj);//NanReturnValue(obj);
	}

	public: static TYPE Peek(v8::Handle<v8::Value> val)
	{
		NanScope();
		if (!val->IsObject()) { return NULL; }
		v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(val);
		if (obj->InternalFieldCount() <= 0) { return NULL; }
		v8::Local<v8::Value> int_val = obj->GetInternalField(0);
		if (!int_val->IsExternal()) { return NULL; }
		v8::Local<v8::External> ext = v8::Local<v8::External>::Cast(int_val);
		if (ext->IsNull()) { return NULL; }
		return static_cast<TYPE>(ext->Value());
	}

	public: static TYPE Drop(v8::Handle<v8::Value> val)
	{
		NanScope();
		TYPE ptr = Peek(val);
		if (!val->IsObject()) { return ptr; }
		v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(val);
		if (obj->InternalFieldCount() <= 0) { return ptr; }
		obj->SetInternalField(0, NanNull());
		return ptr;
	}
};

// a simple asynchronous task

class SimpleTask
{
	private: uv_work_t m_work;
	protected: SimpleTask() { m_work.data = this; }
	protected: virtual ~SimpleTask() { m_work.data = NULL; }
	private: virtual void DoWork() = 0;
	private: virtual void DoAfterWork(int status) = 0;
	public: static int Run(SimpleTask* task)
	{
		int err = uv_queue_work(uv_default_loop(), &task->m_work, _Work, (uv_after_work_cb) _AfterWork);
		if (err != 0) { delete task; task = NULL; } // self destruct
		return err;
	}
	private: static void _Work(uv_work_t *work)
	{
		SimpleTask* task = static_cast<SimpleTask*>(work->data);
		if (task) { task->DoWork(); }
	}
	private: static void _AfterWork(uv_work_t *work, int status)
	{
		SimpleTask* task = static_cast<SimpleTask*>(work->data);
		if (task) { task->DoAfterWork(status); delete task; task = NULL; work = NULL; } // self destruct
	}
};

// wrap SDL_DisplayMode

class WrapDisplayMode : public node::ObjectWrap
{
private:
	static v8::Persistent<v8::Function> g_constructor;
public:
	static void Init(v8::Handle<v8::Object> exports);
	static v8::Handle<v8::Object> NewInstance();
	static v8::Handle<v8::Object> NewInstance(const SDL_DisplayMode& o);

private:
	SDL_DisplayMode m_display_mode;
private:
	WrapDisplayMode() {}
	WrapDisplayMode(const SDL_DisplayMode& o) { m_display_mode = o; } // struct copy
	~WrapDisplayMode() {}
public:
	SDL_DisplayMode& GetDisplayMode() { return m_display_mode; }
	const SDL_DisplayMode& GetDisplayMode() const { return m_display_mode; }
	void SetDisplayMode(const SDL_DisplayMode& display_mode) { m_display_mode = display_mode; } // struct copy
private:
	static NAN_METHOD(New);
private:
	CLASS_MEMBER_DECLARE(format)
	CLASS_MEMBER_DECLARE(w)
	CLASS_MEMBER_DECLARE(h)
	CLASS_MEMBER_DECLARE(refresh_rate)
};

// wrap SDL_Color

class WrapColor : public node::ObjectWrap
{
private:
	static v8::Persistent<v8::Function> g_constructor;
public:
	static void Init(v8::Handle<v8::Object> exports);
	static v8::Handle<v8::Object> NewInstance();
	static v8::Handle<v8::Object> NewInstance(const SDL_Color& o);

private:
	SDL_Color m_color;
private:
	WrapColor() {}
	WrapColor(const SDL_Color& o) { m_color = o; } // struct copy
	WrapColor(::Uint8 r, ::Uint8 g, ::Uint8 b, ::Uint8 a) { m_color.r = r; m_color.g = g; m_color.b = b; m_color.a = a; }
	~WrapColor() {}
public:
	SDL_Color& GetColor() { return m_color; }
	const SDL_Color& GetColor() const { return m_color; }
	void SetColor(const SDL_Color& color) { m_color = color; } // struct copy
private:
	static NAN_METHOD(New);
private:
	CLASS_MEMBER_DECLARE(r)
	CLASS_MEMBER_DECLARE(g)
	CLASS_MEMBER_DECLARE(b)
	CLASS_MEMBER_DECLARE(a)
};

// wrap SDL_Point

class WrapPoint : public node::ObjectWrap
{
private:
	static v8::Persistent<v8::Function> g_constructor;
public:
	static void Init(v8::Handle<v8::Object> exports);
	static v8::Handle<v8::Object> NewInstance();
	static v8::Handle<v8::Object> NewInstance(const SDL_Point& o);

private:
	SDL_Point m_point;
private:
	WrapPoint() {}
	WrapPoint(const SDL_Point& o) { m_point = o; } // struct copy
	WrapPoint(int x, int y) { m_point.x = x; m_point.y = y; }
	~WrapPoint() {}
public:
	SDL_Point& GetPoint() { return m_point; }
	const SDL_Point& GetPoint() const { return m_point; }
	void SetPoint(const SDL_Point& point) { m_point = point; } // struct copy
private:
	static NAN_METHOD(New);
private:
	CLASS_MEMBER_DECLARE(x)
	CLASS_MEMBER_DECLARE(y)
};

// wrap SDL_Rect

class WrapRect : public node::ObjectWrap
{
private:
	static v8::Persistent<v8::Function> g_constructor;
public:
	static void Init(v8::Handle<v8::Object> exports);
	static v8::Handle<v8::Object> NewInstance();
	static v8::Handle<v8::Object> NewInstance(const SDL_Rect& o);

private:
	SDL_Rect m_rect;
private:
	WrapRect() {}
	WrapRect(const SDL_Rect& o) { m_rect = o; } // struct copy
	WrapRect(int x, int y, int w, int h) { m_rect.x = x; m_rect.y = y; m_rect.w = w; m_rect.h = h; }
	~WrapRect() {}
public:
	SDL_Rect& GetRect() { return m_rect; }
	const SDL_Rect& GetRect() const { return m_rect; }
	void SetRect(const SDL_Rect& rect) { m_rect = rect; } // struct copy
private:
	static NAN_METHOD(New);
private:
	CLASS_MEMBER_DECLARE(x)
	CLASS_MEMBER_DECLARE(y)
	CLASS_MEMBER_DECLARE(w)
	CLASS_MEMBER_DECLARE(h)
};

// wrap SDL_PixelFormat pointer

class WrapPixelFormat : public SimpleWrap<WrapPixelFormat,SDL_PixelFormat*>
{
	#define WRAP_MEMBER_INLINE_GET_UINT32(_cast, _name) \
		static NAN_GETTER(_get_ ## _name)												\
		{																				\
			NanScope();																	\
			SDL_PixelFormat* format = Peek(args.This());                                \
			if (!format)                                                                \
			{                                                                           \
				return NanThrowError(NanNew<v8::String>("null SDL_PixelFormat object"));  \
			}                                                                           \
			NanReturnValue(NanNew<v8::Integer>(format->_name));            				\
		}

	#define WRAP_MEMBER_INLINE_SET_UINT32(_cast, _name) \
		static NAN_SETTER(_set_ ## _name)												\
		{																				\
			NanScope();																	\
			SDL_PixelFormat* format = Peek(args.This());                                \
			if (!format)                                                                \
			{                                                                           \
				return NanThrowError(NanNew<v8::String>("null SDL_PixelFormat object"));  \
			}                                                                           \
			format->_name = (_cast) value->Uint32Value();								\
		}

	#define WRAP_MEMBER_APPLY(_target, _name) \
		_target->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, _set_ ## _name);

	#define WRAP_MEMBER_APPLY_GET(_target, _name) \
		_target->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, NULL); // read-only

	#define WRAP_MEMBER_APPLY_SET(_target, _name) \
		_target->SetAccessor(NanNew<v8::String>(#_name), NULL, _set_ ## _name); // write-only

	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint32, format)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint32, BitsPerPixel)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint32, BytesPerPixel)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Rmask)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Gmask)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Bmask)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Amask)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Rloss)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Gloss)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Bloss)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Aloss)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Rshift)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Gshift)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Bshift)
	private: WRAP_MEMBER_INLINE_GET_UINT32(::Uint8, Ashift)

	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl)
	{
		WRAP_MEMBER_APPLY_GET(tpl, format);
		WRAP_MEMBER_APPLY_GET(tpl, BitsPerPixel);
		WRAP_MEMBER_APPLY_GET(tpl, BytesPerPixel);
		WRAP_MEMBER_APPLY_GET(tpl, Rmask);
		WRAP_MEMBER_APPLY_GET(tpl, Gmask);
		WRAP_MEMBER_APPLY_GET(tpl, Bmask);
		WRAP_MEMBER_APPLY_GET(tpl, Amask);
		WRAP_MEMBER_APPLY_GET(tpl, Rloss);
		WRAP_MEMBER_APPLY_GET(tpl, Gloss);
		WRAP_MEMBER_APPLY_GET(tpl, Bloss);
		WRAP_MEMBER_APPLY_GET(tpl, Aloss);
		WRAP_MEMBER_APPLY_GET(tpl, Rshift);
		WRAP_MEMBER_APPLY_GET(tpl, Gshift);
		WRAP_MEMBER_APPLY_GET(tpl, Bshift);
		WRAP_MEMBER_APPLY_GET(tpl, Ashift);
	}

	public: static void Free(SDL_PixelFormat* format)
	{
		if (format) { SDL_FreeFormat(format); format = NULL; }
	}
};

// wrap SDL_RWops pointer

class WrapRWops : public SimpleWrap<WrapRWops,SDL_RWops*>
{
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl) {}
	public: static void Free(SDL_RWops* rwops)
	{
		if (rwops && (rwops->type != SDL_RWOPS_UNKNOWN)) { SDL_RWclose(rwops); rwops = NULL; }
		if (rwops) { SDL_FreeRW(rwops); rwops = NULL; }
	}
};

// wrap SDL_Window pointer

class WrapWindow : public SimpleWrap<WrapWindow,SDL_Window*>
{
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl) {}
	public: static void Free(SDL_Window* window)
	{
		if (window) { SDL_DestroyWindow(window); window = NULL; }
	}
};

// wrap SDL_GLContext pointer

class WrapGLContext : public SimpleWrap<WrapGLContext,SDL_GLContext>
{
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl) {}
	public: static void Free(SDL_GLContext gl_context)
	{
		if (gl_context) { SDL_GL_DeleteContext(gl_context); gl_context = NULL; }
	}
};

// wrap SDL_Surface pointer

class WrapSurface : public SimpleWrap<WrapSurface,SDL_Surface*>
{
	private: static NAN_GETTER(_get_w)
	{
		NanScope();
		SDL_Surface* surface = Peek(args.This()); if (!surface) { return NanThrowError(NanNew<v8::String>("null object")); }
		NanReturnValue(NanNew<v8::Integer>(surface->w));
	}
	private: static NAN_GETTER(_get_h)
	{
		NanScope();
		SDL_Surface* surface = Peek(args.This()); if (!surface) { return NanThrowError(NanNew<v8::String>("null object")); }
		NanReturnValue(NanNew<v8::Integer>(surface->h));
	}
	private: static NAN_GETTER(_get_pitch)
	{
		NanScope();
		SDL_Surface* surface = Peek(args.This()); if (!surface) { return NanThrowError(NanNew<v8::String>("null object")); }
		NanReturnValue(NanNew<v8::Integer>(surface->pitch));
	}
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl)
	{
		tpl->SetAccessor(NanNew<v8::String>("w"), _get_w, NULL); // read-only
		tpl->SetAccessor(NanNew<v8::String>("h"), _get_h, NULL); // read-only
		tpl->SetAccessor(NanNew<v8::String>("pitch"), _get_pitch, NULL); // read-only
	}
	public: static void Free(SDL_Surface* surface)
	{
		if (surface) { SDL_FreeSurface(surface); surface = NULL; }
	}
};

// wrap SDL_Renderer pointer

class WrapRenderer : public SimpleWrap<WrapRenderer,SDL_Renderer*>
{
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl) {}
	public: static void Free(SDL_Renderer* renderer)
	{
		if (renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }
	}
};

// wrap SDL_Joystick pointer

class WrapJoystick : public SimpleWrap<WrapJoystick,SDL_Joystick*>
{
	public: static void InitTemplate(v8::Handle<v8::ObjectTemplate> tpl) {}
	public: static void Free(SDL_Joystick* joystick)
	{
		if (joystick) { SDL_JoystickClose(joystick); joystick = NULL; }
	}
};

#if NODE_VERSION_AT_LEAST(0,11,0)
void init(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> module, v8::Handle<v8::Context> context);
#else
void init(v8::Handle<v8::Object> exports/*, v8::Handle<v8::Value> module*/);
#endif

} // namespace node_sdl2

#endif // _NODE_SDL2_H_

