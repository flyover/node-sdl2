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

#include <nan.h>

#include <SDL.h>

// nan extensions

#define NANX_STRING(STRING) Nan::New<v8::String>(STRING).ToLocalChecked()
#define NANX_SYMBOL(SYMBOL) Nan::New<v8::String>(SYMBOL).ToLocalChecked()

#define NANX_CONSTANT(TARGET, CONSTANT) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), Nan::New(CONSTANT))
#define NANX_CONSTANT_VALUE(TARGET, CONSTANT, VALUE) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), Nan::New(VALUE))

#define NANX_CONSTANT_STRING(TARGET, CONSTANT) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), NANX_STRING(CONSTANT))
#define NANX_CONSTANT_STRING_VALUE(TARGET, CONSTANT, VALUE) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), NANX_STRING(VALUE))

#define NANX_EXPORT_APPLY(OBJECT, NAME) Nan::Export(OBJECT, #NAME, _export_##NAME)
#define NANX_EXPORT(NAME) static NAN_METHOD(_export_##NAME)

#define NANX_METHOD_APPLY(OBJECT_TEMPLATE, NAME) Nan::SetMethod(OBJECT_TEMPLATE, #NAME, _method_##NAME);
#define NANX_METHOD(NAME) static NAN_METHOD(_method_##NAME)

#define NANX_MEMBER_APPLY(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), _get_##NAME, _set_##NAME);
#define NANX_MEMBER_APPLY_GET(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), _get_##NAME, NULL); // get only
#define NANX_MEMBER_APPLY_SET(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), NULL, _set_##NAME); // set only

#define NANX_MEMBER_VALUE(NAME) NANX_MEMBER_VALUE_GET(NAME) NANX_MEMBER_VALUE_SET(NAME)
#define NANX_MEMBER_VALUE_GET(NAME) static NAN_GETTER(_get_##NAME) { Unwrap(info.This())->SyncPull(); info.GetReturnValue().Set(Nan::New<v8::Value>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_VALUE_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Value>()); Unwrap(info.This())->SyncPush(); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_BOOLEAN(TYPE, NAME) NANX_MEMBER_BOOLEAN_GET(TYPE, NAME) NANX_MEMBER_BOOLEAN_SET(TYPE, NAME)
#define NANX_MEMBER_BOOLEAN_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Boolean>(static_cast<bool>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_BOOLEAN_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->BooleanValue()); }

#define NANX_MEMBER_NUMBER(TYPE, NAME) NANX_MEMBER_NUMBER_GET(TYPE, NAME) NANX_MEMBER_NUMBER_SET(TYPE, NAME)
#define NANX_MEMBER_NUMBER_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Number>(static_cast<double>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_NUMBER_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->NumberValue()); }

#define NANX_MEMBER_INTEGER(TYPE, NAME) NANX_MEMBER_INTEGER_GET(TYPE, NAME) NANX_MEMBER_INTEGER_SET(TYPE, NAME)
#define NANX_MEMBER_INTEGER_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<int32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_INTEGER_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->IntegerValue()); }

#define NANX_MEMBER_INT32(TYPE, NAME) NANX_MEMBER_INT32_GET(TYPE, NAME) NANX_MEMBER_INT32_SET(TYPE, NAME)
#define NANX_MEMBER_INT32_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<int32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_INT32_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->Int32Value()); }

#define NANX_MEMBER_UINT32(TYPE, NAME) NANX_MEMBER_UINT32_GET(TYPE, NAME) NANX_MEMBER_UINT32_SET(TYPE, NAME)
#define NANX_MEMBER_UINT32_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Uint32>(static_cast<uint32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_UINT32_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->Uint32Value()); }

#define NANX_MEMBER_STRING(NAME) NANX_MEMBER_STRING_GET(NAME) NANX_MEMBER_STRING_SET(NAME)
#define NANX_MEMBER_STRING_GET(NAME) static NAN_GETTER(_get_##NAME) { Unwrap(info.This())->SyncPull(); info.GetReturnValue().Set(Nan::New<v8::String>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_STRING_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::String>()); Unwrap(info.This())->SyncPush(); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_OBJECT(NAME) NANX_MEMBER_OBJECT_GET(NAME) NANX_MEMBER_OBJECT_SET(NAME)
#define NANX_MEMBER_OBJECT_GET(NAME) static NAN_GETTER(_get_##NAME) { Unwrap(info.This())->SyncPull(); info.GetReturnValue().Set(Nan::New<v8::Object>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_OBJECT_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Object>()); Unwrap(info.This())->SyncPush(); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_ARRAY(NAME) NANX_MEMBER_ARRAY_GET(NAME) NANX_MEMBER_ARRAY_SET(NAME)
#define NANX_MEMBER_ARRAY_GET(NAME) static NAN_GETTER(_get_##NAME) { Unwrap(info.This())->SyncPull(); info.GetReturnValue().Set(Nan::New<v8::Array>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_ARRAY_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Array>()); Unwrap(info.This())->SyncPush(); info.GetReturnValue().Set(value); }

#define NANX_int(value)		static_cast<int>((value)->Int32Value())
#define NANX_double(value)	static_cast<double>((value)->NumberValue())
#define NANX_Sint8(value)	static_cast< ::Sint8 >((value)->Int32Value())
#define NANX_Uint8(value)	static_cast< ::Uint8 >((value)->Uint32Value())
#define NANX_Sint16(value)	static_cast< ::Sint16 >((value)->Int32Value())
#define NANX_Uint16(value)	static_cast< ::Uint16 >((value)->Uint32Value())
#define NANX_Sint32(value)	static_cast< ::Sint32 >((value)->Int32Value())
#define NANX_Uint32(value)	static_cast< ::Uint32 >((value)->Uint32Value())

namespace Nanx {

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

} // namespace Nanx

namespace node_sdl2 {

// wrap SDL_DisplayMode

class WrapDisplayMode : public Nan::ObjectWrap
{
private:
	SDL_DisplayMode m_display_mode;
private:
	WrapDisplayMode() {}
	WrapDisplayMode(const SDL_DisplayMode& display_mode) { m_display_mode = display_mode; } // struct copy
	~WrapDisplayMode() {}
public:
	SDL_DisplayMode* Peek() { return &m_display_mode; }
	SDL_DisplayMode& GetDisplayMode() { return m_display_mode; }
	const SDL_DisplayMode& GetDisplayMode() const { return m_display_mode; }
	void SetDisplayMode(const SDL_DisplayMode& display_mode) { m_display_mode = display_mode; } // struct copy
public:
	static WrapDisplayMode* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapDisplayMode* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapDisplayMode>(object); }
	static SDL_DisplayMode* Peek(v8::Local<v8::Value> value) { WrapDisplayMode* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapDisplayMode* wrap = new WrapDisplayMode();
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const SDL_DisplayMode& display_mode)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapDisplayMode* wrap = new WrapDisplayMode(display_mode);
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("SDL_DisplayMode"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, format)
			NANX_MEMBER_APPLY(prototype_template, w)
			NANX_MEMBER_APPLY(prototype_template, h)
			NANX_MEMBER_APPLY(prototype_template, refresh_rate)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapDisplayMode* wrap = new WrapDisplayMode();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_UINT32(::Uint32, format)
	NANX_MEMBER_INT32(int, w)
	NANX_MEMBER_INT32(int, h)
	NANX_MEMBER_INT32(int, refresh_rate)
};

// wrap SDL_Color

class WrapColor : public Nan::ObjectWrap
{
private:
	SDL_Color m_color;
private:
	WrapColor() {}
	WrapColor(const SDL_Color& color) { m_color = color; } // struct copy
	WrapColor(::Uint8 r, ::Uint8 g, ::Uint8 b, ::Uint8 a) { m_color.r = r; m_color.g = g; m_color.b = b; m_color.a = a; }
	~WrapColor() {}
public:
	SDL_Color* Peek() { return &m_color; }
	SDL_Color& GetColor() { return m_color; }
	const SDL_Color& GetColor() const { return m_color; }
	void SetColor(const SDL_Color& color) { m_color = color; } // struct copy
public:
	static WrapColor* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapColor* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapColor>(object); }
	static SDL_Color* Peek(v8::Local<v8::Value> value) { WrapColor* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapColor* wrap = new WrapColor();
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const SDL_Color& color)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapColor* wrap = new WrapColor(color);
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("SDL_Color"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, r)
			NANX_MEMBER_APPLY(prototype_template, g)
			NANX_MEMBER_APPLY(prototype_template, b)
			NANX_MEMBER_APPLY(prototype_template, a)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			::Uint8 r = (info.Length() > 0) ? (::Uint8) info[0]->NumberValue() : 0;
			::Uint8 g = (info.Length() > 1) ? (::Uint8) info[1]->NumberValue() : 0;
			::Uint8 b = (info.Length() > 2) ? (::Uint8) info[2]->NumberValue() : 0;
			::Uint8 a = (info.Length() > 3) ? (::Uint8) info[3]->NumberValue() : 0;
			WrapColor* wrap = new WrapColor(r, g, b, a);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_UINT32(::Uint8, r)
	NANX_MEMBER_UINT32(::Uint8, g)
	NANX_MEMBER_UINT32(::Uint8, b)
	NANX_MEMBER_UINT32(::Uint8, a)
};

// wrap SDL_Point

class WrapPoint : public Nan::ObjectWrap
{
private:
	SDL_Point m_point;
private:
	WrapPoint() {}
	WrapPoint(const SDL_Point& point) { m_point = point; } // struct copy
	WrapPoint(int x, int y) { m_point.x = x; m_point.y = y; }
	~WrapPoint() {}
public:
	SDL_Point* Peek() { return &m_point; }
	SDL_Point& GetPoint() { return m_point; }
	const SDL_Point& GetPoint() const { return m_point; }
	void SetPoint(const SDL_Point& point) { m_point = point; } // struct copy
public:
	static WrapPoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPoint>(object); }
	static SDL_Point* Peek(v8::Local<v8::Value> value) { WrapPoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapPoint* wrap = new WrapPoint();
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const SDL_Point& point)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapPoint* wrap = new WrapPoint(point);
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("SDL_Point"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, x)
			NANX_MEMBER_APPLY(prototype_template, y)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			int x = (info.Length() > 0) ? (int) info[0]->NumberValue() : 0;
			int y = (info.Length() > 1) ? (int) info[1]->NumberValue() : 0;
			WrapPoint* wrap = new WrapPoint(x, y);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_INT32(int, x)
	NANX_MEMBER_INT32(int, y)
};

// wrap SDL_Rect

class WrapRect : public Nan::ObjectWrap
{
private:
	SDL_Rect m_rect;
private:
	WrapRect() {}
	WrapRect(const SDL_Rect& rect) { m_rect = rect; } // struct copy
	WrapRect(int x, int y, int w, int h) { m_rect.x = x; m_rect.y = y; m_rect.w = w; m_rect.h = h; }
	~WrapRect() {}
public:
	SDL_Rect* Peek() { return &m_rect; }
	SDL_Rect& GetRect() { return m_rect; }
	const SDL_Rect& GetRect() const { return m_rect; }
	void SetRect(const SDL_Rect& rect) { m_rect = rect; } // struct copy
public:
	static WrapRect* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRect* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRect>(object); }
	static SDL_Rect* Peek(v8::Local<v8::Value> value) { WrapRect* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapRect* wrap = new WrapRect();
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const SDL_Rect& rect)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapRect* wrap = new WrapRect(rect);
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("SDL_Rect"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, x)
			NANX_MEMBER_APPLY(prototype_template, y)
			NANX_MEMBER_APPLY(prototype_template, w)
			NANX_MEMBER_APPLY(prototype_template, h)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			int x = (info.Length() > 0) ? (int) info[0]->NumberValue() : 0;
			int y = (info.Length() > 1) ? (int) info[1]->NumberValue() : 0;
			int w = (info.Length() > 2) ? (int) info[2]->NumberValue() : 0;
			int h = (info.Length() > 3) ? (int) info[3]->NumberValue() : 0;
			WrapRect* wrap = new WrapRect(x, y, w, h);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_INT32(int, x)
	NANX_MEMBER_INT32(int, y)
	NANX_MEMBER_INT32(int, w)
	NANX_MEMBER_INT32(int, h)
};

// wrap SDL_PixelFormat pointer

class WrapPixelFormat : public Nan::ObjectWrap
{
private:
	SDL_PixelFormat* m_pixel_format;
private:
	WrapPixelFormat() : m_pixel_format(NULL) {}
	WrapPixelFormat(SDL_PixelFormat* pixel_format) : m_pixel_format(pixel_format) {}
	virtual ~WrapPixelFormat() { Free(m_pixel_format); m_pixel_format = NULL; }
public:
	SDL_PixelFormat* Peek() { return m_pixel_format; }
	SDL_PixelFormat* Drop() { SDL_PixelFormat* pixel_format = m_pixel_format; m_pixel_format = NULL; return pixel_format; }
public:
	static WrapPixelFormat* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPixelFormat* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPixelFormat>(object); }
	static SDL_PixelFormat* Peek(v8::Local<v8::Value> value) { WrapPixelFormat* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_PixelFormat* pixel_format) { return NewInstance(pixel_format); }
	static SDL_PixelFormat* Drop(v8::Local<v8::Value> value) { WrapPixelFormat* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_PixelFormat* pixel_format)
	{
		if (pixel_format) { SDL_FreeFormat(pixel_format); pixel_format = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_PixelFormat* pixel_format)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapPixelFormat* wrap = new WrapPixelFormat(pixel_format);
		wrap->Wrap(instance);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
			NANX_MEMBER_APPLY(object_template, format)
			NANX_MEMBER_APPLY(object_template, BitsPerPixel)
			NANX_MEMBER_APPLY(object_template, BytesPerPixel)
			NANX_MEMBER_APPLY(object_template, Rmask)
			NANX_MEMBER_APPLY(object_template, Gmask)
			NANX_MEMBER_APPLY(object_template, Bmask)
			NANX_MEMBER_APPLY(object_template, Amask)
			NANX_MEMBER_APPLY(object_template, Rloss)
			NANX_MEMBER_APPLY(object_template, Gloss)
			NANX_MEMBER_APPLY(object_template, Bloss)
			NANX_MEMBER_APPLY(object_template, Aloss)
			NANX_MEMBER_APPLY(object_template, Rshift)
			NANX_MEMBER_APPLY(object_template, Gshift)
			NANX_MEMBER_APPLY(object_template, Bshift)
			NANX_MEMBER_APPLY(object_template, Ashift)
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
private:
	NANX_MEMBER_UINT32(::Uint32, format)
	NANX_MEMBER_UINT32(::Uint32, BitsPerPixel)
	NANX_MEMBER_UINT32(::Uint32, BytesPerPixel)
	NANX_MEMBER_UINT32(::Uint8,  Rmask)
	NANX_MEMBER_UINT32(::Uint8,  Gmask)
	NANX_MEMBER_UINT32(::Uint8,  Bmask)
	NANX_MEMBER_UINT32(::Uint8,  Amask)
	NANX_MEMBER_UINT32(::Uint8,  Rloss)
	NANX_MEMBER_UINT32(::Uint8,  Gloss)
	NANX_MEMBER_UINT32(::Uint8,  Bloss)
	NANX_MEMBER_UINT32(::Uint8,  Aloss)
	NANX_MEMBER_UINT32(::Uint8,  Rshift)
	NANX_MEMBER_UINT32(::Uint8,  Gshift)
	NANX_MEMBER_UINT32(::Uint8,  Bshift)
	NANX_MEMBER_UINT32(::Uint8,  Ashift)
};

// wrap SDL_RWops pointer

class WrapRWops : public Nan::ObjectWrap
{
private:
	SDL_RWops* m_rwops;
public:
	WrapRWops(SDL_RWops* rwops) : m_rwops(rwops) {}
	~WrapRWops() { Free(m_rwops); m_rwops = NULL; }
public:
	SDL_RWops* Peek() { return m_rwops; }
	SDL_RWops* Drop() { SDL_RWops* rwops = m_rwops; m_rwops = NULL; return rwops; }
public:
	static WrapRWops* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRWops* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRWops>(object); }
	static SDL_RWops* Peek(v8::Local<v8::Value> value) { WrapRWops* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_RWops* rwops) { return NewInstance(rwops); }
	static SDL_RWops* Drop(v8::Local<v8::Value> value) { WrapRWops* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_RWops* rwops)
	{
		if (rwops && (rwops->type != SDL_RWOPS_UNKNOWN)) { SDL_RWclose(rwops); rwops = NULL; }
		if (rwops) { SDL_FreeRW(rwops); rwops = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_RWops* rwops)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapRWops* wrap = new WrapRWops(rwops);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

// wrap SDL_Window pointer

class WrapWindow : public Nan::ObjectWrap
{
private:
	SDL_Window* m_window;
public:
	WrapWindow(SDL_Window* window) : m_window(window) {}
	~WrapWindow() { Free(m_window); m_window = NULL; }
public:
	SDL_Window* Peek() { return m_window; }
	SDL_Window* Drop() { SDL_Window* window = m_window; m_window = NULL; return window; }
public:
	static WrapWindow* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWindow* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWindow>(object); }
	static SDL_Window* Peek(v8::Local<v8::Value> value) { WrapWindow* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_Window* window) { return NewInstance(window); }
	static SDL_Window* Drop(v8::Local<v8::Value> value) { WrapWindow* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_Window* window)
	{
		if (window) { SDL_DestroyWindow(window); window = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_Window* window)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapWindow* wrap = new WrapWindow(window);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

// wrap SDL_GLContext pointer

class WrapGLContext : public Nan::ObjectWrap
{
private:
	SDL_GLContext* m_gl_context;
public:
	WrapGLContext(SDL_GLContext* gl_context) : m_gl_context(gl_context) {}
	~WrapGLContext() { Free(m_gl_context); m_gl_context = NULL; }
public:
	SDL_GLContext* Peek() { return m_gl_context; }
	SDL_GLContext* Drop() { SDL_GLContext* gl_context = m_gl_context; m_gl_context = NULL; return gl_context; }
public:
	static WrapGLContext* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapGLContext* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapGLContext>(object); }
	static SDL_GLContext* Peek(v8::Local<v8::Value> value) { WrapGLContext* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_GLContext* gl_context) { return NewInstance(gl_context); }
	static SDL_GLContext* Drop(v8::Local<v8::Value> value) { WrapGLContext* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_GLContext* gl_context)
	{
		if (gl_context) { SDL_GL_DeleteContext(*gl_context); delete gl_context; gl_context = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_GLContext* gl_context)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapGLContext* wrap = new WrapGLContext(gl_context);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

// wrap SDL_Surface pointer

class WrapSurface : public Nan::ObjectWrap
{
private:
	SDL_Surface* m_surface;
public:
	WrapSurface(SDL_Surface* surface) : m_surface(surface) {}
	~WrapSurface() { Free(m_surface); m_surface = NULL; }
public:
	SDL_Surface* Peek() { return m_surface; }
	SDL_Surface* Drop() { SDL_Surface* surface = m_surface; m_surface = NULL; return surface; }
public:
	static WrapSurface* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapSurface* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapSurface>(object); }
	static SDL_Surface* Peek(v8::Local<v8::Value> value) { WrapSurface* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_Surface* surface) { return NewInstance(surface); }
	static SDL_Surface* Drop(v8::Local<v8::Value> value) { WrapSurface* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_Surface* surface)
	{
		if (surface) { SDL_FreeSurface(surface); surface = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_Surface* surface)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapSurface* wrap = new WrapSurface(surface);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
			NANX_MEMBER_APPLY_GET(object_template, w)
			NANX_MEMBER_APPLY_GET(object_template, h)
			NANX_MEMBER_APPLY_GET(object_template, pitch)
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
private:
	NANX_MEMBER_UINT32_GET(::Uint32, w)
	NANX_MEMBER_UINT32_GET(::Uint32, h)
	NANX_MEMBER_UINT32_GET(::Uint32, pitch)
};

// wrap SDL_Renderer pointer

class WrapRenderer : public Nan::ObjectWrap
{
private:
	SDL_Renderer* m_renderer;
public:
	WrapRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}
	~WrapRenderer() { Free(m_renderer); m_renderer = NULL; }
public:
	SDL_Renderer* Peek() { return m_renderer; }
	SDL_Renderer* Drop() { SDL_Renderer* renderer = m_renderer; m_renderer = NULL; return renderer; }
public:
	static WrapRenderer* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRenderer* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRenderer>(object); }
	static SDL_Renderer* Peek(v8::Local<v8::Value> value) { WrapRenderer* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_Renderer* renderer) { return NewInstance(renderer); }
	static SDL_Renderer* Drop(v8::Local<v8::Value> value) { WrapRenderer* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_Renderer* renderer)
	{
		if (renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_Renderer* renderer)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapRenderer* wrap = new WrapRenderer(renderer);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

// wrap SDL_Joystick pointer

class WrapJoystick : public Nan::ObjectWrap
{
private:
	SDL_Joystick* m_joystick;
public:
	WrapJoystick(SDL_Joystick* joystick) : m_joystick(joystick) {}
	~WrapJoystick() { Free(m_joystick); m_joystick = NULL; }
public:
	SDL_Joystick* Peek() { return m_joystick; }
	SDL_Joystick* Drop() { SDL_Joystick* joystick = m_joystick; m_joystick = NULL; return joystick; }
public:
	static WrapJoystick* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapJoystick* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapJoystick>(object); }
	static SDL_Joystick* Peek(v8::Local<v8::Value> value) { WrapJoystick* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(SDL_Joystick* joystick) { return NewInstance(joystick); }
	static SDL_Joystick* Drop(v8::Local<v8::Value> value) { WrapJoystick* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(SDL_Joystick* joystick)
	{
		if (joystick) { SDL_JoystickClose(joystick); joystick = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(SDL_Joystick* joystick)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapJoystick* wrap = new WrapJoystick(joystick);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

NAN_MODULE_INIT(init);

} // namespace node_sdl2

#endif // _NODE_SDL2_H_
