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

#include "node-sdl2.h"

#include <v8.h>
#include <node.h>
#include <SDL.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "printf", __VA_ARGS__)
#endif

#define countof(_a) (sizeof(_a)/sizeof((_a)[0]))

static ::Uint32 _SDL_GetPixel(SDL_Surface* surface, int x, int y)
{
	::Uint32 pixel = 0;
	switch (surface->format->BytesPerPixel)
	{
	case 1:
	{
		::Uint8* bufp = (::Uint8*) surface->pixels + y * surface->pitch + x;
		pixel = *bufp;
		break;
	}
	case 2:
	{
		::Uint16* bufp = (::Uint16*) surface->pixels + y * surface->pitch / 2 + x;
		pixel = *bufp;
		break;
	}
	case 3:
	{
		::Uint8* bufp = (::Uint8*) surface->pixels + y * surface->pitch + x * 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		pixel = bufp[0] | (bufp[1] << 8) | (bufp[2] << 16);
#else
		pixel = bufp[2] | (bufp[1] << 8) | (bufp[0] << 16);
#endif
		break;
	}
	case 4:
	{
		::Uint32* bufp = (::Uint32*) surface->pixels + y * surface->pitch / 4 + x;
		pixel = *bufp;
		break;
	}
	}
	return pixel;
}

static void _SDL_PutPixel(SDL_Surface* surface, int x, int y, ::Uint32 pixel)
{
	switch (surface->format->BytesPerPixel)
	{
	case 1:
	{
		::Uint8* bufp = (::Uint8*) surface->pixels + y * surface->pitch + x;
		*bufp = pixel;
		break;
	}
	case 2:
	{
		::Uint16* bufp = (::Uint16*) surface->pixels + y * surface->pitch / 2 + x;
		*bufp = pixel;
		break;
	}
	case 3:
	{
		::Uint8* bufp = (::Uint8*) surface->pixels + y * surface->pitch + x * 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		bufp[0] = pixel; bufp[1] = pixel >> 8; bufp[2] = pixel >> 16;
#else
		bufp[2] = pixel; bufp[1] = pixel >> 8; bufp[0] = pixel >> 16;
#endif
		break;
	}
	case 4:
	{
		::Uint32* bufp = (::Uint32*) surface->pixels + y * surface->pitch / 4 + x;
		*bufp = pixel;
		break;
	}
	}
}

using namespace v8;

namespace node_sdl2 {

static Persistent<Value> _gl_current_window;
static Persistent<Value> _gl_current_context;

// wrap SDL_DisplayMode

Persistent<Function> WrapDisplayMode::g_constructor;

void WrapDisplayMode::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> t_SDL_DisplayMode = NanNew<FunctionTemplate>(New);
	t_SDL_DisplayMode->SetClassName(NanNew<String>("SDL_DisplayMode"));
	t_SDL_DisplayMode->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(t_SDL_DisplayMode, format)
	CLASS_MEMBER_APPLY(t_SDL_DisplayMode, w)
	CLASS_MEMBER_APPLY(t_SDL_DisplayMode, h)
	CLASS_MEMBER_APPLY(t_SDL_DisplayMode, refresh_rate)
	NanAssignPersistent<Function>(g_constructor, t_SDL_DisplayMode->GetFunction());
	exports->Set(NanNew<String>("SDL_DisplayMode"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapDisplayMode::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapDisplayMode::NewInstance(const SDL_DisplayMode& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapDisplayMode* that = node::ObjectWrap::Unwrap<WrapDisplayMode>(instance);
	that->SetDisplayMode(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapDisplayMode::New)
{
	NanScope();
	WrapDisplayMode* that = new WrapDisplayMode();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER(WrapDisplayMode, m_display_mode, ::Uint32, format)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapDisplayMode, m_display_mode, int, w)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapDisplayMode, m_display_mode, int, h)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapDisplayMode, m_display_mode, int, refresh_rate)

// wrap SDL_Color

Persistent<Function> WrapColor::g_constructor;

void WrapColor::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> t_SDL_Color = NanNew<FunctionTemplate>(New);
	t_SDL_Color->SetClassName(NanNew<String>("SDL_Color"));
	t_SDL_Color->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(t_SDL_Color, r)
	CLASS_MEMBER_APPLY(t_SDL_Color, g)
	CLASS_MEMBER_APPLY(t_SDL_Color, b)
	CLASS_MEMBER_APPLY(t_SDL_Color, a)
	NanAssignPersistent<Function>(g_constructor, t_SDL_Color->GetFunction());
	exports->Set(NanNew<String>("SDL_Color"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapColor::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapColor::NewInstance(const SDL_Color& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapColor* that = node::ObjectWrap::Unwrap<WrapColor>(instance);
	that->SetColor(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapColor::New)
{
	NanScope();
	::Uint8 r = (args.Length() > 0) ? (::Uint8) args[0]->NumberValue() : 0;
	::Uint8 g = (args.Length() > 1) ? (::Uint8) args[1]->NumberValue() : 0;
	::Uint8 b = (args.Length() > 2) ? (::Uint8) args[2]->NumberValue() : 0;
	::Uint8 a = (args.Length() > 3) ? (::Uint8) args[3]->NumberValue() : 0;
	WrapColor* that = new WrapColor(r, g, b, a);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER(WrapColor, m_color, ::Uint8, r)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapColor, m_color, ::Uint8, g)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapColor, m_color, ::Uint8, b)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapColor, m_color, ::Uint8, a)

// wrap SDL_Point

Persistent<Function> WrapPoint::g_constructor;

void WrapPoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> t_SDL_Point = NanNew<FunctionTemplate>(New);
	t_SDL_Point->SetClassName(NanNew<String>("SDL_Point"));
	t_SDL_Point->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(t_SDL_Point, x)
	CLASS_MEMBER_APPLY(t_SDL_Point, y)
	NanAssignPersistent<Function>(g_constructor, t_SDL_Point->GetFunction());
	exports->Set(NanNew<String>("SDL_Point"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapPoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapPoint::NewInstance(const SDL_Point& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapPoint* that = node::ObjectWrap::Unwrap<WrapPoint>(instance);
	that->SetPoint(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapPoint::New)
{
	NanScope();
	int x = (args.Length() > 0) ? (int) args[0]->NumberValue() : 0;
	int y = (args.Length() > 1) ? (int) args[1]->NumberValue() : 0;
	WrapPoint* that = new WrapPoint(x, y);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER(WrapPoint, m_point, int, x)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapPoint, m_point, int, y)

// wrap SDL_Rect

Persistent<Function> WrapRect::g_constructor;

void WrapRect::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> t_SDL_Rect = NanNew<FunctionTemplate>(New);
	t_SDL_Rect->SetClassName(NanNew<String>("SDL_Rect"));
	t_SDL_Rect->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(t_SDL_Rect, x)
	CLASS_MEMBER_APPLY(t_SDL_Rect, y)
	CLASS_MEMBER_APPLY(t_SDL_Rect, w)
	CLASS_MEMBER_APPLY(t_SDL_Rect, h)
	NanAssignPersistent<Function>(g_constructor, t_SDL_Rect->GetFunction());
	exports->Set(NanNew<String>("SDL_Rect"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapRect::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapRect::NewInstance(const SDL_Rect& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapRect* that = node::ObjectWrap::Unwrap<WrapRect>(instance);
	that->SetRect(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapRect::New)
{
	NanScope();
	int x = (args.Length() > 0) ? (int) args[0]->NumberValue() : 0;
	int y = (args.Length() > 1) ? (int) args[1]->NumberValue() : 0;
	int w = (args.Length() > 2) ? (int) args[2]->NumberValue() : 0;
	int h = (args.Length() > 3) ? (int) args[3]->NumberValue() : 0;
	WrapRect* that = new WrapRect(x, y, w, h);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER(WrapRect, m_rect, int, x)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapRect, m_rect, int, y)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapRect, m_rect, int, w)
CLASS_MEMBER_IMPLEMENT_NUMBER(WrapRect, m_rect, int, h)

// load surface

class TaskLoadBMP : public SimpleTask
{
	public: Persistent<Function> m_callback;
	public: char* m_file;
	public: SDL_Surface* m_surface;
	public: TaskLoadBMP(Handle<String> file, Handle<Function> callback) :
		m_file(strdup(*String::Utf8Value(file))),
		m_surface(NULL)
	{
		NanAssignPersistent(m_callback, callback);
	}
	public: ~TaskLoadBMP()
	{
		NanDisposePersistent(m_callback);
		free(m_file); m_file = NULL; // strdup
		if (m_surface) { SDL_FreeSurface(m_surface); m_surface = NULL; }
	}
	public: void DoWork()
	{
		m_surface = SDL_LoadBMP(m_file);
	}
	public: void DoAfterWork(int status)
	{
		NanScope();
		Handle<Value> argv[] = { WrapSurface::Hold(m_surface) };
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv);
		m_surface = NULL; // script owns pointer
	}
};

// save surface

class TaskSaveBMP : public SimpleTask
{
	public: Persistent<Value> m_hold_surface;
	public: Persistent<Function> m_callback;
	public: SDL_Surface* m_surface;
	public: char* m_file;
	public: int m_err;
	public: TaskSaveBMP(Handle<Value> surface, Handle<String> file, Handle<Function> callback) :
		m_surface(WrapSurface::Peek(surface)),
		m_file(strdup(*String::Utf8Value(file))),
		m_err(0)
	{
		NanAssignPersistent(m_hold_surface, surface);
		NanAssignPersistent(m_callback, callback);
	}
	public: ~TaskSaveBMP()
	{
		NanDisposePersistent(m_hold_surface);
		NanDisposePersistent(m_callback);
		free(m_file); m_file = NULL; // strdup
	}
	public: void DoWork()
	{
		m_err = SDL_SaveBMP(m_surface, m_file);
	}
	public: void DoAfterWork(int status)
	{
		NanScope();
		Handle<Value> argv[] = { NanNew<Integer>(m_err) };
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv);
	}
};

// SDL.h

MODULE_EXPORT_IMPLEMENT(SDL_Init)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	int err = SDL_Init(flags);
	if (err < 0)
	{
		printf("SDL_Init error: %d\n", err);
	}
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_InitSubSystem)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	int err = SDL_InitSubSystem(flags);
	if (err < 0)
	{
		printf("SDL_InitSubSystem error: %d\n", err);
	}
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_QuitSubSystem)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	SDL_QuitSubSystem(flags);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(SDL_WasInit)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	::Uint32 mask = SDL_WasInit(flags);
	NanReturnValue(NanNew<v8::Uint32>(mask));
}

MODULE_EXPORT_IMPLEMENT(SDL_Quit)
{
	NanScope();
	SDL_Quit();
	NanReturnUndefined();
}

// SDL_assert.h
// SDL_atomic.h
// SDL_audio.h
// SDL_bits.h

// SDL_blendmode.h

// SDL_clipboard.h

MODULE_EXPORT_IMPLEMENT(SDL_SetClipboardText)
{
	NanScope();
	Local<String> text = Local<String>::Cast(args[0]);
	int err = SDL_SetClipboardText(*String::Utf8Value(text));
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_GetClipboardText)
{
	NanScope();
	const char* clipboard_text = SDL_GetClipboardText();
	NanReturnValue(NanNew<String>(clipboard_text));
}

MODULE_EXPORT_IMPLEMENT(SDL_HasClipboardText)
{
	NanScope();
	SDL_bool has_clipboard_text = SDL_HasClipboardText();
	NanReturnValue(NanNew<Boolean>(has_clipboard_text != SDL_FALSE));
}

// SDL_config.h
// SDL_config_android.h
// SDL_config_iphoneos.h
// SDL_config_macosx.h
// SDL_config_minimal.h
// SDL_config_pandora.h
// SDL_config_psp.h
// SDL_config_windows.h
// SDL_config_wiz.h
// SDL_copying.h

// SDL_cpuinfo.h

// TODO: #define SDL_CACHELINE_SIZE  128
// TODO: extern DECLSPEC int SDLCALL SDL_GetCPUCount(void);
// TODO: extern DECLSPEC int SDLCALL SDL_GetCPUCacheLineSize(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasRDTSC(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasAltiVec(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasMMX(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_Has3DNow(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE2(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE3(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE41(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE42(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX(void);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX2(void);
// TODO: extern DECLSPEC int SDLCALL SDL_GetSystemRAM(void);

// SDL_endian.h

// SDL_error.h

MODULE_EXPORT_IMPLEMENT(SDL_GetError)
{
	NanScope();
	const char* sdl_error = SDL_GetError();
	NanReturnValue(NanNew<String>(sdl_error));
}

MODULE_EXPORT_IMPLEMENT(SDL_ClearError)
{
	NanScope();
	SDL_ClearError();
	NanReturnUndefined();
}

// SDL_events.h

class WrapEvent : public node::ObjectWrap
{
private:
	SDL_Event m_event;
private:
	static Persistent<Function> g_constructor;
private:
	static NAN_METHOD(New)
	{
		NanScope();
		WrapEvent* that = new WrapEvent();
		that->Wrap(args.This());
		NanReturnValue(args.This());
	}
    /// Uint32 type;                    /**< Event type, shared with all events */
	CLASS_MEMBER_INLINE_INTEGER(WrapEvent, m_event, ::Uint32, type)
    /// SDL_CommonEvent common;         /**< Common event data */
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, common, ::Uint32, type)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, common, ::Uint32, timestamp)
    /// SDL_WindowEvent window;         /**< Window event data */
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Uint32, type)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Uint32, timestamp)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Uint32, windowID)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Uint8, event)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Sint32, data1)
	CLASS_MEMBER_UNION_INLINE_INTEGER(WrapEvent, m_event, window, ::Sint32, data2)
	// TODO: other events
    /// SDL_KeyboardEvent key;          /**< Keyboard event data */
    /// SDL_TextEditingEvent edit;      /**< Text editing event data */
    /// SDL_TextInputEvent text;        /**< Text input event data */
    /// SDL_MouseMotionEvent motion;    /**< Mouse motion event data */
    /// SDL_MouseButtonEvent button;    /**< Mouse button event data */
    /// SDL_MouseWheelEvent wheel;      /**< Mouse wheel event data */
    /// SDL_JoyAxisEvent jaxis;         /**< Joystick axis event data */
    /// SDL_JoyBallEvent jball;         /**< Joystick ball event data */
    /// SDL_JoyHatEvent jhat;           /**< Joystick hat event data */
    /// SDL_JoyButtonEvent jbutton;     /**< Joystick button event data */
    /// SDL_JoyDeviceEvent jdevice;     /**< Joystick device change event data */
    /// SDL_ControllerAxisEvent caxis;      /**< Game Controller axis event data */
    /// SDL_ControllerButtonEvent cbutton;  /**< Game Controller button event data */
    /// SDL_ControllerDeviceEvent cdevice;  /**< Game Controller device event data */
    /// SDL_AudioDeviceEvent adevice;   /**< Audio device event data */
    /// SDL_QuitEvent quit;             /**< Quit request event data */
    /// SDL_UserEvent user;             /**< Custom event data */
    /// SDL_SysWMEvent syswm;           /**< System dependent window event data */
    /// SDL_TouchFingerEvent tfinger;   /**< Touch finger event data */
    /// SDL_MultiGestureEvent mgesture; /**< Gesture event data */
    /// SDL_DollarGestureEvent dgesture; /**< Gesture event data */
    /// SDL_DropEvent drop;             /**< Drag and drop event data */
public:
	static void Init(Handle<Object> exports)
	{
		Local<FunctionTemplate> t_SDL_Event = NanNew<FunctionTemplate>(New);
		t_SDL_Event->SetClassName(NanNew<String>("SDL_Event"));
		t_SDL_Event->InstanceTemplate()->SetInternalFieldCount(1);
		// SDL_Event
		CLASS_MEMBER_APPLY(t_SDL_Event, type)
		// SDL_CommonEvent
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, common, type)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, common, timestamp)
		// SDL_WindowEvent
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, type)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, timestamp)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, windowID)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, event)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, data1)
		CLASS_MEMBER_UNION_APPLY(t_SDL_Event, window, data2)
		// TODO: other events
		NanAssignPersistent<Function>(g_constructor, t_SDL_Event->GetFunction());
		exports->Set(NanNew<String>("SDL_Event"), NanNew<Function>(g_constructor));
	}
	static Handle<Object> NewInstance(const SDL_Event& o)
	{
		NanEscapableScope();
		Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
		WrapEvent* that = node::ObjectWrap::Unwrap<WrapEvent>(instance);
		that->m_event = o;
		return NanEscapeScope(instance);
	}
};

Persistent<Function> WrapEvent::g_constructor;

class WrapCommonEvent : public node::ObjectWrap
{
private:
	SDL_CommonEvent m_event;
private:
	static Persistent<Function> g_constructor;
private:
	static NAN_METHOD(New)
	{
		NanScope();
		WrapCommonEvent* that = new WrapCommonEvent();
		that->Wrap(args.This());
		NanReturnValue(args.This());
	}
	CLASS_MEMBER_INLINE_INTEGER(WrapCommonEvent, m_event, ::Uint32, type)
	CLASS_MEMBER_INLINE_INTEGER(WrapCommonEvent, m_event, ::Uint32, timestamp)
public:
	static void Init(Handle<Object> exports)
	{
		Local<FunctionTemplate> t_SDL_CommonEvent = NanNew<FunctionTemplate>(New);
		t_SDL_CommonEvent->SetClassName(NanNew<String>("SDL_CommonEvent"));
		t_SDL_CommonEvent->InstanceTemplate()->SetInternalFieldCount(1);
		CLASS_MEMBER_APPLY(t_SDL_CommonEvent, type)
		CLASS_MEMBER_APPLY(t_SDL_CommonEvent, timestamp)
		NanAssignPersistent<Function>(g_constructor, t_SDL_CommonEvent->GetFunction());
		exports->Set(NanNew<String>("SDL_CommonEvent"), NanNew<Function>(g_constructor));
	}
	static Handle<Object> NewInstance(const SDL_CommonEvent& o)
	{
		NanEscapableScope();
		Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
		WrapCommonEvent* that = node::ObjectWrap::Unwrap<WrapCommonEvent>(instance);
		that->m_event = o;
		return NanEscapeScope(instance);
	}
};

Persistent<Function> WrapCommonEvent::g_constructor;

class WrapWindowEvent : public node::ObjectWrap
{
private:
	SDL_WindowEvent m_event;
private:
	static Persistent<Function> g_constructor;
private:
	static NAN_METHOD(New)
	{
		NanScope();
		WrapWindowEvent* that = new WrapWindowEvent();
		that->Wrap(args.This());
		NanReturnValue(args.This());
	}
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Uint32, type)
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Uint32, timestamp)
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Uint32, windowID)
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Uint8, event)
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Sint32, data1)
	CLASS_MEMBER_INLINE_INTEGER(WrapWindowEvent, m_event, ::Sint32, data2)
public:
	static void Init(Handle<Object> exports)
	{
		Local<FunctionTemplate> t_SDL_WindowEvent = NanNew<FunctionTemplate>(New);
		t_SDL_WindowEvent->SetClassName(NanNew<String>("SDL_WindowEvent"));
		t_SDL_WindowEvent->InstanceTemplate()->SetInternalFieldCount(1);
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, type)
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, timestamp)
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, windowID)
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, event)
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, data1)
		CLASS_MEMBER_APPLY(t_SDL_WindowEvent, data2)
		NanAssignPersistent<Function>(g_constructor, t_SDL_WindowEvent->GetFunction());
		exports->Set(NanNew<String>("SDL_WindowEvent"), NanNew<Function>(g_constructor));
	}
	static Handle<Object> NewInstance(const SDL_WindowEvent& o)
	{
		NanEscapableScope();
		Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
		WrapWindowEvent* that = node::ObjectWrap::Unwrap<WrapWindowEvent>(instance);
		that->m_event = o;
		return NanEscapeScope(instance);
	}
};

Persistent<Function> WrapWindowEvent::g_constructor;

MODULE_EXPORT_IMPLEMENT(SDL_PollEvent)
{
	NanScope();

	SDL_Event event;
	if (!SDL_PollEvent(&event))
	{
		NanReturnNull();
	}

	Local<Object> evt = NanNew<Object>();
	evt->Set(NanNew<String>("type"), NanNew(event.type));
	evt->Set(NanNew<String>("timestamp"), NanNew(event.common.timestamp));

	switch (event.type)
	{
	case SDL_QUIT:
		//NanReturnValue(n_SDL_QuitEvent::NewInstance(event.quit));
		break;
	case SDL_APP_TERMINATING:
	case SDL_APP_LOWMEMORY:
	case SDL_APP_WILLENTERBACKGROUND:
	case SDL_APP_DIDENTERBACKGROUND:
	case SDL_APP_WILLENTERFOREGROUND:
	case SDL_APP_DIDENTERFOREGROUND:
		//NanReturnValue(WrapCommonEvent::NewInstance(event.common));
		break;
	case SDL_WINDOWEVENT:
		//NanReturnValue(WrapWindowEvent::NewInstance(event.window));
		evt->Set(NanNew<String>("windowID"), NanNew(event.window.windowID));
		evt->Set(NanNew<String>("event"), NanNew(event.window.event));
		evt->Set(NanNew<String>("data1"), NanNew(event.window.data1));
		evt->Set(NanNew<String>("data2"), NanNew(event.window.data2));
		break;
	case SDL_SYSWMEVENT:
		// TODO
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		evt->Set(NanNew<String>("windowID"), NanNew(event.key.windowID));
		evt->Set(NanNew<String>("state"), NanNew(event.key.state));
		evt->Set(NanNew<String>("repeat"), NanNew(event.key.repeat));
		evt->Set(NanNew<String>("scancode"), NanNew(event.key.keysym.scancode));
		evt->Set(NanNew<String>("sym"), NanNew(event.key.keysym.sym));
		evt->Set(NanNew<String>("mod"), NanNew(event.key.keysym.mod));
		break;
	case SDL_TEXTEDITING:
	case SDL_TEXTINPUT:
		// TODO
		break;
	case SDL_MOUSEMOTION:
		evt->Set(NanNew<String>("windowID"), NanNew(event.motion.windowID));
		evt->Set(NanNew<String>("which"), NanNew(event.motion.which));
		evt->Set(NanNew<String>("state"), NanNew(event.motion.state));
		evt->Set(NanNew<String>("x"), NanNew(event.motion.x));
		evt->Set(NanNew<String>("y"), NanNew(event.motion.y));
		evt->Set(NanNew<String>("xrel"), NanNew(event.motion.xrel));
		evt->Set(NanNew<String>("yrel"), NanNew(event.motion.yrel));
		{
			int w = 0, h = 0;
			SDL_GetWindowSize(SDL_GetWindowFromID(event.motion.windowID), &w, &h);
			evt->Set(NanNew<String>("nx"), NanNew((2.0f * float(event.motion.x) / w) - 1.0f));
			evt->Set(NanNew<String>("ny"), NanNew(1.0f - (2.0f * float(event.motion.y) / h)));
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		evt->Set(NanNew<String>("windowID"), NanNew(event.button.windowID));
		evt->Set(NanNew<String>("which"), NanNew(event.button.which));
		evt->Set(NanNew<String>("button"), NanNew(event.button.button));
		evt->Set(NanNew<String>("state"), NanNew(event.button.state));
		evt->Set(NanNew<String>("x"), NanNew(event.button.x));
		evt->Set(NanNew<String>("y"), NanNew(event.button.y));
		{
			int w = 0, h = 0;
			SDL_GetWindowSize(SDL_GetWindowFromID(event.motion.windowID), &w, &h);
			evt->Set(NanNew<String>("nx"), NanNew((2.0f * float(event.motion.x) / w) - 1.0f));
			evt->Set(NanNew<String>("ny"), NanNew(1.0f - (2.0f * float(event.motion.y) / h)));
		}
		break;
	case SDL_MOUSEWHEEL:
		evt->Set(NanNew<String>("windowID"), NanNew(event.wheel.windowID));
		evt->Set(NanNew<String>("which"), NanNew(event.wheel.which));
		evt->Set(NanNew<String>("x"), NanNew(event.wheel.x));
		evt->Set(NanNew<String>("y"), NanNew(event.wheel.y));
		break;
	case SDL_JOYAXISMOTION:
		evt->Set(NanNew<String>("which"), NanNew(event.jaxis.which));
		evt->Set(NanNew<String>("axis"), NanNew(event.jaxis.axis));
		evt->Set(NanNew<String>("value"), NanNew(event.jaxis.value));
		break;
	case SDL_JOYBALLMOTION:
		evt->Set(NanNew<String>("which"), NanNew(event.jball.which));
		evt->Set(NanNew<String>("ball"), NanNew(event.jball.ball));
		evt->Set(NanNew<String>("xrel"), NanNew(event.jball.xrel));
		evt->Set(NanNew<String>("yrel"), NanNew(event.jball.yrel));
		break;
	case SDL_JOYHATMOTION:
		evt->Set(NanNew<String>("which"), NanNew(event.jhat.which));
		evt->Set(NanNew<String>("hat"), NanNew(event.jhat.hat));
		evt->Set(NanNew<String>("value"), NanNew(event.jhat.value));
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		evt->Set(NanNew<String>("which"), NanNew(event.jbutton.which));
		evt->Set(NanNew<String>("button"), NanNew(event.jbutton.button));
		evt->Set(NanNew<String>("state"), NanNew(event.jbutton.state));
		break;
	case SDL_JOYDEVICEADDED:
	case SDL_JOYDEVICEREMOVED:
		evt->Set(NanNew<String>("which"), NanNew(event.jdevice.which));
		break;
	case SDL_CONTROLLERAXISMOTION:
		evt->Set(NanNew<String>("which"), NanNew(event.caxis.which));
		evt->Set(NanNew<String>("axis"), NanNew(event.caxis.axis));
		evt->Set(NanNew<String>("value"), NanNew(event.caxis.value));
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		evt->Set(NanNew<String>("which"), NanNew(event.cbutton.which));
		evt->Set(NanNew<String>("button"), NanNew(event.cbutton.button));
		evt->Set(NanNew<String>("state"), NanNew(event.cbutton.state));
		break;
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
	case SDL_CONTROLLERDEVICEREMAPPED:
		evt->Set(NanNew<String>("which"), NanNew(event.cdevice.which));
		break;
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
	case SDL_FINGERMOTION:
		evt->Set(NanNew<String>("touchId"), NanNew<Integer>((int32_t) event.tfinger.touchId)); // TODO: 64 bit integer
		evt->Set(NanNew<String>("fingerId"), NanNew<Integer>((int32_t) event.tfinger.fingerId)); // TODO: 64 bit integer
		evt->Set(NanNew<String>("x"), NanNew<Number>(event.tfinger.x));
		evt->Set(NanNew<String>("y"), NanNew<Number>(event.tfinger.y));
		evt->Set(NanNew<String>("dx"), NanNew<Number>(event.tfinger.dx));
		evt->Set(NanNew<String>("dy"), NanNew<Number>(event.tfinger.dy));
		evt->Set(NanNew<String>("pressure"), NanNew<Number>(event.tfinger.pressure));
		{
			evt->Set(NanNew<String>("nx"), NanNew((2.0f * float(event.tfinger.x)) - 1.0f));
			evt->Set(NanNew<String>("ny"), NanNew(1.0f - (2.0f * float(event.tfinger.y))));
		}
		break;
	case SDL_DOLLARGESTURE:
	case SDL_DOLLARRECORD:
	case SDL_MULTIGESTURE:
	case SDL_CLIPBOARDUPDATE:
	case SDL_DROPFILE:
	case SDL_USEREVENT:
		// TODO
		break;
	default:
		break;
	}

	NanReturnValue(evt);
}

// SDL_gamecontroller.h
// SDL_gesture.h
// SDL_haptic.h

// SDL_hints.h

MODULE_EXPORT_IMPLEMENT(SDL_SetHintWithPriority)
{
	NanScope();
	Local<String> name = Local<String>::Cast(args[0]);
	Local<String> value = Local<String>::Cast(args[1]);
	SDL_HintPriority priority = (SDL_HintPriority) args[2]->Int32Value();
	SDL_bool ret = SDL_SetHintWithPriority(*String::Utf8Value(name), *String::Utf8Value(value), priority);
	NanReturnValue(NanNew<Boolean>(ret != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_SetHint)
{
	NanScope();
	Local<String> name = Local<String>::Cast(args[0]);
	Local<String> value = Local<String>::Cast(args[1]);
	SDL_bool ret = SDL_SetHint(*String::Utf8Value(name), *String::Utf8Value(value));
	NanReturnValue(NanNew<Boolean>(ret != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_GetHint)
{
	NanScope();
	Local<String> name = Local<String>::Cast(args[0]);
	const char* value = SDL_GetHint(*String::Utf8Value(name));
	NanReturnValue(NanNew<String>(value));
}

// TODO: typedef void (*SDL_HintCallback)(void *userdata, const char *name, const char *oldValue, const char *newValue);
// TODO: extern DECLSPEC void SDLCALL SDL_AddHintCallback(const char *name, SDL_HintCallback callback, void *userdata);
// TODO: extern DECLSPEC void SDLCALL SDL_DelHintCallback(const char *name, SDL_HintCallback callback, void *userdata);

MODULE_EXPORT_IMPLEMENT(SDL_ClearHints)
{
	NanScope();
	SDL_ClearHints();
	NanReturnUndefined();
}

// SDL_joystick.h

MODULE_EXPORT_IMPLEMENT(SDL_NumJoysticks)
{
	NanScope();
	NanReturnValue(NanNew<Integer>(SDL_NumJoysticks()));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickNameForIndex)
{
	NanScope();
	int device_index = args[0]->Int32Value();
	const char* name = SDL_JoystickNameForIndex(device_index);
	NanReturnValue(NanNew<String>(name));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickOpen)
{
	NanScope();
	int device_index = args[0]->Int32Value();
	SDL_Joystick* joystick = SDL_JoystickOpen(device_index);
	NanReturnValue(WrapJoystick::Hold(joystick));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickName)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	const char* name = SDL_JoystickName(joystick);
	NanReturnValue(NanNew<String>(name));
}

// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetDeviceGUID(int device_index);
// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUID(SDL_Joystick * joystick);
// TODO: extern DECLSPEC void SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char *pszGUID, int cbGUID);
// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUIDFromString(const char *pchGUID);

MODULE_EXPORT_IMPLEMENT(SDL_JoystickGetAttached)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	SDL_bool attached = SDL_JoystickGetAttached(joystick);
	NanReturnValue(NanNew<Boolean>(attached != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickInstanceID)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
	NanReturnValue(NanNew<Integer>(id));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickNumAxes)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int num = SDL_JoystickNumAxes(joystick);
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickNumBalls)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int num = SDL_JoystickNumBalls(joystick);
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickNumHats)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int num = SDL_JoystickNumHats(joystick);
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickNumButtons)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int num = SDL_JoystickNumButtons(joystick);
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickUpdate)
{
	NanScope();
	SDL_JoystickUpdate();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickEventState)
{
	NanScope();
	int state = args[0]->Int32Value();
	int err = SDL_JoystickEventState(state);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickGetAxis)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int index = args[1]->Int32Value();
	Sint16 value = SDL_JoystickGetAxis(joystick, index);
	NanReturnValue(NanNew<Integer>(value));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickGetBall)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int index = args[1]->Int32Value();
	int dx = 0; // TODO
	int dy = 0; // TODO
	int value = SDL_JoystickGetBall(joystick, index, &dx, &dy);
	NanReturnValue(NanNew<Integer>(value));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickGetHat)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int index = args[1]->Int32Value();
	Uint8 value = SDL_JoystickGetHat(joystick, index);
	NanReturnValue(NanNew(value));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickGetButton)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Peek(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	int index = args[1]->Int32Value();
	Uint8 value = SDL_JoystickGetButton(joystick, index);
	NanReturnValue(NanNew(value));
}

MODULE_EXPORT_IMPLEMENT(SDL_JoystickClose)
{
	NanScope();
	SDL_Joystick* joystick = WrapJoystick::Drop(args[0]); if (!joystick) { return NanThrowError(NanNew<String>("null SDL_Joystick object")); }
	SDL_JoystickClose(joystick);
	NanReturnUndefined();
}

// SDL_keyboard.h
// SDL_keycode.h
// SDL_loadso.h
// SDL_log.h
// SDL_main.h
// SDL_messagebox.h
// SDL_mouse.h
// SDL_mutex.h
// SDL_name.h
// SDL_opengl.h
// SDL_opengles.h
// SDL_opengles2.h
// SDL_pixels.h

MODULE_EXPORT_IMPLEMENT(SDL_PIXELFLAG)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_PIXELFLAG(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_PIXELTYPE)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_PIXELTYPE(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_PIXELORDER)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_PIXELORDER(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_PIXELLAYOUT)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_PIXELLAYOUT(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_BITSPERPIXEL)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_BITSPERPIXEL(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_BYTESPERPIXEL)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<v8::Uint32>(SDL_BYTESPERPIXEL(format)));
}

MODULE_EXPORT_IMPLEMENT(SDL_ISPIXELFORMAT_INDEXED)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<Boolean>(SDL_ISPIXELFORMAT_INDEXED(format) != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_ISPIXELFORMAT_ALPHA)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<Boolean>(SDL_ISPIXELFORMAT_ALPHA(format) != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_ISPIXELFORMAT_FOURCC)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	NanReturnValue(NanNew<Boolean>(SDL_ISPIXELFORMAT_FOURCC(format) != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_GetPixelFormatName)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	const char* name = SDL_GetPixelFormatName(format);
	NanReturnValue(NanNew<String>(name));
}

MODULE_EXPORT_IMPLEMENT(SDL_PixelFormatEnumToMasks)
{
	NanScope();
	::Uint32 format = args[0]->Uint32Value();
	int bpp = 0;
	::Uint32 Rmask = 0;
	::Uint32 Gmask = 0;
	::Uint32 Bmask = 0;
	::Uint32 Amask = 0;
	SDL_bool success = SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
	if (args[1]->IsObject())
	{
		Local<Object> masks = Local<Object>::Cast(args[1]);
		masks->Set(NanNew<String>("bpp"), NanNew<Integer>(bpp));
		masks->Set(NanNew<String>("Rmask"), NanNew(Rmask));
		masks->Set(NanNew<String>("Gmask"), NanNew(Gmask));
		masks->Set(NanNew<String>("Bmask"), NanNew(Bmask));
		masks->Set(NanNew<String>("Amask"), NanNew(Amask));
	}
	NanReturnValue(NanNew<Boolean>(success != SDL_FALSE));
}

MODULE_EXPORT_IMPLEMENT(SDL_MasksToPixelFormatEnum)
{
	NanScope();
	int bpp = args[0]->Int32Value();
	::Uint32 Rmask = args[1]->Uint32Value();
	::Uint32 Gmask = args[2]->Uint32Value();
	::Uint32 Bmask = args[3]->Uint32Value();
	::Uint32 Amask = args[4]->Uint32Value();
	::Uint32 format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
	NanReturnValue(NanNew(format));
}

// extern DECLSPEC SDL_PixelFormat * SDLCALL SDL_AllocFormat(Uint32 pixel_format);
MODULE_EXPORT_IMPLEMENT(SDL_AllocFormat)
{
	NanScope();
	::Uint32 pixel_format = args[0]->Uint32Value();
	SDL_PixelFormat* format = SDL_AllocFormat(pixel_format);
	NanReturnValue(WrapPixelFormat::Hold(format));
}

// extern DECLSPEC void SDLCALL SDL_FreeFormat(SDL_PixelFormat *format);
MODULE_EXPORT_IMPLEMENT(SDL_FreeFormat)
{
	NanScope();
	SDL_PixelFormat* format = WrapPixelFormat::Drop(args[0]); if (!format) { return NanThrowError(NanNew<String>("null SDL_PixelFormat object")); }
	SDL_FreeFormat(format); format = NULL;
	NanReturnUndefined();
}

// TODO: extern DECLSPEC SDL_Palette *SDLCALL SDL_AllocPalette(int ncolors);
// TODO: extern DECLSPEC int SDLCALL SDL_SetPixelFormatPalette(SDL_PixelFormat * format, SDL_Palette *palette);
// TODO: extern DECLSPEC int SDLCALL SDL_SetPaletteColors(SDL_Palette * palette, const SDL_Color * colors, int firstcolor, int ncolors);
// TODO: extern DECLSPEC void SDLCALL SDL_FreePalette(SDL_Palette * palette);

// extern DECLSPEC Uint32 SDLCALL SDL_MapRGB(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b);
MODULE_EXPORT_IMPLEMENT(SDL_MapRGB)
{
	NanScope();
	SDL_PixelFormat* format = WrapPixelFormat::Peek(args[0]); if (!format) { return NanThrowError(NanNew<String>("null SDL_PixelFormat object")); }
	::Uint8 r = (::Uint8) args[1]->Uint32Value();
	::Uint8 g = (::Uint8) args[2]->Uint32Value();
	::Uint8 b = (::Uint8) args[3]->Uint32Value();
	::Uint32 pixel = SDL_MapRGB(format, r, g, b);
	NanReturnValue(NanNew(pixel));
}

// extern DECLSPEC Uint32 SDLCALL SDL_MapRGBA(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
MODULE_EXPORT_IMPLEMENT(SDL_MapRGBA)
{
	NanScope();
	SDL_PixelFormat* format = WrapPixelFormat::Peek(args[0]); if (!format) { return NanThrowError(NanNew<String>("null SDL_PixelFormat object")); }
	::Uint8 r = (::Uint8) args[1]->Uint32Value();
	::Uint8 g = (::Uint8) args[2]->Uint32Value();
	::Uint8 b = (::Uint8) args[3]->Uint32Value();
	::Uint8 a = (::Uint8) args[4]->Uint32Value();
	::Uint32 pixel = SDL_MapRGBA(format, r, g, b, a);
	NanReturnValue(NanNew(pixel));
}

// TODO: extern DECLSPEC void SDLCALL SDL_GetRGB(Uint32 pixel, const SDL_PixelFormat * format, Uint8 * r, Uint8 * g, Uint8 * b);
// TODO: extern DECLSPEC void SDLCALL SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat * format, Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a);
// TODO: extern DECLSPEC void SDLCALL SDL_CalculateGammaRamp(float gamma, Uint16 * ramp);

// SDL_platform.h

// extern DECLSPEC const char * SDLCALL SDL_GetPlatform (void);
MODULE_EXPORT_IMPLEMENT(SDL_GetPlatform)
{
	NanScope();
	NanReturnValue(NanNew<String>(SDL_GetPlatform()));
}

// SDL_power.h

// extern DECLSPEC SDL_PowerState SDLCALL SDL_GetPowerInfo(int *secs, int *pct);
MODULE_EXPORT_IMPLEMENT(SDL_GetPowerInfo)
{
	NanScope();
	int secs = 0;
	int pct = 0;
	SDL_PowerState power_state = SDL_GetPowerInfo(&secs, &pct);
	if (args[1]->IsObject())
	{
		// var info = { secs: -1, pct: -1 };
		// var power_state = sdl.SDL_GetPowerInfo(info);
		Local<Object> info = Local<Object>::Cast(args[1]);
		info->Set(NanNew<String>("secs"), NanNew<Integer>(secs));
		info->Set(NanNew<String>("pct"), NanNew<Integer>(pct));
	}
	else
	{
		// var a_secs = [ -1 ];
		// var a_pct = [ -1 ];
		// var power_state = sdl.SDL_GetPowerInfo(a_secs, a_pct);
		// var secs = a_secs[0];
		// var pct = a_pcs[0];
		if (args[1]->IsArray())
		{
			Local<Object> a = Local<Object>::Cast(args[1]);
			a->Set(0, NanNew<Integer>(secs));
		}
		if (args[2]->IsArray())
		{
			Local<Object> a = Local<Object>::Cast(args[2]);
			a->Set(0, NanNew<Integer>(pct));
		}
	}
	NanReturnValue(NanNew<Integer>(power_state));
}

// SDL_quit.h

MODULE_EXPORT_IMPLEMENT(SDL_QuitRequested)
{
	NanScope();
	NanReturnValue(NanNew<Boolean>(SDL_QuitRequested() != SDL_FALSE));
}

// SDL_rect.h

// SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
MODULE_EXPORT_IMPLEMENT(SDL_RectEmpty)
{
	NanScope();
	WrapRect* r = node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[0]));
	NanReturnValue(NanNew<Boolean>(SDL_RectEmpty(&r->GetRect()) != SDL_FALSE));
}

// SDL_FORCE_INLINE SDL_bool SDL_RectEquals(const SDL_Rect *a, const SDL_Rect *b)
MODULE_EXPORT_IMPLEMENT(SDL_RectEquals)
{
	NanScope();
	WrapRect* a = node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[0]));
	WrapRect* b = node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]));
	NanReturnValue(NanNew<Boolean>(SDL_RectEquals(&a->GetRect(), &b->GetRect()) != SDL_FALSE));
}

// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_HasIntersection(const SDL_Rect * A, const SDL_Rect * B);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result);
// TODO: extern DECLSPEC void SDLCALL SDL_UnionRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_EnclosePoints(const SDL_Point * points, int count, const SDL_Rect * clip, SDL_Rect * result);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRectAndLine(const SDL_Rect *rect, int *X1, int *Y1, int *X2, int *Y2);

// SDL_render.h

// TODO: extern DECLSPEC int SDLCALL SDL_GetNumRenderDrivers(void);
// TODO: extern DECLSPEC int SDLCALL SDL_GetRenderDriverInfo(int index, SDL_RendererInfo * info);
// TODO: extern DECLSPEC int SDLCALL SDL_CreateWindowAndRenderer(int width, int height, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer);

// extern DECLSPEC SDL_Renderer * SDLCALL SDL_CreateRenderer(SDL_Window * window,
MODULE_EXPORT_IMPLEMENT(SDL_CreateRenderer)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	int index = args[1]->Int32Value();
	::Uint32 flags = args[2]->Uint32Value();
	SDL_Renderer* renderer = SDL_CreateRenderer(window, index, flags);
	NanReturnValue(WrapRenderer::Hold(renderer));
}

// extern DECLSPEC SDL_Renderer * SDLCALL SDL_CreateSoftwareRenderer(SDL_Surface * surface);
MODULE_EXPORT_IMPLEMENT(SDL_CreateSoftwareRenderer)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
	NanReturnValue(WrapRenderer::Hold(renderer));
}

// TODO: extern DECLSPEC SDL_Renderer * SDLCALL SDL_GetRenderer(SDL_Window * window);
// TODO: extern DECLSPEC int SDLCALL SDL_GetRendererInfo(SDL_Renderer * renderer, SDL_RendererInfo * info);
// TODO: extern DECLSPEC int SDLCALL SDL_GetRendererOutputSize(SDL_Renderer * renderer, int *w, int *h);
// TODO: extern DECLSPEC SDL_Texture * SDLCALL SDL_CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h);
// TODO: extern DECLSPEC SDL_Texture * SDLCALL SDL_CreateTextureFromSurface(SDL_Renderer * renderer, SDL_Surface * surface);
// TODO: extern DECLSPEC int SDLCALL SDL_QueryTexture(SDL_Texture * texture, Uint32 * format, int *access, int *w, int *h);
// TODO: extern DECLSPEC int SDLCALL SDL_SetTextureColorMod(SDL_Texture * texture, Uint8 r, Uint8 g, Uint8 b);
// TODO: extern DECLSPEC int SDLCALL SDL_GetTextureColorMod(SDL_Texture * texture, Uint8 * r, Uint8 * g, Uint8 * b);
// TODO: extern DECLSPEC int SDLCALL SDL_SetTextureAlphaMod(SDL_Texture * texture, Uint8 alpha);
// TODO: extern DECLSPEC int SDLCALL SDL_GetTextureAlphaMod(SDL_Texture * texture, Uint8 * alpha);
// TODO: extern DECLSPEC int SDLCALL SDL_SetTextureBlendMode(SDL_Texture * texture, SDL_BlendMode blendMode);
// TODO: extern DECLSPEC int SDLCALL SDL_GetTextureBlendMode(SDL_Texture * texture, SDL_BlendMode *blendMode);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateTexture(SDL_Texture * texture, const SDL_Rect * rect, const void *pixels, int pitch);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateYUVTexture(SDL_Texture * texture, const SDL_Rect * rect, const Uint8 *Yplane, int Ypitch, const Uint8 *Uplane, int Upitch, const Uint8 *Vplane, int Vpitch);
// TODO: extern DECLSPEC int SDLCALL SDL_LockTexture(SDL_Texture * texture, const SDL_Rect * rect, void **pixels, int *pitch);
// TODO: extern DECLSPEC void SDLCALL SDL_UnlockTexture(SDL_Texture * texture);

// extern DECLSPEC SDL_bool SDLCALL SDL_RenderTargetSupported(SDL_Renderer *renderer);
MODULE_EXPORT_IMPLEMENT(SDL_RenderTargetSupported)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_bool supported = SDL_RenderTargetSupported(renderer);
	NanReturnValue(NanNew<Boolean>(supported != SDL_FALSE));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);
// TODO: extern DECLSPEC SDL_Texture * SDLCALL SDL_GetRenderTarget(SDL_Renderer *renderer);

// extern DECLSPEC int SDLCALL SDL_RenderSetLogicalSize(SDL_Renderer * renderer, int w, int h);
MODULE_EXPORT_IMPLEMENT(SDL_RenderSetLogicalSize)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	int w = args[1]->Int32Value();
	int h = args[2]->Int32Value();
	int err = SDL_RenderSetLogicalSize(renderer, w, h);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC void SDLCALL SDL_RenderGetLogicalSize(SDL_Renderer * renderer, int *w, int *h);

// extern DECLSPEC int SDLCALL SDL_RenderSetViewport(SDL_Renderer * renderer, const SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderSetViewport)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	int err = SDL_RenderSetViewport(renderer, rect);
	NanReturnValue(NanNew<Integer>(err));
}

// extern DECLSPEC void SDLCALL SDL_RenderGetViewport(SDL_Renderer * renderer, SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderGetViewport)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	SDL_RenderGetViewport(renderer, rect);
	NanReturnUndefined();
}

// extern DECLSPEC int SDLCALL SDL_RenderSetClipRect(SDL_Renderer * renderer, const SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderSetClipRect)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	int err = SDL_RenderSetClipRect(renderer, rect);
	NanReturnValue(NanNew<Integer>(err));
}

// extern DECLSPEC void SDLCALL SDL_RenderGetClipRect(SDL_Renderer * renderer, SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderGetClipRect)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	SDL_RenderGetClipRect(renderer, rect);
	NanReturnUndefined();
}

// extern DECLSPEC int SDLCALL SDL_RenderSetScale(SDL_Renderer * renderer, float scaleX, float scaleY);
MODULE_EXPORT_IMPLEMENT(SDL_RenderSetScale)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	float scaleX = (float) args[1]->NumberValue();
	float scaleY = (float) args[2]->NumberValue();
	int err = SDL_RenderSetScale(renderer, scaleX, scaleY);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC void SDLCALL SDL_RenderGetScale(SDL_Renderer * renderer, float *scaleX, float *scaleY);

// extern DECLSPEC int SDL_SetRenderDrawColor(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
MODULE_EXPORT_IMPLEMENT(SDL_SetRenderDrawColor)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	::Uint8 r = (::Uint8) args[1]->Uint32Value();
	::Uint8 g = (::Uint8) args[2]->Uint32Value();
	::Uint8 b = (::Uint8) args[3]->Uint32Value();
	::Uint8 a = (::Uint8) args[4]->Uint32Value();
	int err = SDL_SetRenderDrawColor(renderer, r, g, b, a);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDL_GetRenderDrawColor(SDL_Renderer * renderer, Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a);

// extern DECLSPEC int SDLCALL SDL_SetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode blendMode);
MODULE_EXPORT_IMPLEMENT(SDL_SetRenderDrawBlendMode)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_BlendMode mode = (SDL_BlendMode) args[1]->Uint32Value();
	int err = SDL_SetRenderDrawBlendMode(renderer, mode);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode *blendMode);

// extern DECLSPEC int SDLCALL SDL_RenderClear(SDL_Renderer * renderer);
MODULE_EXPORT_IMPLEMENT(SDL_RenderClear)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	int err = SDL_RenderClear(renderer);
	NanReturnValue(NanNew<Integer>(err));
}

// extern DECLSPEC int SDLCALL SDL_RenderDrawPoint(SDL_Renderer * renderer, int x, int y);
MODULE_EXPORT_IMPLEMENT(SDL_RenderDrawPoint)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	int x = args[1]->Int32Value();
	int y = args[2]->Int32Value();
	int err = SDL_RenderDrawPoint(renderer, x, y);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points, int count);

// extern DECLSPEC int SDLCALL SDL_RenderDrawLine(SDL_Renderer * renderer, int x1, int y1, int x2, int y2);
MODULE_EXPORT_IMPLEMENT(SDL_RenderDrawLine)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	int x1 = args[1]->Int32Value();
	int y1 = args[2]->Int32Value();
	int x2 = args[3]->Int32Value();
	int y2 = args[4]->Int32Value();
	int err = SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points, int count);

// extern DECLSPEC int SDLCALL SDL_RenderDrawRect(SDL_Renderer * renderer, const SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderDrawRect)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	int err = SDL_RenderDrawRect(renderer, rect);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count);

// extern DECLSPEC int SDLCALL SDL_RenderFillRect(SDL_Renderer * renderer, const SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_RenderFillRect)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Peek(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	int err = SDL_RenderFillRect(renderer, rect);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const double angle, const SDL_Point *center, const SDL_RendererFlip flip);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 format, void *pixels, int pitch);

// extern DECLSPEC void SDLCALL SDL_RenderPresent(SDL_Renderer * renderer);
MODULE_EXPORT_IMPLEMENT(SDL_RenderPresent)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Drop(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_RenderPresent(renderer);
	NanReturnUndefined();
}

// TODO: extern DECLSPEC void SDLCALL SDL_DestroyTexture(SDL_Texture * texture);

// extern DECLSPEC void SDLCALL SDL_DestroyRenderer(SDL_Renderer * renderer);
MODULE_EXPORT_IMPLEMENT(SDL_DestroyRenderer)
{
	NanScope();
	SDL_Renderer* renderer = WrapRenderer::Drop(args[0]); if (!renderer) { return NanThrowError(NanNew<String>("null SDL_Renderer object")); }
	SDL_DestroyRenderer(renderer);
	NanReturnUndefined();
}

// TODO: extern DECLSPEC int SDLCALL SDL_GL_BindTexture(SDL_Texture *texture, float *texw, float *texh);
// TODO: extern DECLSPEC int SDLCALL SDL_GL_UnbindTexture(SDL_Texture *texture);

// SDL_rwops.h

MODULE_EXPORT_IMPLEMENT(SDL_AllocRW)
{
	NanScope();
	SDL_RWops* rwops = SDL_AllocRW();
	NanReturnValue(WrapRWops::Hold(rwops));
}

MODULE_EXPORT_IMPLEMENT(SDL_FreeRW)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Drop(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	if (rwops && (rwops->type != SDL_RWOPS_UNKNOWN)) { SDL_RWclose(rwops); rwops = NULL; }
	if (rwops) { SDL_FreeRW(rwops); rwops = NULL; }
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(SDL_RWFromFile)
{
	NanScope();
	Local<String> file = Local<String>::Cast(args[0]);
	Local<String> mode = Local<String>::Cast(args[1]);
	SDL_RWops* rwops = SDL_RWFromFile(*String::Utf8Value(file), *String::Utf8Value(mode));
	printf("SDL_RWFromFile: %s %s -> %p\n", *String::Utf8Value(file), *String::Utf8Value(mode), rwops);
	NanReturnValue(WrapRWops::Hold(rwops));
}

MODULE_EXPORT_IMPLEMENT(SDL_RWsize)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Peek(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	Sint64 size = SDL_RWsize(rwops);
	NanReturnValue(NanNew<Integer>((int32_t) size)); // TODO: 64 bit integer
}

MODULE_EXPORT_IMPLEMENT(SDL_RWseek)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Peek(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	Sint64 offset = (Sint64) args[1]->IntegerValue();
	int whence = args[2]->Int32Value();
	Sint64 pos = SDL_RWseek(rwops, offset, whence);
	NanReturnValue(NanNew<Integer>((int32_t) pos)); // TODO: 64 bit integer
}

MODULE_EXPORT_IMPLEMENT(SDL_RWtell)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Peek(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	Sint64 pos = SDL_RWtell(rwops);
	NanReturnValue(NanNew<Integer>((int32_t) pos)); // TODO: 64 bit integer
}

MODULE_EXPORT_IMPLEMENT(SDL_RWread)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Peek(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	Local<Object> _ptr = Local<Object>::Cast(args[1]);
	void* ptr = (void*) _ptr->GetIndexedPropertiesExternalArrayData();
	size_t size = args[2]->Uint32Value();
	size_t maxnum = args[3]->Uint32Value();
	size_t num = 0;
	if ((size * maxnum) <= _ptr->GetIndexedPropertiesExternalArrayDataLength())
	{
		num = SDL_RWread(rwops, ptr, size, maxnum);
	}
	NanReturnValue(NanNew((uint32_t) num));
}

MODULE_EXPORT_IMPLEMENT(SDL_RWwrite)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Peek(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	Local<Object> _ptr = Local<Object>::Cast(args[1]);
	const void* ptr = (const void*) _ptr->GetIndexedPropertiesExternalArrayData();
	size_t size = args[2]->Uint32Value();
	size_t maxnum = args[3]->Uint32Value();
	size_t num = 0;
	if ((size * maxnum) <= _ptr->GetIndexedPropertiesExternalArrayDataLength())
	{
		num = SDL_RWwrite(rwops, ptr, size, maxnum);
	}
	NanReturnValue(NanNew((uint32_t) num));
}

MODULE_EXPORT_IMPLEMENT(SDL_RWclose)
{
	NanScope();
	SDL_RWops* rwops = WrapRWops::Drop(args[0]); if (!rwops) { return NanThrowError(NanNew<String>("null SDL_RWops object")); }
	int err = SDL_RWclose(rwops);
	NanReturnValue(NanNew<Integer>(err));
}

// SDL_scancode.h
// SDL_shape.h
// SDL_stdinc.h

MODULE_EXPORT_IMPLEMENT(SDL_getenv)
{
	NanScope();
	Local<String> name = Local<String>::Cast(args[0]);
	char* env = SDL_getenv(*String::Utf8Value(name));
	NanReturnValue(NanNew<String>(env));
}

MODULE_EXPORT_IMPLEMENT(SDL_setenv)
{
	NanScope();
	Local<String> name = Local<String>::Cast(args[0]);
	Local<String> value = Local<String>::Cast(args[1]);
	int overwrite = args[2]->Int32Value();
	int err = SDL_setenv(*String::Utf8Value(name), *String::Utf8Value(value), overwrite);
	NanReturnValue(NanNew<Integer>(err));
}

// SDL_surface.h

MODULE_EXPORT_IMPLEMENT(SDL_CreateRGBSurface)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	int width = args[1]->Int32Value();
	int height = args[2]->Int32Value();
	int depth = args[3]->Int32Value();
	::Uint32 Rmask = args[4]->Uint32Value();
	::Uint32 Gmask = args[5]->Uint32Value();
	::Uint32 Bmask = args[6]->Uint32Value();
	::Uint32 Amask = args[7]->Uint32Value();
	SDL_Surface* surface = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
	NanReturnValue(WrapSurface::Hold(surface));
}

MODULE_EXPORT_IMPLEMENT(SDL_CreateRGBSurfaceFrom)
{
	NanScope();
	void* pixels = Handle<Object>::Cast(args[0])->GetIndexedPropertiesExternalArrayData();
	int width = args[1]->Int32Value();
	int height = args[2]->Int32Value();
	int depth = args[3]->Int32Value();
	int pitch = args[4]->Int32Value();
	::Uint32 Rmask = args[5]->Uint32Value();
	::Uint32 Gmask = args[6]->Uint32Value();
	::Uint32 Bmask = args[7]->Uint32Value();
	::Uint32 Amask = args[8]->Uint32Value();
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);
	NanReturnValue(WrapSurface::Hold(surface));
}

MODULE_EXPORT_IMPLEMENT(SDL_FreeSurface)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Drop(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_FreeSurface(surface);
	NanReturnUndefined();
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfacePalette(SDL_Surface * surface, SDL_Palette * palette);
// TODO: extern DECLSPEC int SDLCALL SDL_LockSurface(SDL_Surface * surface);
// TODO: extern DECLSPEC void SDLCALL SDL_UnlockSurface(SDL_Surface * surface);

MODULE_EXPORT_IMPLEMENT(SDL_LoadBMP)
{
	NanScope();
	Local<String> file = Local<String>::Cast(args[0]);
	Local<Function> callback = Local<Function>::Cast(args[1]);
	int err = SimpleTask::Run(new TaskLoadBMP(file, callback));
	NanReturnValue(NanNew<v8::Int32>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_SaveBMP)
{
	NanScope();
	Local<Value> surface = args[0];
	Local<String> file = Local<String>::Cast(args[1]);
	Local<Function> callback = Local<Function>::Cast(args[2]);
	int err = SimpleTask::Run(new TaskSaveBMP(surface, file, callback));
	NanReturnValue(NanNew<v8::Int32>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceRLE(SDL_Surface * surface, int flag);
// TODO: extern DECLSPEC int SDLCALL SDL_SetColorKey(SDL_Surface * surface, int flag, Uint32 key);
// TODO: extern DECLSPEC int SDLCALL SDL_GetColorKey(SDL_Surface * surface, Uint32 * key);
// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceColorMod(SDL_Surface * surface, Uint8 r, Uint8 g, Uint8 b);
// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceColorMod(SDL_Surface * surface, Uint8 * r, Uint8 * g, Uint8 * b);
// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceAlphaMod(SDL_Surface * surface, Uint8 alpha);
// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceAlphaMod(SDL_Surface * surface, Uint8 * alpha);

// extern DECLSPEC int SDLCALL SDL_SetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode blendMode);
MODULE_EXPORT_IMPLEMENT(SDL_SetSurfaceBlendMode)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_BlendMode mode = (SDL_BlendMode) args[1]->Uint32Value();
	int err = SDL_SetSurfaceBlendMode(surface, mode);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode *blendMode);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect);
// TODO: extern DECLSPEC void SDLCALL SDL_GetClipRect(SDL_Surface * surface, SDL_Rect * rect);
// TODO: extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurface (SDL_Surface * src, SDL_PixelFormat * fmt, Uint32 flags);

// extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurfaceFormat (SDL_Surface * src, Uint32 pixel_format, Uint32 flags);
MODULE_EXPORT_IMPLEMENT(SDL_ConvertSurfaceFormat)
{
	NanScope();
	SDL_Surface* src_surface = WrapSurface::Peek(args[0]); if (!src_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	::Uint32 format = args[1]->Uint32Value();
	::Uint32 flags = args[2]->Uint32Value();
	SDL_Surface* dst_surface = SDL_ConvertSurfaceFormat(src_surface, format, flags);
	NanReturnValue(WrapSurface::Hold(dst_surface));
}

// TODO: extern DECLSPEC int SDLCALL SDL_ConvertPixels(int width, int height, Uint32 src_format, const void * src, int src_pitch, Uint32 dst_format, void * dst, int dst_pitch);

// extern DECLSPEC int SDLCALL SDL_FillRect(SDL_Surface * dst, const SDL_Rect * rect, Uint32 color);
MODULE_EXPORT_IMPLEMENT(SDL_FillRect)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	::Uint32 color = args[2]->Uint32Value();
	int err = SDL_FillRect(surface, rect, color);
	NanReturnValue(NanNew<Integer>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_FillRects(SDL_Surface * dst, const SDL_Rect * rects, int count, Uint32 color);

// #define SDL_BlitSurface SDL_UpperBlit
// extern DECLSPEC int SDLCALL SDL_UpperBlit(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
// extern DECLSPEC int SDLCALL SDL_LowerBlit(SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
MODULE_EXPORT_IMPLEMENT(SDL_BlitSurface)
{
	NanScope();
	SDL_Surface* src_surface = WrapSurface::Peek(args[0]); if (!src_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* src_rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(args[2]); if (!dst_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* dst_rect = (args[3]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[3]))->GetRect()));
	int err = SDL_BlitSurface(src_surface, src_rect, dst_surface, dst_rect);
	NanReturnValue(NanNew<Integer>(err));
}

// extern DECLSPEC int SDLCALL SDL_SoftStretch(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, const SDL_Rect * dstrect);
MODULE_EXPORT_IMPLEMENT(SDL_SoftStretch)
{
	NanScope();
	SDL_Surface* src_surface = WrapSurface::Peek(args[0]); if (!src_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* src_rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(args[2]); if (!dst_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* dst_rect = (args[3]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[3]))->GetRect()));
	int err = SDL_SoftStretch(src_surface, src_rect, dst_surface, dst_rect);
	NanReturnValue(NanNew<Integer>(err));
}

// #define SDL_BlitScaled SDL_UpperBlitScaled
// extern DECLSPEC int SDLCALL SDL_UpperBlitScaled(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
// extern DECLSPEC int SDLCALL SDL_LowerBlitScaled(SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
MODULE_EXPORT_IMPLEMENT(SDL_BlitScaled)
{
	NanScope();
	SDL_Surface* src_surface = WrapSurface::Peek(args[0]); if (!src_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* src_rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(args[2]); if (!dst_surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_Rect* dst_rect = (args[3]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[3]))->GetRect()));
	int err = SDL_BlitScaled(src_surface, src_rect, dst_surface, dst_rect);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_GetPixel)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	int x = args[1]->Int32Value();
	int y = args[2]->Int32Value();
	::Uint32 pixel = _SDL_GetPixel(surface, x, y);
	NanReturnValue(NanNew(pixel));
}

MODULE_EXPORT_IMPLEMENT(SDL_PutPixel)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	int x = args[1]->Int32Value();
	int y = args[2]->Int32Value();
	::Uint32 pixel = args[3]->Uint32Value();
	_SDL_PutPixel(surface, x, y, pixel);
	NanReturnUndefined();
}

// SDL_system.h

#if defined(__ANDROID__)

MODULE_EXPORT_IMPLEMENT(SDL_AndroidGetInternalStoragePath)
{
	NanScope();
	NanReturnValue(NanNew<String>(SDL_AndroidGetInternalStoragePath()));
}

MODULE_EXPORT_IMPLEMENT(SDL_AndroidGetExternalStorageState)
{
	NanScope();
	NanReturnValue(NanNew<Integer>(SDL_AndroidGetExternalStorageState()));
}

MODULE_EXPORT_IMPLEMENT(SDL_AndroidGetExternalStoragePath)
{
	NanScope();
	NanReturnValue(NanNew<String>(SDL_AndroidGetExternalStoragePath()));
}

#endif // defined(__ANDROID__)

// SDL_syswm.h
// SDL_test.h
// SDL_test_assert.h
// SDL_test_common.h
// SDL_test_compare.h
// SDL_test_crc32.h
// SDL_test_font.h
// SDL_test_fuzzer.h
// SDL_test_harness.h
// SDL_test_images.h
// SDL_test_log.h
// SDL_test_md5.h
// SDL_test_random.h
// SDL_thread.h
// SDL_timer.h

MODULE_EXPORT_IMPLEMENT(SDL_GetTicks)
{
	NanScope();
	::Uint32 ticks = SDL_GetTicks();
	NanReturnValue(NanNew<v8::Uint32>(ticks));
}

MODULE_EXPORT_IMPLEMENT(SDL_Delay)
{
	NanScope();
	::Uint32 ms = args[0]->Uint32Value();
	SDL_Delay(ms);
	NanReturnUndefined();
}

// SDL_touch.h

// TODO: extern DECLSPEC int SDLCALL SDL_GetNumTouchDevices(void);
// TODO: extern DECLSPEC SDL_TouchID SDLCALL SDL_GetTouchDevice(int index);
// TODO: extern DECLSPEC int SDLCALL SDL_GetNumTouchFingers(SDL_TouchID touchID);
// TODO: extern DECLSPEC SDL_Finger * SDLCALL SDL_GetTouchFinger(SDL_TouchID touchID, int index);

// SDL_types.h
// SDL_version.h

MODULE_EXPORT_IMPLEMENT(SDL_GetRevisionNumber)
{
	NanScope();
	NanReturnValue(NanNew<v8::Int32>(SDL_GetRevisionNumber()));
}

// SDL_video.h

// extern DECLSPEC int SDLCALL SDL_GetNumVideoDrivers(void);
MODULE_EXPORT_IMPLEMENT(SDL_GetNumVideoDrivers)
{
	NanScope();
	NanReturnValue(NanNew<v8::Int32>(SDL_GetNumVideoDrivers()));
}

// extern DECLSPEC const char *SDLCALL SDL_GetVideoDriver(int index);
MODULE_EXPORT_IMPLEMENT(SDL_GetVideoDriver)
{
	NanScope();
	int index = args[0]->Int32Value();
	NanReturnValue(NanNew<String>(SDL_GetVideoDriver(index)));
}

// extern DECLSPEC int SDLCALL SDL_VideoInit(const char *driver_name);
MODULE_EXPORT_IMPLEMENT(SDL_VideoInit)
{
	NanScope();
	Local<String> driver_name = Local<String>::Cast(args[0]);
	NanReturnValue(NanNew<v8::Int32>(SDL_VideoInit(*String::Utf8Value(driver_name))));
}

// extern DECLSPEC void SDLCALL SDL_VideoQuit(void);
MODULE_EXPORT_IMPLEMENT(SDL_VideoQuit)
{
	NanScope();
	SDL_VideoQuit();
	NanReturnUndefined();
}

// extern DECLSPEC const char *SDLCALL SDL_GetCurrentVideoDriver(void);
MODULE_EXPORT_IMPLEMENT(SDL_GetCurrentVideoDriver)
{
	NanScope();
	NanReturnValue(NanNew<String>(SDL_GetCurrentVideoDriver()));
}

// extern DECLSPEC int SDLCALL SDL_GetNumVideoDisplays(void);
MODULE_EXPORT_IMPLEMENT(SDL_GetNumVideoDisplays)
{
	NanScope();
	NanReturnValue(NanNew<v8::Int32>(SDL_GetNumVideoDisplays()));
}

// extern DECLSPEC const char * SDLCALL SDL_GetDisplayName(int displayIndex);
MODULE_EXPORT_IMPLEMENT(SDL_GetDisplayName)
{
	NanScope();
	int index = args[0]->Int32Value();
	NanReturnValue(NanNew<String>(SDL_GetDisplayName(index)));
}

// extern DECLSPEC int SDLCALL SDL_GetDisplayBounds(int displayIndex, SDL_Rect * rect);
MODULE_EXPORT_IMPLEMENT(SDL_GetDisplayBounds)
{
	NanScope();
	int index = args[0]->Int32Value();
	SDL_Rect* rect = (args[1]->IsNull())?(NULL):(&(node::ObjectWrap::Unwrap<WrapRect>(Handle<Object>::Cast(args[1]))->GetRect()));
	int err = SDL_GetDisplayBounds(index, rect);
	NanReturnValue(NanNew<v8::Int32>(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetNumDisplayModes(int displayIndex);
// TODO: extern DECLSPEC int SDLCALL SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode * mode);
// TODO: extern DECLSPEC int SDLCALL SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode * mode);

// TODO: extern DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode * mode);
MODULE_EXPORT_IMPLEMENT(SDL_GetCurrentDisplayMode)
{
	NanScope();
	int displayIndex = args[0]->Int32Value();
	//Local<Object> _mode = Local<Object>::Cast(args[1]);
	//SDL_DisplayMode mode;
	//int err = SDL_GetCurrentDisplayMode(displayIndex, &mode);
	//_mode->Set(NanNew<String>("format"), NanNew<v8::Uint32>(mode.format));
	//_mode->Set(NanNew<String>("w"), NanNew<v8::Int32>(mode.w));
	//_mode->Set(NanNew<String>("h"), NanNew<v8::Int32>(mode.h));
	//_mode->Set(NanNew<String>("refresh_rate"), NanNew<v8::Int32>(mode.refresh_rate));
	WrapDisplayMode* mode = node::ObjectWrap::Unwrap<WrapDisplayMode>(Handle<Object>::Cast(args[1]));
	if (mode != NULL)
	{
		int err = SDL_GetCurrentDisplayMode(displayIndex, &(mode->GetDisplayMode()));
		NanReturnValue(NanNew<Integer>(err));
	}
	NanReturnValue(NanNew<Integer>(-1));
}

// TODO: extern DECLSPEC SDL_DisplayMode * SDLCALL SDL_GetClosestDisplayMode(int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest);

// extern DECLSPEC int SDLCALL SDL_GetWindowDisplayIndex(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowDisplayIndex)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	NanReturnValue(NanNew<v8::Int32>(SDL_GetWindowDisplayIndex(window)));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetWindowDisplayMode(SDL_Window * window, const SDL_DisplayMode * mode);
// TODO: extern DECLSPEC int SDLCALL SDL_GetWindowDisplayMode(SDL_Window * window, SDL_DisplayMode * mode);

// extern DECLSPEC Uint32 SDLCALL SDL_GetWindowPixelFormat(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowPixelFormat)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	NanReturnValue(NanNew<v8::Uint32>(SDL_GetWindowPixelFormat(window)));
}

MODULE_EXPORT_IMPLEMENT(SDL_CreateWindow)
{
	NanScope();
	Local<String> title = Local<String>::Cast(args[0]);
	int x = args[1]->Int32Value();
	int y = args[2]->Int32Value();
	int w = args[3]->Int32Value();
	int h = args[4]->Int32Value();
	::Uint32 flags = args[5]->Uint32Value();
	SDL_Window* window = SDL_CreateWindow(*String::Utf8Value(title), x, y, w, h, flags);
	NanReturnValue(WrapWindow::Hold(window));
}

// TODO: extern DECLSPEC SDL_Window * SDLCALL SDL_CreateWindowFrom(const void *data);
// TODO: extern DECLSPEC Uint32 SDLCALL SDL_GetWindowID(SDL_Window * window);
// TODO: extern DECLSPEC SDL_Window * SDLCALL SDL_GetWindowFromID(Uint32 id);

// extern DECLSPEC Uint32 SDLCALL SDL_GetWindowFlags(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowFlags)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	NanReturnValue(NanNew<v8::Uint32>(SDL_GetWindowFlags(window)));
}

MODULE_EXPORT_IMPLEMENT(SDL_SetWindowTitle)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<String> title = Local<String>::Cast(args[1]);
	SDL_SetWindowTitle(window, *String::Utf8Value(title));
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(SDL_GetWindowTitle)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	const char* title = SDL_GetWindowTitle(window);
	NanReturnValue(NanNew<String>(title));
}

MODULE_EXPORT_IMPLEMENT(SDL_SetWindowIcon)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_Surface* surface = WrapSurface::Peek(args[1]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }
	SDL_SetWindowIcon(window, surface);
	NanReturnUndefined();
}

// TODO: extern DECLSPEC void* SDLCALL SDL_SetWindowData(SDL_Window * window, const char *name, void *userdata);
// TODO: extern DECLSPEC void *SDLCALL SDL_GetWindowData(SDL_Window * window, const char *name);

// extern DECLSPEC void SDLCALL SDL_SetWindowPosition(SDL_Window * window, int x, int y);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowPosition)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	int x = args[1]->Int32Value();
	int y = args[2]->Int32Value();
	SDL_SetWindowPosition(window, x, y);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_GetWindowPosition(SDL_Window * window, int *x, int *y);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowPosition)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<Object> position = Local<Object>::Cast(args[1]);
	int x = 0;
	int y = 0;
	SDL_GetWindowPosition(window, &x, &y);
	position->Set(NanNew<String>("x"), NanNew<v8::Int32>(x));
	position->Set(NanNew<String>("y"), NanNew<v8::Int32>(y));
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_SetWindowSize(SDL_Window * window, int w, int h);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	int w = args[1]->Int32Value();
	int h = args[2]->Int32Value();
	SDL_SetWindowSize(window, w, h);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(SDL_GetWindowSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<Object> size = Local<Object>::Cast(args[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(window, &w, &h);
	size->Set(NanNew<String>("w"), NanNew<v8::Int32>(w));
	size->Set(NanNew<String>("h"), NanNew<v8::Int32>(h));
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_SetWindowMinimumSize(SDL_Window * window, int min_w, int min_h);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowMinimumSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	int w = args[1]->Int32Value();
	int h = args[2]->Int32Value();
	SDL_SetWindowMinimumSize(window, w, h);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_GetWindowMinimumSize(SDL_Window * window, int *w, int *h);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowMinimumSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<Object> size = Local<Object>::Cast(args[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowMinimumSize(window, &w, &h);
	size->Set(NanNew<String>("w"), NanNew<v8::Int32>(w));
	size->Set(NanNew<String>("h"), NanNew<v8::Int32>(h));
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_SetWindowMaximumSize(SDL_Window * window, int max_w, int max_h);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowMaximumSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	int w = args[1]->Int32Value();
	int h = args[2]->Int32Value();
	SDL_SetWindowMaximumSize(window, w, h);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_GetWindowMaximumSize(SDL_Window * window, int *w, int *h);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowMaximumSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<Object> size = Local<Object>::Cast(args[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowMaximumSize(window, &w, &h);
	size->Set(NanNew<String>("w"), NanNew<v8::Int32>(w));
	size->Set(NanNew<String>("h"), NanNew<v8::Int32>(h));
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_SetWindowBordered(SDL_Window * window, SDL_bool bordered);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowBordered)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_bool bordered = (SDL_bool)(args[1]->BooleanValue() != false);
	SDL_SetWindowBordered(window, bordered);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_ShowWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_ShowWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_ShowWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_HideWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_HideWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_HideWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_RaiseWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_RaiseWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_RaiseWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_MaximizeWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_MaximizeWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_MaximizeWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_MinimizeWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_MinimizeWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_MinimizeWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_RestoreWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_RestoreWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_RestoreWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC int SDLCALL SDL_SetWindowFullscreen(SDL_Window * window, Uint32 flags);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowFullscreen)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	::Uint32 flags = args[1]->Uint32Value();
	int err = SDL_SetWindowFullscreen(window, flags);
	NanReturnValue(NanNew<v8::Int32>(err));
}

// TODO: extern DECLSPEC SDL_Surface * SDLCALL SDL_GetWindowSurface(SDL_Window * window);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateWindowSurface(SDL_Window * window);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateWindowSurfaceRects(SDL_Window * window, const SDL_Rect * rects, int numrects);

// extern DECLSPEC void SDLCALL SDL_SetWindowGrab(SDL_Window * window, SDL_bool grabbed);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowGrab)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_bool grab = (SDL_bool)(args[1]->BooleanValue() != false);
	SDL_SetWindowGrab(window, grab);
	NanReturnUndefined();
}

// extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowGrab(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowGrab)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_bool grab = SDL_GetWindowGrab(window);
	NanReturnValue(NanNew<Boolean>(grab != SDL_FALSE));
}

// extern DECLSPEC int SDLCALL SDL_SetWindowBrightness(SDL_Window * window, float brightness);
MODULE_EXPORT_IMPLEMENT(SDL_SetWindowBrightness)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	float brightness = (float) args[1]->NumberValue();
	SDL_SetWindowBrightness(window, brightness);
	NanReturnUndefined();
}

// extern DECLSPEC float SDLCALL SDL_GetWindowBrightness(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GetWindowBrightness)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	float brightness = SDL_GetWindowBrightness(window);
	NanReturnValue(NanNew<Number>(brightness));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetWindowGammaRamp(SDL_Window * window, const Uint16 * red, const Uint16 * green, const Uint16 * blue);
// TODO: extern DECLSPEC int SDLCALL SDL_GetWindowGammaRamp(SDL_Window * window, Uint16 * red, Uint16 * green, Uint16 * blue);

// extern DECLSPEC void SDLCALL SDL_DestroyWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_DestroyWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Drop(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_DestroyWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC SDL_bool SDLCALL SDL_IsScreenSaverEnabled(void);
MODULE_EXPORT_IMPLEMENT(SDL_IsScreenSaverEnabled)
{
	NanScope();
	NanReturnValue(NanNew<Boolean>(SDL_IsScreenSaverEnabled() != SDL_FALSE));
}

// extern DECLSPEC void SDLCALL SDL_EnableScreenSaver(void);
MODULE_EXPORT_IMPLEMENT(SDL_EnableScreenSaver)
{
	NanScope();
	SDL_EnableScreenSaver();
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_DisableScreenSaver(void);
MODULE_EXPORT_IMPLEMENT(SDL_DisableScreenSaver)
{
	NanScope();
	SDL_DisableScreenSaver();
	NanReturnUndefined();
}

// TODO: extern DECLSPEC int SDLCALL SDL_GL_LoadLibrary(const char *path);
// TODO: extern DECLSPEC void *SDLCALL SDL_GL_GetProcAddress(const char *proc);
// TODO: extern DECLSPEC void SDLCALL SDL_GL_UnloadLibrary(void);

// extern DECLSPEC SDL_bool SDLCALL SDL_GL_ExtensionSupported(const char *extension);
MODULE_EXPORT_IMPLEMENT(SDL_GL_ExtensionSupported)
{
	NanScope();
	Local<String> extension = Local<String>::Cast(args[0]);
	NanReturnValue(NanNew<Boolean>(SDL_GL_ExtensionSupported(*String::Utf8Value(extension)) != SDL_FALSE));
}

// extern DECLSPEC int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value);
MODULE_EXPORT_IMPLEMENT(SDL_GL_SetAttribute)
{
	NanScope();
	SDL_GLattr attr = (SDL_GLattr) args[0]->Int32Value();
	int value = args[1]->Int32Value();
	int err = SDL_GL_SetAttribute(attr, value);
	NanReturnValue(NanNew<v8::Int32>(err));
}

// extern DECLSPEC int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int *value);
MODULE_EXPORT_IMPLEMENT(SDL_GL_GetAttribute)
{
	NanScope();
	SDL_GLattr attr = (SDL_GLattr) args[0]->Int32Value();
	Local<Array> ret_value = Local<Array>::Cast(args[1]);
	int value = 0;
	int err = SDL_GL_GetAttribute(attr, &value);
	ret_value->Set(0, NanNew<v8::Int32>(value));
	NanReturnValue(NanNew<v8::Int32>(err));
}

// extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GL_CreateContext)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	NanReturnValue(WrapGLContext::Hold(gl_context));
}

// extern DECLSPEC int SDLCALL SDL_GL_MakeCurrent(SDL_Window * window, SDL_GLContext context);
MODULE_EXPORT_IMPLEMENT(SDL_GL_MakeCurrent)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	if (args[1]->IsNull())
	{
		int err = SDL_GL_MakeCurrent(window, NULL);
		NanDisposePersistent(_gl_current_window);
		NanDisposePersistent(_gl_current_context);
		NanReturnValue(NanNew<v8::Int32>(err));
	}
	else
	{
		SDL_GLContext gl_context = WrapGLContext::Peek(args[1]); if (!gl_context) { return NanThrowError(NanNew<String>("null SDL_GLContext object")); }
		int err = SDL_GL_MakeCurrent(window, gl_context);
		NanAssignPersistent(_gl_current_window, args[0]);
		NanAssignPersistent(_gl_current_context, args[1]);
		NanReturnValue(NanNew<v8::Int32>(err));
	}
}

// extern DECLSPEC SDL_Window* SDLCALL SDL_GL_GetCurrentWindow(void);
MODULE_EXPORT_IMPLEMENT(SDL_GL_GetCurrentWindow)
{
	NanScope();
	if (_gl_current_window.IsEmpty())
	{
		SDL_assert(SDL_GL_GetCurrentWindow() == NULL);
		NanReturnNull();
	}
	else
	{
		SDL_assert(SDL_GL_GetCurrentWindow() == WrapWindow::Peek(NanNew<v8::Value>(_gl_current_window)));
		NanReturnValue(NanNew(_gl_current_window));
	}
}

// extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_GetCurrentContext(void);
MODULE_EXPORT_IMPLEMENT(SDL_GL_GetCurrentContext)
{
	NanScope();
	if (_gl_current_context.IsEmpty())
	{
		SDL_assert(SDL_GL_GetCurrentContext() == NULL);
		NanReturnNull();
	}
	else
	{
		SDL_assert(SDL_GL_GetCurrentContext() == WrapWindow::Peek(NanNew<v8::Value>(_gl_current_context)));
		NanReturnValue(NanNew(_gl_current_context));
	}
}

// extern DECLSPEC void SDLCALL SDL_GL_GetDrawableSize(SDL_Window * window, int *w, int *h);
MODULE_EXPORT_IMPLEMENT(SDL_GL_GetDrawableSize)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	Local<Object> size = Local<Object>::Cast(args[1]);
	int w = 0;
	int h = 0;
	SDL_GL_GetDrawableSize(window, &w, &h);
	size->Set(NanNew<String>("w"), NanNew<v8::Int32>(w));
	size->Set(NanNew<String>("h"), NanNew<v8::Int32>(h));
	NanReturnUndefined();
}

// extern DECLSPEC int SDLCALL SDL_GL_SetSwapInterval(int interval);
MODULE_EXPORT_IMPLEMENT(SDL_GL_SetSwapInterval)
{
	NanScope();
	int interval = args[0]->Int32Value();
	int err = SDL_GL_SetSwapInterval(interval);
	NanReturnValue(NanNew<v8::Int32>(err));
}

// extern DECLSPEC int SDLCALL SDL_GL_GetSwapInterval(void);
MODULE_EXPORT_IMPLEMENT(SDL_GL_GetSwapInterval)
{
	NanScope();
	int interval = SDL_GL_GetSwapInterval();
	NanReturnValue(NanNew<v8::Int32>(interval));
}

// extern DECLSPEC void SDLCALL SDL_GL_SwapWindow(SDL_Window * window);
MODULE_EXPORT_IMPLEMENT(SDL_GL_SwapWindow)
{
	NanScope();
	SDL_Window* window = WrapWindow::Peek(args[0]); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
	SDL_GL_SwapWindow(window);
	NanReturnUndefined();
}

// extern DECLSPEC void SDLCALL SDL_GL_DeleteContext(SDL_GLContext context);
MODULE_EXPORT_IMPLEMENT(SDL_GL_DeleteContext)
{
	NanScope();
	if (WrapGLContext::Peek(args[0]) == WrapGLContext::Peek(NanNew<v8::Value>(_gl_current_context)))
	{
		SDL_Window* window = WrapWindow::Peek(NanNew<v8::Value>(_gl_current_window)); if (!window) { return NanThrowError(NanNew<String>("null SDL_Window object")); }
		SDL_GL_MakeCurrent(window, NULL);
		NanDisposePersistent(_gl_current_window);
		NanDisposePersistent(_gl_current_context);
	}
	SDL_GLContext gl_context = WrapGLContext::Drop(args[0]); if (!gl_context) { return NanThrowError(NanNew<String>("null SDL_GLContext object")); }
	SDL_GL_DeleteContext(gl_context);
	NanReturnUndefined();
}

static void SurfaceToImageData(SDL_Surface* surface, void* pixels, int length)
{
	const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format

	if (surface->format->format == format)
	{
		SDL_LockSurface(surface);
		SDL_memcpy(pixels, surface->pixels, length);
		SDL_UnlockSurface(surface);
	}
	else
	{
		SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, format, 0);
		if (rgba)
		{
			SDL_LockSurface(rgba);
			SDL_memcpy(pixels, rgba->pixels, length);
			SDL_UnlockSurface(rgba);
			SDL_FreeSurface(rgba); rgba = NULL;
		}
		else
		{
			::Uint8* u8_pixels = (::Uint8*) pixels;
			int pixel_i = 0;
			::Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_LockSurface(surface);
			for (int y = 0; (y < surface->h) && (pixel_i < length); ++y)
			{
				for (int x = 0; (x < surface->w) && (pixel_i < length); ++x)
				{
					const ::Uint32 pixel = _SDL_GetPixel(surface, x, y);
					SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
					u8_pixels[pixel_i++] = r;
					u8_pixels[pixel_i++] = g;
					u8_pixels[pixel_i++] = b;
					u8_pixels[pixel_i++] = a;
				}
			}
			SDL_UnlockSurface(surface);
		}
	}
}

class SurfaceToImageDataTask : public SimpleTask
{
public:
	Persistent<Value> m_hold_surface;
	Persistent<Function> m_callback;
	Persistent<Object> m_image_data;
	SDL_Surface* m_surface;
	int m_length;
	void* m_pixels;
public:
	SurfaceToImageDataTask(Handle<Value> surface, Handle<Function> callback) :
		m_surface(WrapSurface::Peek(surface)),
		m_length(0),
		m_pixels(NULL)
	{
		NanScope();

		NanAssignPersistent(m_hold_surface, surface);
		NanAssignPersistent(m_callback, callback);
		NanAssignPersistent(m_image_data, NanNew<Object>());

        static const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format

		int w = (m_surface)?(m_surface->w):(0);
		int h = (m_surface)?(m_surface->h):(0);
		m_length = w * h * SDL_BYTESPERPIXEL(format);

		// find the Uint8ClampedArray constructor and create a buffer
		Local<Object> global = NanGetCurrentContext()->Global();
		Local<Function> ctor = Local<Function>::Cast(global->Get(NanNew<String>("Uint8ClampedArray")));
		Handle<Value> argv[] = { NanNew(m_length) };
		Local<Object> data = ctor->NewInstance(countof(argv), argv);

		NanNew<Object>(m_image_data)->ForceSet(NanNew<String>("width"), NanNew<Integer>(w), ReadOnly);
		NanNew<Object>(m_image_data)->ForceSet(NanNew<String>("height"), NanNew<Integer>(h), ReadOnly);
		NanNew<Object>(m_image_data)->ForceSet(NanNew<String>("data"), data, ReadOnly);

		m_pixels = data->GetIndexedPropertiesExternalArrayData();
	}
	~SurfaceToImageDataTask()
	{
		NanDisposePersistent(m_hold_surface);
		NanDisposePersistent(m_callback);
		NanDisposePersistent(m_image_data);
		m_surface = NULL;
		m_length = 0;
		m_pixels = NULL;
	}
	void DoWork()
	{
		if (m_surface)
		{
            SurfaceToImageData(m_surface, m_pixels, m_length);
		}
	}
	void DoAfterWork(int status)
	{
		NanScope();
		Handle<Value> argv[] = { NanNew<Object>(m_image_data) };
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv);
	}
};

MODULE_EXPORT_IMPLEMENT(SDL_EXT_SurfaceToImageDataAsync)
{
	NanScope();
	Local<Value> surface = args[0];
	Local<Function> callback = Local<Function>::Cast(args[1]);
	int err = SimpleTask::Run(new SurfaceToImageDataTask(surface, callback));
	NanReturnValue(NanNew<v8::Int32>(err));
}

MODULE_EXPORT_IMPLEMENT(SDL_EXT_SurfaceToImageData)
{
	NanScope();
	SDL_Surface* surface = WrapSurface::Peek(args[0]); if (!surface) { return NanThrowError(NanNew<String>("null SDL_Surface object")); }

	const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format

	int length = surface->w * surface->h * SDL_BYTESPERPIXEL(format);

	// find the Uint8ClampedArray constructor and create a buffer
	Local<Object> global = NanGetCurrentContext()->Global();
	Local<Function> ctor = Local<Function>::Cast(global->Get(NanNew<String>("Uint8ClampedArray")));
	Handle<Value> argv[] = { NanNew(length) };
	Local<Object> data = ctor->NewInstance(countof(argv), argv);

	Local<Object> image_data = NanNew<Object>();
	image_data->ForceSet(NanNew<String>("width"), NanNew<Integer>(surface->w), ReadOnly);
	image_data->ForceSet(NanNew<String>("height"), NanNew<Integer>(surface->h), ReadOnly);
	image_data->ForceSet(NanNew<String>("data"), data, ReadOnly);

	void* pixels = data->GetIndexedPropertiesExternalArrayData();

    SurfaceToImageData(surface, pixels, length);

	NanReturnValue(image_data);
}

MODULE_EXPORT_IMPLEMENT(SDL_EXT_ImageDataToSurface)
{
	// TODO: make this async?
	NanScope();
	Local<Object> image_data = Local<Object>::Cast(args[0]);
	Local<Integer> width = Local<Integer>::Cast(image_data->Get(NanNew<String>("width")));
	Local<Integer> height = Local<Integer>::Cast(image_data->Get(NanNew<String>("height")));
	Local<Object> data = Local<Object>::Cast(image_data->Get(NanNew<String>("data")));
	const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format
	void* pixels = data->GetIndexedPropertiesExternalArrayData();
	int w = width->Int32Value();
	int h = height->Int32Value();
	int depth = 0;
	int pitch = w * SDL_BYTESPERPIXEL(format);
	::Uint32 Rmask = 0, Gmask = 0, Bmask = 0, Amask = 0;
	SDL_PixelFormatEnumToMasks(format, &depth, &Rmask, &Gmask, &Bmask, &Amask);
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, w, h, depth, pitch, Rmask, Gmask, Bmask, Amask);
	NanReturnValue(WrapSurface::Hold(surface));
}

#if NODE_VERSION_AT_LEAST(0,11,0)
void init(Handle<Object> exports, Handle<Value> module, Handle<Context> context)
#else
void init(Handle<Object> exports/*, Handle<Value> module*/)
#endif
{
	NanScope();

	#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
	SDL_SetMainReady();
	#endif

	WrapDisplayMode::Init(exports);
	WrapColor::Init(exports);
	WrapPoint::Init(exports);
	WrapRect::Init(exports);

	// SDL.h

	MODULE_CONSTANT(exports, SDL_INIT_TIMER);
	MODULE_CONSTANT(exports, SDL_INIT_AUDIO);
	MODULE_CONSTANT(exports, SDL_INIT_VIDEO);
	MODULE_CONSTANT(exports, SDL_INIT_JOYSTICK);
	MODULE_CONSTANT(exports, SDL_INIT_HAPTIC);
	MODULE_CONSTANT(exports, SDL_INIT_GAMECONTROLLER);
	MODULE_CONSTANT(exports, SDL_INIT_EVENTS);
	MODULE_CONSTANT(exports, SDL_INIT_NOPARACHUTE);
	MODULE_CONSTANT(exports, SDL_INIT_EVERYTHING);

	MODULE_EXPORT_APPLY(exports, SDL_Init);
	MODULE_EXPORT_APPLY(exports, SDL_InitSubSystem);
	MODULE_EXPORT_APPLY(exports, SDL_QuitSubSystem);
	MODULE_EXPORT_APPLY(exports, SDL_WasInit);
	MODULE_EXPORT_APPLY(exports, SDL_Quit);

	// SDL_assert.h
	// SDL_atomic.h
	// SDL_audio.h

	MODULE_CONSTANT(exports, AUDIO_U8);
	MODULE_CONSTANT(exports, AUDIO_S8);
	MODULE_CONSTANT(exports, AUDIO_U16LSB);
	MODULE_CONSTANT(exports, AUDIO_S16LSB);
	MODULE_CONSTANT(exports, AUDIO_U16MSB);
	MODULE_CONSTANT(exports, AUDIO_S16MSB);
	MODULE_CONSTANT(exports, AUDIO_U16);
	MODULE_CONSTANT(exports, AUDIO_S16);

	MODULE_CONSTANT(exports, AUDIO_S32LSB);
	MODULE_CONSTANT(exports, AUDIO_S32MSB);
	MODULE_CONSTANT(exports, AUDIO_S32);

	MODULE_CONSTANT(exports, AUDIO_F32LSB);
	MODULE_CONSTANT(exports, AUDIO_F32MSB);
	MODULE_CONSTANT(exports, AUDIO_F32);

	MODULE_CONSTANT(exports, AUDIO_U16SYS);
	MODULE_CONSTANT(exports, AUDIO_S16SYS);
	MODULE_CONSTANT(exports, AUDIO_S32SYS);
	MODULE_CONSTANT(exports, AUDIO_F32SYS);

	MODULE_CONSTANT(exports, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	MODULE_CONSTANT(exports, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	MODULE_CONSTANT(exports, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
	MODULE_CONSTANT(exports, SDL_AUDIO_ALLOW_ANY_CHANGE);

	MODULE_CONSTANT(exports, SDL_MIX_MAXVOLUME);

	// SDL_bits.h

	// SDL_blendmode.h

	// SDL_BlendMode
	Local<Object> BlendMode = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_BlendMode"), BlendMode);
	MODULE_CONSTANT(BlendMode, SDL_BLENDMODE_NONE);
	MODULE_CONSTANT(BlendMode, SDL_BLENDMODE_BLEND);
	MODULE_CONSTANT(BlendMode, SDL_BLENDMODE_ADD);
	MODULE_CONSTANT(BlendMode, SDL_BLENDMODE_MOD);

	// SDL_clipboard.h

	MODULE_EXPORT_APPLY(exports, SDL_SetClipboardText);
	MODULE_EXPORT_APPLY(exports, SDL_GetClipboardText);
	MODULE_EXPORT_APPLY(exports, SDL_HasClipboardText);

	// SDL_config.h
	// SDL_config_android.h
	// SDL_config_iphoneos.h
	// SDL_config_macosx.h
	// SDL_config_minimal.h
	// SDL_config_pandora.h
	// SDL_config_psp.h
	// SDL_config_windows.h
	// SDL_config_wiz.h
	// SDL_copying.h
	// SDL_cpuinfo.h

	// SDL_endian.h

	MODULE_CONSTANT(exports, SDL_LIL_ENDIAN);
	MODULE_CONSTANT(exports, SDL_BIG_ENDIAN);
	MODULE_CONSTANT(exports, SDL_BYTEORDER);

	// SDL_error.h

	MODULE_EXPORT_APPLY(exports, SDL_GetError);
	MODULE_EXPORT_APPLY(exports, SDL_ClearError);

	// SDL_events.h

	WrapCommonEvent::Init(exports);
	WrapWindowEvent::Init(exports);

	MODULE_CONSTANT(exports, SDL_RELEASED);
	MODULE_CONSTANT(exports, SDL_PRESSED);

	// SDL_EventType
	Local<Object> EventType = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_EventType"), EventType);
	MODULE_CONSTANT(EventType, SDL_FIRSTEVENT);
	MODULE_CONSTANT(EventType, SDL_QUIT);
	MODULE_CONSTANT(EventType, SDL_APP_TERMINATING);
	MODULE_CONSTANT(EventType, SDL_APP_LOWMEMORY);
	MODULE_CONSTANT(EventType, SDL_APP_WILLENTERBACKGROUND);
	MODULE_CONSTANT(EventType, SDL_APP_DIDENTERBACKGROUND);
	MODULE_CONSTANT(EventType, SDL_APP_WILLENTERFOREGROUND);
	MODULE_CONSTANT(EventType, SDL_APP_DIDENTERFOREGROUND);
	MODULE_CONSTANT(EventType, SDL_WINDOWEVENT);
	MODULE_CONSTANT(EventType, SDL_SYSWMEVENT);
	MODULE_CONSTANT(EventType, SDL_KEYDOWN);
	MODULE_CONSTANT(EventType, SDL_KEYUP);
	MODULE_CONSTANT(EventType, SDL_TEXTEDITING);
	MODULE_CONSTANT(EventType, SDL_TEXTINPUT);
	MODULE_CONSTANT(EventType, SDL_MOUSEMOTION);
	MODULE_CONSTANT(EventType, SDL_MOUSEBUTTONDOWN);
	MODULE_CONSTANT(EventType, SDL_MOUSEBUTTONUP);
	MODULE_CONSTANT(EventType, SDL_JOYAXISMOTION);
	MODULE_CONSTANT(EventType, SDL_JOYBALLMOTION);
	MODULE_CONSTANT(EventType, SDL_JOYHATMOTION);
	MODULE_CONSTANT(EventType, SDL_JOYBUTTONDOWN);
	MODULE_CONSTANT(EventType, SDL_JOYBUTTONUP);
	MODULE_CONSTANT(EventType, SDL_JOYDEVICEADDED);
	MODULE_CONSTANT(EventType, SDL_JOYDEVICEREMOVED);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERAXISMOTION);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERBUTTONDOWN);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERBUTTONUP);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERDEVICEADDED);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERDEVICEREMOVED);
	MODULE_CONSTANT(EventType, SDL_CONTROLLERDEVICEREMAPPED);
	MODULE_CONSTANT(EventType, SDL_FINGERDOWN);
	MODULE_CONSTANT(EventType, SDL_FINGERUP);
	MODULE_CONSTANT(EventType, SDL_FINGERMOTION);
	MODULE_CONSTANT(EventType, SDL_DOLLARGESTURE);
	MODULE_CONSTANT(EventType, SDL_DOLLARRECORD);
	MODULE_CONSTANT(EventType, SDL_MULTIGESTURE);
	MODULE_CONSTANT(EventType, SDL_CLIPBOARDUPDATE);
	MODULE_CONSTANT(EventType, SDL_DROPFILE);
	MODULE_CONSTANT(EventType, SDL_USEREVENT);
	MODULE_CONSTANT(EventType, SDL_LASTEVENT);

	MODULE_EXPORT_APPLY(exports, SDL_PollEvent);

	MODULE_CONSTANT(exports, SDL_QUERY);
	MODULE_CONSTANT(exports, SDL_IGNORE);
	MODULE_CONSTANT(exports, SDL_DISABLE);
	MODULE_CONSTANT(exports, SDL_ENABLE);

	// SDL_gamecontroller.h
	// SDL_gesture.h
	// SDL_haptic.h

	// SDL_hints.h

	MODULE_CONSTANT_STRING(exports, SDL_HINT_FRAMEBUFFER_ACCELERATION);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_DRIVER);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_OPENGL_SHADERS);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_DIRECT3D_THREADSAFE);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_DIRECT3D11_DEBUG);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_SCALE_QUALITY);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_RENDER_VSYNC);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_ALLOW_SCREENSAVER);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_X11_XVIDMODE);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_X11_XINERAMA);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_X11_XRANDR);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_GRAB_KEYBOARD);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_MOUSE_RELATIVE_MODE_WARP);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_IDLE_TIMER_DISABLED);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_ORIENTATIONS);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_ACCELEROMETER_AS_JOYSTICK);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_XINPUT_ENABLED);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_GAMECONTROLLERCONFIG);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_ALLOW_TOPMOST);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_TIMER_RESOLUTION);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_HIGHDPI_DISABLED);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_WIN_D3DCOMPILER);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_WINRT_PRIVACY_POLICY_URL);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_WINRT_PRIVACY_POLICY_LABEL);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_WINRT_HANDLE_BACK_BUTTON);
	MODULE_CONSTANT_STRING(exports, SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_IME_INTERNAL_EDITING);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT);
	//MODULE_CONSTANT_STRING(exports, SDL_HINT_NO_SIGNAL_HANDLERS);

	// SDL_HintPriority
	Local<Object> HintPriority = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_HintPriority"), HintPriority);
	MODULE_CONSTANT(HintPriority, SDL_HINT_DEFAULT);
	MODULE_CONSTANT(HintPriority, SDL_HINT_NORMAL);
	MODULE_CONSTANT(HintPriority, SDL_HINT_OVERRIDE);

	MODULE_EXPORT_APPLY(exports, SDL_SetHintWithPriority);
	MODULE_EXPORT_APPLY(exports, SDL_SetHint);
	MODULE_EXPORT_APPLY(exports, SDL_GetHint);
	MODULE_EXPORT_APPLY(exports, SDL_ClearHints);

	// SDL_joystick.h

	MODULE_CONSTANT(exports, SDL_HAT_CENTERED);
	MODULE_CONSTANT(exports, SDL_HAT_UP);
	MODULE_CONSTANT(exports, SDL_HAT_RIGHT);
	MODULE_CONSTANT(exports, SDL_HAT_DOWN);
	MODULE_CONSTANT(exports, SDL_HAT_LEFT);
	MODULE_CONSTANT(exports, SDL_HAT_RIGHTUP);
	MODULE_CONSTANT(exports, SDL_HAT_RIGHTDOWN);
	MODULE_CONSTANT(exports, SDL_HAT_LEFTUP);
	MODULE_CONSTANT(exports, SDL_HAT_LEFTDOWN);

	MODULE_EXPORT_APPLY(exports, SDL_NumJoysticks);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickNameForIndex);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickOpen);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickName);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickGetAttached);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickInstanceID);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickNumAxes);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickNumBalls);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickNumHats);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickNumButtons);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickUpdate);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickEventState);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickGetAxis);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickGetBall);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickGetHat);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickGetButton);
	MODULE_EXPORT_APPLY(exports, SDL_JoystickClose);

	// SDL_keyboard.h
	// SDL_keycode.h

	// SDL_Keycode
	Local<Object> Keycode = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_Keycode"), Keycode);
	MODULE_CONSTANT(Keycode, SDLK_UNKNOWN);
	MODULE_CONSTANT(Keycode, SDLK_RETURN);
	MODULE_CONSTANT(Keycode, SDLK_ESCAPE);
	MODULE_CONSTANT(Keycode, SDLK_BACKSPACE);
	MODULE_CONSTANT(Keycode, SDLK_TAB);
	MODULE_CONSTANT(Keycode, SDLK_SPACE);
	MODULE_CONSTANT(Keycode, SDLK_EXCLAIM);
	MODULE_CONSTANT(Keycode, SDLK_QUOTEDBL);
	MODULE_CONSTANT(Keycode, SDLK_HASH);
	MODULE_CONSTANT(Keycode, SDLK_PERCENT);
	MODULE_CONSTANT(Keycode, SDLK_DOLLAR);
	MODULE_CONSTANT(Keycode, SDLK_AMPERSAND);
	MODULE_CONSTANT(Keycode, SDLK_QUOTE);
	MODULE_CONSTANT(Keycode, SDLK_LEFTPAREN);
	MODULE_CONSTANT(Keycode, SDLK_RIGHTPAREN);
	MODULE_CONSTANT(Keycode, SDLK_ASTERISK);
	MODULE_CONSTANT(Keycode, SDLK_PLUS);
	MODULE_CONSTANT(Keycode, SDLK_COMMA);
	MODULE_CONSTANT(Keycode, SDLK_MINUS);
	MODULE_CONSTANT(Keycode, SDLK_PERIOD);
	MODULE_CONSTANT(Keycode, SDLK_SLASH);
	MODULE_CONSTANT(Keycode, SDLK_0);
	MODULE_CONSTANT(Keycode, SDLK_1);
	MODULE_CONSTANT(Keycode, SDLK_2);
	MODULE_CONSTANT(Keycode, SDLK_3);
	MODULE_CONSTANT(Keycode, SDLK_4);
	MODULE_CONSTANT(Keycode, SDLK_5);
	MODULE_CONSTANT(Keycode, SDLK_6);
	MODULE_CONSTANT(Keycode, SDLK_7);
	MODULE_CONSTANT(Keycode, SDLK_8);
	MODULE_CONSTANT(Keycode, SDLK_9);
	MODULE_CONSTANT(Keycode, SDLK_COLON);
	MODULE_CONSTANT(Keycode, SDLK_SEMICOLON);
	MODULE_CONSTANT(Keycode, SDLK_LESS);
	MODULE_CONSTANT(Keycode, SDLK_EQUALS);
	MODULE_CONSTANT(Keycode, SDLK_GREATER);
	MODULE_CONSTANT(Keycode, SDLK_QUESTION);
	MODULE_CONSTANT(Keycode, SDLK_AT);
	MODULE_CONSTANT(Keycode, SDLK_LEFTBRACKET);
	MODULE_CONSTANT(Keycode, SDLK_BACKSLASH);
	MODULE_CONSTANT(Keycode, SDLK_RIGHTBRACKET);
	MODULE_CONSTANT(Keycode, SDLK_CARET);
	MODULE_CONSTANT(Keycode, SDLK_UNDERSCORE);
	MODULE_CONSTANT(Keycode, SDLK_BACKQUOTE);
	MODULE_CONSTANT(Keycode, SDLK_a);
	MODULE_CONSTANT(Keycode, SDLK_b);
	MODULE_CONSTANT(Keycode, SDLK_c);
	MODULE_CONSTANT(Keycode, SDLK_d);
	MODULE_CONSTANT(Keycode, SDLK_e);
	MODULE_CONSTANT(Keycode, SDLK_f);
	MODULE_CONSTANT(Keycode, SDLK_g);
	MODULE_CONSTANT(Keycode, SDLK_h);
	MODULE_CONSTANT(Keycode, SDLK_i);
	MODULE_CONSTANT(Keycode, SDLK_j);
	MODULE_CONSTANT(Keycode, SDLK_k);
	MODULE_CONSTANT(Keycode, SDLK_l);
	MODULE_CONSTANT(Keycode, SDLK_m);
	MODULE_CONSTANT(Keycode, SDLK_n);
	MODULE_CONSTANT(Keycode, SDLK_o);
	MODULE_CONSTANT(Keycode, SDLK_p);
	MODULE_CONSTANT(Keycode, SDLK_q);
	MODULE_CONSTANT(Keycode, SDLK_r);
	MODULE_CONSTANT(Keycode, SDLK_s);
	MODULE_CONSTANT(Keycode, SDLK_t);
	MODULE_CONSTANT(Keycode, SDLK_u);
	MODULE_CONSTANT(Keycode, SDLK_v);
	MODULE_CONSTANT(Keycode, SDLK_w);
	MODULE_CONSTANT(Keycode, SDLK_x);
	MODULE_CONSTANT(Keycode, SDLK_y);
	MODULE_CONSTANT(Keycode, SDLK_z);
	MODULE_CONSTANT(Keycode, SDLK_CAPSLOCK);
	MODULE_CONSTANT(Keycode, SDLK_F1);
	MODULE_CONSTANT(Keycode, SDLK_F2);
	MODULE_CONSTANT(Keycode, SDLK_F3);
	MODULE_CONSTANT(Keycode, SDLK_F4);
	MODULE_CONSTANT(Keycode, SDLK_F5);
	MODULE_CONSTANT(Keycode, SDLK_F6);
	MODULE_CONSTANT(Keycode, SDLK_F7);
	MODULE_CONSTANT(Keycode, SDLK_F8);
	MODULE_CONSTANT(Keycode, SDLK_F9);
	MODULE_CONSTANT(Keycode, SDLK_F10);
	MODULE_CONSTANT(Keycode, SDLK_F11);
	MODULE_CONSTANT(Keycode, SDLK_F12);
	MODULE_CONSTANT(Keycode, SDLK_PRINTSCREEN);
	MODULE_CONSTANT(Keycode, SDLK_SCROLLLOCK);
	MODULE_CONSTANT(Keycode, SDLK_PAUSE);
	MODULE_CONSTANT(Keycode, SDLK_INSERT);
	MODULE_CONSTANT(Keycode, SDLK_HOME);
	MODULE_CONSTANT(Keycode, SDLK_PAGEUP);
	MODULE_CONSTANT(Keycode, SDLK_DELETE);
	MODULE_CONSTANT(Keycode, SDLK_END);
	MODULE_CONSTANT(Keycode, SDLK_PAGEDOWN);
	MODULE_CONSTANT(Keycode, SDLK_RIGHT);
	MODULE_CONSTANT(Keycode, SDLK_LEFT);
	MODULE_CONSTANT(Keycode, SDLK_DOWN);
	MODULE_CONSTANT(Keycode, SDLK_UP);
	MODULE_CONSTANT(Keycode, SDLK_NUMLOCKCLEAR);
	MODULE_CONSTANT(Keycode, SDLK_KP_DIVIDE);
	MODULE_CONSTANT(Keycode, SDLK_KP_MULTIPLY);
	MODULE_CONSTANT(Keycode, SDLK_KP_MINUS);
	MODULE_CONSTANT(Keycode, SDLK_KP_PLUS);
	MODULE_CONSTANT(Keycode, SDLK_KP_ENTER);
	MODULE_CONSTANT(Keycode, SDLK_KP_1);
	MODULE_CONSTANT(Keycode, SDLK_KP_2);
	MODULE_CONSTANT(Keycode, SDLK_KP_3);
	MODULE_CONSTANT(Keycode, SDLK_KP_4);
	MODULE_CONSTANT(Keycode, SDLK_KP_5);
	MODULE_CONSTANT(Keycode, SDLK_KP_6);
	MODULE_CONSTANT(Keycode, SDLK_KP_7);
	MODULE_CONSTANT(Keycode, SDLK_KP_8);
	MODULE_CONSTANT(Keycode, SDLK_KP_9);
	MODULE_CONSTANT(Keycode, SDLK_KP_0);
	MODULE_CONSTANT(Keycode, SDLK_KP_PERIOD);
	MODULE_CONSTANT(Keycode, SDLK_APPLICATION);
	MODULE_CONSTANT(Keycode, SDLK_POWER);
	MODULE_CONSTANT(Keycode, SDLK_KP_EQUALS);
	MODULE_CONSTANT(Keycode, SDLK_F13);
	MODULE_CONSTANT(Keycode, SDLK_F14);
	MODULE_CONSTANT(Keycode, SDLK_F15);
	MODULE_CONSTANT(Keycode, SDLK_F16);
	MODULE_CONSTANT(Keycode, SDLK_F17);
	MODULE_CONSTANT(Keycode, SDLK_F18);
	MODULE_CONSTANT(Keycode, SDLK_F19);
	MODULE_CONSTANT(Keycode, SDLK_F20);
	MODULE_CONSTANT(Keycode, SDLK_F21);
	MODULE_CONSTANT(Keycode, SDLK_F22);
	MODULE_CONSTANT(Keycode, SDLK_F23);
	MODULE_CONSTANT(Keycode, SDLK_F24);
	MODULE_CONSTANT(Keycode, SDLK_EXECUTE);
	MODULE_CONSTANT(Keycode, SDLK_HELP);
	MODULE_CONSTANT(Keycode, SDLK_MENU);
	MODULE_CONSTANT(Keycode, SDLK_SELECT);
	MODULE_CONSTANT(Keycode, SDLK_STOP);
	MODULE_CONSTANT(Keycode, SDLK_AGAIN);
	MODULE_CONSTANT(Keycode, SDLK_UNDO);
	MODULE_CONSTANT(Keycode, SDLK_CUT);
	MODULE_CONSTANT(Keycode, SDLK_COPY);
	MODULE_CONSTANT(Keycode, SDLK_PASTE);
	MODULE_CONSTANT(Keycode, SDLK_FIND);
	MODULE_CONSTANT(Keycode, SDLK_MUTE);
	MODULE_CONSTANT(Keycode, SDLK_VOLUMEUP);
	MODULE_CONSTANT(Keycode, SDLK_VOLUMEDOWN);
	MODULE_CONSTANT(Keycode, SDLK_KP_COMMA);
	MODULE_CONSTANT(Keycode, SDLK_KP_EQUALSAS400);
	MODULE_CONSTANT(Keycode, SDLK_ALTERASE);
	MODULE_CONSTANT(Keycode, SDLK_SYSREQ);
	MODULE_CONSTANT(Keycode, SDLK_CANCEL);
	MODULE_CONSTANT(Keycode, SDLK_CLEAR);
	MODULE_CONSTANT(Keycode, SDLK_PRIOR);
	MODULE_CONSTANT(Keycode, SDLK_RETURN2);
	MODULE_CONSTANT(Keycode, SDLK_SEPARATOR);
	MODULE_CONSTANT(Keycode, SDLK_OUT);
	MODULE_CONSTANT(Keycode, SDLK_OPER);
	MODULE_CONSTANT(Keycode, SDLK_CLEARAGAIN);
	MODULE_CONSTANT(Keycode, SDLK_CRSEL);
	MODULE_CONSTANT(Keycode, SDLK_EXSEL);
	MODULE_CONSTANT(Keycode, SDLK_KP_00);
	MODULE_CONSTANT(Keycode, SDLK_KP_000);
	MODULE_CONSTANT(Keycode, SDLK_THOUSANDSSEPARATOR);
	MODULE_CONSTANT(Keycode, SDLK_DECIMALSEPARATOR);
	MODULE_CONSTANT(Keycode, SDLK_CURRENCYUNIT);
	MODULE_CONSTANT(Keycode, SDLK_CURRENCYSUBUNIT);
	MODULE_CONSTANT(Keycode, SDLK_KP_LEFTPAREN);
	MODULE_CONSTANT(Keycode, SDLK_KP_RIGHTPAREN);
	MODULE_CONSTANT(Keycode, SDLK_KP_LEFTBRACE);
	MODULE_CONSTANT(Keycode, SDLK_KP_RIGHTBRACE);
	MODULE_CONSTANT(Keycode, SDLK_KP_TAB);
	MODULE_CONSTANT(Keycode, SDLK_KP_BACKSPACE);
	MODULE_CONSTANT(Keycode, SDLK_KP_A);
	MODULE_CONSTANT(Keycode, SDLK_KP_B);
	MODULE_CONSTANT(Keycode, SDLK_KP_C);
	MODULE_CONSTANT(Keycode, SDLK_KP_D);
	MODULE_CONSTANT(Keycode, SDLK_KP_E);
	MODULE_CONSTANT(Keycode, SDLK_KP_F);
	MODULE_CONSTANT(Keycode, SDLK_KP_XOR);
	MODULE_CONSTANT(Keycode, SDLK_KP_POWER);
	MODULE_CONSTANT(Keycode, SDLK_KP_PERCENT);
	MODULE_CONSTANT(Keycode, SDLK_KP_LESS);
	MODULE_CONSTANT(Keycode, SDLK_KP_GREATER);
	MODULE_CONSTANT(Keycode, SDLK_KP_AMPERSAND);
	MODULE_CONSTANT(Keycode, SDLK_KP_DBLAMPERSAND);
	MODULE_CONSTANT(Keycode, SDLK_KP_VERTICALBAR);
	MODULE_CONSTANT(Keycode, SDLK_KP_DBLVERTICALBAR);
	MODULE_CONSTANT(Keycode, SDLK_KP_COLON);
	MODULE_CONSTANT(Keycode, SDLK_KP_HASH);
	MODULE_CONSTANT(Keycode, SDLK_KP_SPACE);
	MODULE_CONSTANT(Keycode, SDLK_KP_AT);
	MODULE_CONSTANT(Keycode, SDLK_KP_EXCLAM);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMSTORE);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMRECALL);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMCLEAR);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMADD);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMSUBTRACT);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMMULTIPLY);
	MODULE_CONSTANT(Keycode, SDLK_KP_MEMDIVIDE);
	MODULE_CONSTANT(Keycode, SDLK_KP_PLUSMINUS);
	MODULE_CONSTANT(Keycode, SDLK_KP_CLEAR);
	MODULE_CONSTANT(Keycode, SDLK_KP_CLEARENTRY);
	MODULE_CONSTANT(Keycode, SDLK_KP_BINARY);
	MODULE_CONSTANT(Keycode, SDLK_KP_OCTAL);
	MODULE_CONSTANT(Keycode, SDLK_KP_DECIMAL);
	MODULE_CONSTANT(Keycode, SDLK_KP_HEXADECIMAL);
	MODULE_CONSTANT(Keycode, SDLK_LCTRL);
	MODULE_CONSTANT(Keycode, SDLK_LSHIFT);
	MODULE_CONSTANT(Keycode, SDLK_LALT);
	MODULE_CONSTANT(Keycode, SDLK_LGUI);
	MODULE_CONSTANT(Keycode, SDLK_RCTRL);
	MODULE_CONSTANT(Keycode, SDLK_RSHIFT);
	MODULE_CONSTANT(Keycode, SDLK_RALT);
	MODULE_CONSTANT(Keycode, SDLK_RGUI);
	MODULE_CONSTANT(Keycode, SDLK_MODE);
	MODULE_CONSTANT(Keycode, SDLK_AUDIONEXT);
	MODULE_CONSTANT(Keycode, SDLK_AUDIOPREV);
	MODULE_CONSTANT(Keycode, SDLK_AUDIOSTOP);
	MODULE_CONSTANT(Keycode, SDLK_AUDIOPLAY);
	MODULE_CONSTANT(Keycode, SDLK_AUDIOMUTE);
	MODULE_CONSTANT(Keycode, SDLK_MEDIASELECT);
	MODULE_CONSTANT(Keycode, SDLK_WWW);
	MODULE_CONSTANT(Keycode, SDLK_MAIL);
	MODULE_CONSTANT(Keycode, SDLK_CALCULATOR);
	MODULE_CONSTANT(Keycode, SDLK_COMPUTER);
	MODULE_CONSTANT(Keycode, SDLK_AC_SEARCH);
	MODULE_CONSTANT(Keycode, SDLK_AC_HOME);
	MODULE_CONSTANT(Keycode, SDLK_AC_BACK);
	MODULE_CONSTANT(Keycode, SDLK_AC_FORWARD);
	MODULE_CONSTANT(Keycode, SDLK_AC_STOP);
	MODULE_CONSTANT(Keycode, SDLK_AC_REFRESH);
	MODULE_CONSTANT(Keycode, SDLK_AC_BOOKMARKS);
	MODULE_CONSTANT(Keycode, SDLK_BRIGHTNESSDOWN);
	MODULE_CONSTANT(Keycode, SDLK_BRIGHTNESSUP);
	MODULE_CONSTANT(Keycode, SDLK_DISPLAYSWITCH);
	MODULE_CONSTANT(Keycode, SDLK_KBDILLUMTOGGLE);
	MODULE_CONSTANT(Keycode, SDLK_KBDILLUMDOWN);
	MODULE_CONSTANT(Keycode, SDLK_KBDILLUMUP);
	MODULE_CONSTANT(Keycode, SDLK_EJECT);
	MODULE_CONSTANT(Keycode, SDLK_SLEEP);

	// SDL_Keymod
	Local<Object> Keymod = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_Keymod"), Keymod);
	MODULE_CONSTANT(Keymod, KMOD_NONE);
	MODULE_CONSTANT(Keymod, KMOD_LSHIFT);
	MODULE_CONSTANT(Keymod, KMOD_RSHIFT);
	MODULE_CONSTANT(Keymod, KMOD_LCTRL);
	MODULE_CONSTANT(Keymod, KMOD_RCTRL);
	MODULE_CONSTANT(Keymod, KMOD_LALT);
	MODULE_CONSTANT(Keymod, KMOD_RALT);
	MODULE_CONSTANT(Keymod, KMOD_LGUI);
	MODULE_CONSTANT(Keymod, KMOD_RGUI);
	MODULE_CONSTANT(Keymod, KMOD_NUM);
	MODULE_CONSTANT(Keymod, KMOD_CAPS);
	MODULE_CONSTANT(Keymod, KMOD_MODE);
	MODULE_CONSTANT(Keymod, KMOD_RESERVED);
	MODULE_CONSTANT(Keymod, KMOD_CTRL);
	MODULE_CONSTANT(Keymod, KMOD_SHIFT);
	MODULE_CONSTANT(Keymod, KMOD_ALT);
	MODULE_CONSTANT(Keymod, KMOD_GUI);

	// SDL_loadso.h
	// SDL_log.h
	// SDL_main.h
	// SDL_messagebox.h
	// SDL_mouse.h

	MODULE_CONSTANT(exports, SDL_BUTTON_LEFT);
	MODULE_CONSTANT(exports, SDL_BUTTON_MIDDLE);
	MODULE_CONSTANT(exports, SDL_BUTTON_RIGHT);
	MODULE_CONSTANT(exports, SDL_BUTTON_X1);
	MODULE_CONSTANT(exports, SDL_BUTTON_X2);
	MODULE_CONSTANT(exports, SDL_BUTTON_LMASK);
	MODULE_CONSTANT(exports, SDL_BUTTON_MMASK);
	MODULE_CONSTANT(exports, SDL_BUTTON_RMASK);
	MODULE_CONSTANT(exports, SDL_BUTTON_X1MASK);
	MODULE_CONSTANT(exports, SDL_BUTTON_X2MASK);

	// SDL_mutex.h
	// SDL_name.h
	// SDL_opengl.h
	// SDL_opengles.h
	// SDL_opengles2.h
	// SDL_pixels.h

	MODULE_CONSTANT(exports, SDL_ALPHA_OPAQUE);
	MODULE_CONSTANT(exports, SDL_ALPHA_TRANSPARENT);

	MODULE_CONSTANT(exports, SDL_PIXELTYPE_UNKNOWN);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_INDEX1);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_INDEX4);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_INDEX8);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_PACKED8);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_PACKED16);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_PACKED32);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_ARRAYU8);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_ARRAYU16);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_ARRAYU32);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_ARRAYF16);
	MODULE_CONSTANT(exports, SDL_PIXELTYPE_ARRAYF32);

	MODULE_CONSTANT(exports, SDL_BITMAPORDER_NONE);
	MODULE_CONSTANT(exports, SDL_BITMAPORDER_4321);
	MODULE_CONSTANT(exports, SDL_BITMAPORDER_1234);

	MODULE_CONSTANT(exports, SDL_PACKEDORDER_NONE);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_XRGB);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_RGBX);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_ARGB);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_RGBA);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_XBGR);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_BGRX);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_ABGR);
	MODULE_CONSTANT(exports, SDL_PACKEDORDER_BGRA);

	MODULE_CONSTANT(exports, SDL_ARRAYORDER_NONE);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_RGB);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_RGBA);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_ARGB);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_BGR);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_BGRA);
	MODULE_CONSTANT(exports, SDL_ARRAYORDER_ABGR);

	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_NONE);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_332);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_4444);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_1555);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_5551);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_565);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_8888);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_2101010);
	MODULE_CONSTANT(exports, SDL_PACKEDLAYOUT_1010102);

	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_UNKNOWN);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_INDEX1LSB);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_INDEX1MSB);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_INDEX4LSB);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_INDEX4MSB);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_INDEX8);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB332);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB444);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB555);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGR555);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ARGB4444);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGBA4444);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ABGR4444);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGRA4444);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ARGB1555);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGBA5551);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ABGR1555);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGRA5551);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB565);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGR565);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB24);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGR24);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGB888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGBX8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGR888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGRX8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ARGB8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_RGBA8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ABGR8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_BGRA8888);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_ARGB2101010);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_YV12);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_IYUV);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_YUY2);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_UYVY);
	MODULE_CONSTANT(exports, SDL_PIXELFORMAT_YVYU);

	MODULE_EXPORT_APPLY(exports, SDL_PIXELFLAG);
	MODULE_EXPORT_APPLY(exports, SDL_PIXELTYPE);
	MODULE_EXPORT_APPLY(exports, SDL_PIXELORDER);
	MODULE_EXPORT_APPLY(exports, SDL_PIXELLAYOUT);
	MODULE_EXPORT_APPLY(exports, SDL_BITSPERPIXEL);
	MODULE_EXPORT_APPLY(exports, SDL_BYTESPERPIXEL);
	MODULE_EXPORT_APPLY(exports, SDL_ISPIXELFORMAT_INDEXED);
	MODULE_EXPORT_APPLY(exports, SDL_ISPIXELFORMAT_ALPHA);
	MODULE_EXPORT_APPLY(exports, SDL_ISPIXELFORMAT_FOURCC);

	MODULE_EXPORT_APPLY(exports, SDL_GetPixelFormatName);
	MODULE_EXPORT_APPLY(exports, SDL_PixelFormatEnumToMasks);
	MODULE_EXPORT_APPLY(exports, SDL_MasksToPixelFormatEnum);
	MODULE_EXPORT_APPLY(exports, SDL_AllocFormat);
	MODULE_EXPORT_APPLY(exports, SDL_FreeFormat);
	MODULE_EXPORT_APPLY(exports, SDL_MapRGB);
	MODULE_EXPORT_APPLY(exports, SDL_MapRGBA);

	// SDL_platform.h

	MODULE_EXPORT_APPLY(exports, SDL_GetPlatform);

	// SDL_power.h

	// SDL_PowerState

	Local<Object> PowerState = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_PowerState"), PowerState);
	MODULE_CONSTANT(PowerState, SDL_POWERSTATE_UNKNOWN);
	MODULE_CONSTANT(PowerState, SDL_POWERSTATE_ON_BATTERY);
	MODULE_CONSTANT(PowerState, SDL_POWERSTATE_NO_BATTERY);
	MODULE_CONSTANT(PowerState, SDL_POWERSTATE_CHARGING);
	MODULE_CONSTANT(PowerState, SDL_POWERSTATE_CHARGED);

	MODULE_EXPORT_APPLY(exports, SDL_GetPowerInfo);

	// SDL_quit.h

	MODULE_EXPORT_APPLY(exports, SDL_QuitRequested);

	// SDL_rect.h

	MODULE_EXPORT_APPLY(exports, SDL_RectEmpty);
	MODULE_EXPORT_APPLY(exports, SDL_RectEquals);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_HasIntersection);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_IntersectRect);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_UnionRect);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_EnclosePoints);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_IntersectRectAndLine);

	// SDL_render.h

	// SDL_RendererFlags
	Local<Object> RendererFlags = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_RendererFlags"), RendererFlags);
	MODULE_CONSTANT(RendererFlags, SDL_RENDERER_SOFTWARE);
	MODULE_CONSTANT(RendererFlags, SDL_RENDERER_ACCELERATED);
	MODULE_CONSTANT(RendererFlags, SDL_RENDERER_PRESENTVSYNC);
	MODULE_CONSTANT(RendererFlags, SDL_RENDERER_TARGETTEXTURE);

	// SDL_TextureAccess
	Local<Object> TextureAccess = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_TextureAccess"), TextureAccess);
	MODULE_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_STATIC);
	MODULE_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_STREAMING);
	MODULE_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_TARGET);

	// SDL_TextureModulate
	Local<Object> TextureModulate = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_TextureModulate"), TextureModulate);
	MODULE_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_NONE);
	MODULE_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_COLOR);
	MODULE_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_ALPHA);

	// SDL_RendererFlip
	Local<Object> RendererFlip = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_RendererFlip"), RendererFlip);
	MODULE_CONSTANT(RendererFlip, SDL_FLIP_NONE);
	MODULE_CONSTANT(RendererFlip, SDL_FLIP_HORIZONTAL);
	MODULE_CONSTANT(RendererFlip, SDL_FLIP_VERTICAL);

	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetNumRenderDrivers);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRenderDriverInfo);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_CreateWindowAndRenderer);
	MODULE_EXPORT_APPLY(exports, SDL_CreateRenderer);
	MODULE_EXPORT_APPLY(exports, SDL_CreateSoftwareRenderer);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRenderer);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRendererInfo);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRendererOutputSize);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_CreateTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_CreateTextureFromSurface);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_QueryTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_SetTextureColorMod);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetTextureColorMod);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_SetTextureAlphaMod);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetTextureAlphaMod);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_SetTextureBlendMode);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetTextureBlendMode);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_UpdateTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_UpdateYUVTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_LockTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_UnlockTexture);
	MODULE_EXPORT_APPLY(exports, SDL_RenderTargetSupported);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_SetRenderTarget);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRenderTarget);
	MODULE_EXPORT_APPLY(exports, SDL_RenderSetLogicalSize);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderGetLogicalSize);
	MODULE_EXPORT_APPLY(exports, SDL_RenderSetViewport);
	MODULE_EXPORT_APPLY(exports, SDL_RenderGetViewport);
	MODULE_EXPORT_APPLY(exports, SDL_RenderSetClipRect);
	MODULE_EXPORT_APPLY(exports, SDL_RenderGetClipRect);
	MODULE_EXPORT_APPLY(exports, SDL_RenderSetScale);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderGetScale);
	MODULE_EXPORT_APPLY(exports, SDL_SetRenderDrawColor);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRenderDrawColor);
	MODULE_EXPORT_APPLY(exports, SDL_SetRenderDrawBlendMode);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetRenderDrawBlendMode);
	MODULE_EXPORT_APPLY(exports, SDL_RenderClear);
	MODULE_EXPORT_APPLY(exports, SDL_RenderDrawPoint);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderDrawPoints);
	MODULE_EXPORT_APPLY(exports, SDL_RenderDrawLine);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderDrawLines);
	MODULE_EXPORT_APPLY(exports, SDL_RenderDrawRect);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderDrawRects);
	MODULE_EXPORT_APPLY(exports, SDL_RenderFillRect);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderFillRects);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderCopy);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderCopyEx);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_RenderReadPixels);
	MODULE_EXPORT_APPLY(exports, SDL_RenderPresent);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_DestroyTexture);
	MODULE_EXPORT_APPLY(exports, SDL_DestroyRenderer);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GL_BindTexture);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GL_UnbindTexture);

	// SDL_revision.h

	// TODO: MODULE_CONSTANT_STRING(exports, SDL_REVISION);
	// TODO: MODULE_CONSTANT(exports, SDL_REVISION_NUMBER);

	// SDL_rwops.h

	MODULE_CONSTANT(exports, RW_SEEK_SET);
	MODULE_CONSTANT(exports, RW_SEEK_CUR);
	MODULE_CONSTANT(exports, RW_SEEK_END);

	MODULE_EXPORT_APPLY(exports, SDL_AllocRW);
	MODULE_EXPORT_APPLY(exports, SDL_FreeRW);
	MODULE_EXPORT_APPLY(exports, SDL_RWFromFile);
	MODULE_EXPORT_APPLY(exports, SDL_RWsize);
	MODULE_EXPORT_APPLY(exports, SDL_RWseek);
	MODULE_EXPORT_APPLY(exports, SDL_RWtell);
	MODULE_EXPORT_APPLY(exports, SDL_RWread);
	MODULE_EXPORT_APPLY(exports, SDL_RWwrite);
	MODULE_EXPORT_APPLY(exports, SDL_RWclose);

	// SDL_scancode.h
	// SDL_shape.h
	// SDL_stdinc.h

	MODULE_EXPORT_APPLY(exports, SDL_getenv);
	MODULE_EXPORT_APPLY(exports, SDL_setenv);

	// SDL_surface.h

	MODULE_EXPORT_APPLY(exports, SDL_CreateRGBSurface);
	MODULE_EXPORT_APPLY(exports, SDL_CreateRGBSurfaceFrom);
	MODULE_EXPORT_APPLY(exports, SDL_FreeSurface);
	MODULE_EXPORT_APPLY(exports, SDL_LoadBMP);
	MODULE_EXPORT_APPLY(exports, SDL_SaveBMP);
	MODULE_EXPORT_APPLY(exports, SDL_SetSurfaceBlendMode);
	MODULE_EXPORT_APPLY(exports, SDL_ConvertSurfaceFormat);
	MODULE_EXPORT_APPLY(exports, SDL_FillRect);
	MODULE_EXPORT_APPLY(exports, SDL_BlitSurface);
	MODULE_EXPORT_APPLY(exports, SDL_SoftStretch);
	MODULE_EXPORT_APPLY(exports, SDL_BlitScaled);

	MODULE_EXPORT_APPLY(exports, SDL_GetPixel);
	MODULE_EXPORT_APPLY(exports, SDL_PutPixel);

	// SDL_system.h

	#if defined(__ANDROID__)

	MODULE_CONSTANT(exports, SDL_ANDROID_EXTERNAL_STORAGE_READ);
	MODULE_CONSTANT(exports, SDL_ANDROID_EXTERNAL_STORAGE_WRITE);

	MODULE_EXPORT_APPLY(exports, SDL_AndroidGetInternalStoragePath);
	MODULE_EXPORT_APPLY(exports, SDL_AndroidGetExternalStorageState);
	MODULE_EXPORT_APPLY(exports, SDL_AndroidGetExternalStoragePath);

	#endif // defined(__ANDROID__)

	// SDL_syswm.h
	// SDL_test.h
	// SDL_test_assert.h
	// SDL_test_common.h
	// SDL_test_compare.h
	// SDL_test_crc32.h
	// SDL_test_font.h
	// SDL_test_fuzzer.h
	// SDL_test_harness.h
	// SDL_test_images.h
	// SDL_test_log.h
	// SDL_test_md5.h
	// SDL_test_random.h
	// SDL_thread.h
	// SDL_timer.h

	MODULE_EXPORT_APPLY(exports, SDL_GetTicks);
	MODULE_EXPORT_APPLY(exports, SDL_Delay);

	// SDL_touch.h

	// TODO: #define SDL_TOUCH_MOUSEID ((Uint32)-1)
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetNumTouchDevices);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetTouchDevice);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetNumTouchFingers);
	// TODO: MODULE_EXPORT_APPLY(exports, SDL_GetTouchFinger);

	// SDL_types.h
	// SDL_version.h

	MODULE_CONSTANT(exports, SDL_MAJOR_VERSION);
	MODULE_CONSTANT(exports, SDL_MINOR_VERSION);
	MODULE_CONSTANT(exports, SDL_PATCHLEVEL);

	MODULE_EXPORT_APPLY(exports, SDL_GetRevisionNumber);

	// SDL_video.h

	// SDL_WindowFlags
	Local<Object> WindowFlags = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_WindowFlags"), WindowFlags);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_FULLSCREEN);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_OPENGL);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_SHOWN);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_HIDDEN);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_BORDERLESS);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_RESIZABLE);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_MINIMIZED);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_MAXIMIZED);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_INPUT_GRABBED);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_INPUT_FOCUS);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_MOUSE_FOCUS);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_FULLSCREEN_DESKTOP);
	MODULE_CONSTANT(WindowFlags, SDL_WINDOW_FOREIGN);

	MODULE_CONSTANT(exports, SDL_WINDOWPOS_UNDEFINED);
	MODULE_CONSTANT(exports, SDL_WINDOWPOS_CENTERED);

	// SDL_WindowEventID
	Local<Object> WindowEventID = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_WindowEventID"), WindowEventID);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_NONE);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_SHOWN);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_HIDDEN);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_EXPOSED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MOVED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_RESIZED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_SIZE_CHANGED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MINIMIZED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MAXIMIZED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_RESTORED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_ENTER);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_LEAVE);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_FOCUS_GAINED);
	MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_FOCUS_LOST);
    MODULE_CONSTANT(WindowEventID, SDL_WINDOWEVENT_CLOSE);

	// SDL_GLattr
	Local<Object> GLattr = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_GLattr"), GLattr);
	MODULE_CONSTANT(GLattr, SDL_GL_RED_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_GREEN_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_BLUE_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_ALPHA_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_BUFFER_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_DOUBLEBUFFER);
	MODULE_CONSTANT(GLattr, SDL_GL_DEPTH_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_STENCIL_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_ACCUM_RED_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_ACCUM_GREEN_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_ACCUM_BLUE_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_ACCUM_ALPHA_SIZE);
	MODULE_CONSTANT(GLattr, SDL_GL_STEREO);
	MODULE_CONSTANT(GLattr, SDL_GL_MULTISAMPLEBUFFERS);
	MODULE_CONSTANT(GLattr, SDL_GL_MULTISAMPLESAMPLES);
	MODULE_CONSTANT(GLattr, SDL_GL_ACCELERATED_VISUAL);
	MODULE_CONSTANT(GLattr, SDL_GL_RETAINED_BACKING);
	MODULE_CONSTANT(GLattr, SDL_GL_CONTEXT_MAJOR_VERSION);
	MODULE_CONSTANT(GLattr, SDL_GL_CONTEXT_MINOR_VERSION);
	MODULE_CONSTANT(GLattr, SDL_GL_CONTEXT_EGL);
	MODULE_CONSTANT(GLattr, SDL_GL_CONTEXT_FLAGS);
	MODULE_CONSTANT(GLattr, SDL_GL_CONTEXT_PROFILE_MASK);
	MODULE_CONSTANT(GLattr, SDL_GL_SHARE_WITH_CURRENT_CONTEXT);

	// SDL_GLprofile
	Local<Object> GLprofile = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_GLprofile"), GLprofile);
	MODULE_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_CORE);
	MODULE_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	MODULE_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_ES);

	// SDL_GLcontextFlag
	Local<Object> GLcontextFlag = NanNew<Object>();
	exports->Set(NanNew<String>("SDL_GLcontextFlag"), GLcontextFlag);
	MODULE_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_DEBUG_FLAG);
	MODULE_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	MODULE_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG);
	MODULE_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_RESET_ISOLATION_FLAG);

	MODULE_EXPORT_APPLY(exports, SDL_GetNumVideoDrivers);
	MODULE_EXPORT_APPLY(exports, SDL_GetVideoDriver);
	MODULE_EXPORT_APPLY(exports, SDL_VideoInit);
	MODULE_EXPORT_APPLY(exports, SDL_VideoQuit);
	MODULE_EXPORT_APPLY(exports, SDL_GetCurrentVideoDriver);
	MODULE_EXPORT_APPLY(exports, SDL_GetNumVideoDisplays);
	MODULE_EXPORT_APPLY(exports, SDL_GetDisplayName);
	MODULE_EXPORT_APPLY(exports, SDL_GetDisplayBounds);
	MODULE_EXPORT_APPLY(exports, SDL_GetCurrentDisplayMode);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowDisplayIndex);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowPixelFormat);
	MODULE_EXPORT_APPLY(exports, SDL_CreateWindow);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowFlags);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowTitle);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowTitle);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowIcon);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowPosition);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowPosition);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowSize);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowSize);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowMinimumSize);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowMinimumSize);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowMaximumSize);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowMaximumSize);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowBordered);
	MODULE_EXPORT_APPLY(exports, SDL_ShowWindow);
	MODULE_EXPORT_APPLY(exports, SDL_HideWindow);
	MODULE_EXPORT_APPLY(exports, SDL_RaiseWindow);
	MODULE_EXPORT_APPLY(exports, SDL_MaximizeWindow);
	MODULE_EXPORT_APPLY(exports, SDL_MinimizeWindow);
	MODULE_EXPORT_APPLY(exports, SDL_RestoreWindow);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowFullscreen);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowGrab);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowGrab);
	MODULE_EXPORT_APPLY(exports, SDL_SetWindowBrightness);
	MODULE_EXPORT_APPLY(exports, SDL_GetWindowBrightness);
	MODULE_EXPORT_APPLY(exports, SDL_DestroyWindow);
	MODULE_EXPORT_APPLY(exports, SDL_IsScreenSaverEnabled);
	MODULE_EXPORT_APPLY(exports, SDL_EnableScreenSaver);
	MODULE_EXPORT_APPLY(exports, SDL_DisableScreenSaver);

	MODULE_EXPORT_APPLY(exports, SDL_GL_ExtensionSupported);
	MODULE_EXPORT_APPLY(exports, SDL_GL_SetAttribute);
	MODULE_EXPORT_APPLY(exports, SDL_GL_GetAttribute);
	MODULE_EXPORT_APPLY(exports, SDL_GL_CreateContext);
	MODULE_EXPORT_APPLY(exports, SDL_GL_MakeCurrent);
	MODULE_EXPORT_APPLY(exports, SDL_GL_GetCurrentWindow);
	MODULE_EXPORT_APPLY(exports, SDL_GL_GetCurrentContext);
	MODULE_EXPORT_APPLY(exports, SDL_GL_GetDrawableSize);
	MODULE_EXPORT_APPLY(exports, SDL_GL_SetSwapInterval);
	MODULE_EXPORT_APPLY(exports, SDL_GL_GetSwapInterval);
	MODULE_EXPORT_APPLY(exports, SDL_GL_SwapWindow);
	MODULE_EXPORT_APPLY(exports, SDL_GL_DeleteContext);

	MODULE_EXPORT_APPLY(exports, SDL_EXT_SurfaceToImageDataAsync);
	MODULE_EXPORT_APPLY(exports, SDL_EXT_SurfaceToImageData);
	MODULE_EXPORT_APPLY(exports, SDL_EXT_ImageDataToSurface);
}

} // namespace node_sdl2

#if NODE_VERSION_AT_LEAST(0,11,0)
NODE_MODULE_CONTEXT_AWARE_BUILTIN(node_sdl2, node_sdl2::init)
#else
NODE_MODULE(node_sdl2, node_sdl2::init)
#endif

