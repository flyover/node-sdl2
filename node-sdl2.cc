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

namespace node_sdl2 {

static Nan::Persistent<v8::Value> _gl_current_window;
static Nan::Persistent<v8::Value> _gl_current_context;

// load surface

class TaskLoadBMP : public Nanx::SimpleTask
{
	public: Nan::Persistent<v8::Function> m_callback;
	public: char* m_file;
	public: SDL_Surface* m_surface;
	public: TaskLoadBMP(v8::Local<v8::String> file, v8::Local<v8::Function> callback) :
		m_file(strdup(*v8::String::Utf8Value(file))),
		m_surface(NULL)
	{
		m_callback.Reset(callback);
	}
	public: ~TaskLoadBMP()
	{
		m_callback.Reset();
		free(m_file); m_file = NULL; // strdup
		if (m_surface) { SDL_FreeSurface(m_surface); m_surface = NULL; }
	}
	public: void DoWork()
	{
		m_surface = SDL_LoadBMP(m_file);
	}
	public: void DoAfterWork(int status)
	{
		Nan::HandleScope scope;
		v8::Local<v8::Value> argv[] = { WrapSurface::Hold(m_surface) };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<v8::Function>(m_callback), countof(argv), argv);
		m_surface = NULL; // script owns pointer
	}
};

// save surface

class TaskSaveBMP : public Nanx::SimpleTask
{
	public: Nan::Persistent<v8::Value> m_hold_surface;
	public: Nan::Persistent<v8::Function> m_callback;
	public: SDL_Surface* m_surface;
	public: char* m_file;
	public: int m_err;
	public: TaskSaveBMP(v8::Local<v8::Value> surface, v8::Local<v8::String> file, v8::Local<v8::Function> callback) :
		m_surface(WrapSurface::Peek(surface)),
		m_file(strdup(*v8::String::Utf8Value(file))),
		m_err(0)
	{
		m_hold_surface.Reset(surface);
		m_callback.Reset(callback);
	}
	public: ~TaskSaveBMP()
	{
		m_hold_surface.Reset();
		m_callback.Reset();
		free(m_file); m_file = NULL; // strdup
	}
	public: void DoWork()
	{
		m_err = SDL_SaveBMP(m_surface, m_file);
	}
	public: void DoAfterWork(int status)
	{
		Nan::HandleScope scope;
		v8::Local<v8::Value> argv[] = { Nan::New(m_err) };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<v8::Function>(m_callback), countof(argv), argv);
	}
};

// SDL.h

NANX_EXPORT(SDL_Init)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	int err = SDL_Init(flags);
	if (err < 0)
	{
		printf("SDL_Init error: %d\n", err);
	}
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_InitSubSystem)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	int err = SDL_InitSubSystem(flags);
	if (err < 0)
	{
		printf("SDL_InitSubSystem error: %d\n", err);
	}
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_QuitSubSystem)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	SDL_QuitSubSystem(flags);
}

NANX_EXPORT(SDL_WasInit)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	::Uint32 mask = SDL_WasInit(flags);
	info.GetReturnValue().Set(Nan::New(mask));
}

NANX_EXPORT(SDL_Quit)
{
	SDL_Quit();
}

// SDL_assert.h
// SDL_atomic.h
// SDL_audio.h
// SDL_bits.h

// SDL_blendmode.h

// SDL_clipboard.h

NANX_EXPORT(SDL_SetClipboardText)
{
	v8::Local<v8::String> text = v8::Local<v8::String>::Cast(info[0]);
	int err = SDL_SetClipboardText(*v8::String::Utf8Value(text));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_GetClipboardText)
{
	const char* clipboard_text = SDL_GetClipboardText();
	info.GetReturnValue().Set(NANX_STRING(clipboard_text));
}

NANX_EXPORT(SDL_HasClipboardText)
{
	SDL_bool has_clipboard_text = SDL_HasClipboardText();
	info.GetReturnValue().Set(Nan::New(has_clipboard_text != SDL_FALSE));
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

NANX_EXPORT(SDL_GetError)
{
	const char* sdl_error = SDL_GetError();
	info.GetReturnValue().Set(NANX_STRING(sdl_error));
}

NANX_EXPORT(SDL_ClearError)
{
	SDL_ClearError();
}

// SDL_events.h

NANX_EXPORT(SDL_PollEvent)
{
	SDL_Event event;
	if (!SDL_PollEvent(&event))
	{
		info.GetReturnValue().SetNull();
		return;
	}

	v8::Local<v8::Object> evt = Nan::New<v8::Object>();
	evt->Set(NANX_SYMBOL("type"), Nan::New(event.type));
	evt->Set(NANX_SYMBOL("timestamp"), Nan::New(event.common.timestamp));

	switch (event.type)
	{
	case SDL_QUIT:
		//info.GetReturnValue().Set(n_SDL_QuitEvent::NewInstance(event.quit));
		break;
	case SDL_APP_TERMINATING:
	case SDL_APP_LOWMEMORY:
	case SDL_APP_WILLENTERBACKGROUND:
	case SDL_APP_DIDENTERBACKGROUND:
	case SDL_APP_WILLENTERFOREGROUND:
	case SDL_APP_DIDENTERFOREGROUND:
		//info.GetReturnValue().Set(WrapCommonEvent::NewInstance(event.common));
		break;
	case SDL_WINDOWEVENT:
		//info.GetReturnValue().Set(WrapWindowEvent::NewInstance(event.window));
		evt->Set(NANX_SYMBOL("windowID"), Nan::New(event.window.windowID));
		evt->Set(NANX_SYMBOL("event"), Nan::New(event.window.event));
		evt->Set(NANX_SYMBOL("data1"), Nan::New(event.window.data1));
		evt->Set(NANX_SYMBOL("data2"), Nan::New(event.window.data2));
		break;
	case SDL_SYSWMEVENT:
		// TODO
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		evt->Set(NANX_SYMBOL("windowID"), Nan::New(event.key.windowID));
		evt->Set(NANX_SYMBOL("state"), Nan::New(event.key.state));
		evt->Set(NANX_SYMBOL("repeat"), Nan::New(event.key.repeat));
		evt->Set(NANX_SYMBOL("scancode"), Nan::New(event.key.keysym.scancode));
		evt->Set(NANX_SYMBOL("sym"), Nan::New(event.key.keysym.sym));
		evt->Set(NANX_SYMBOL("mod"), Nan::New(event.key.keysym.mod));
		break;
	case SDL_TEXTEDITING:
	case SDL_TEXTINPUT:
		// TODO
		break;
	case SDL_MOUSEMOTION:
		evt->Set(NANX_SYMBOL("windowID"), Nan::New(event.motion.windowID));
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.motion.which));
		evt->Set(NANX_SYMBOL("state"), Nan::New(event.motion.state));
		evt->Set(NANX_SYMBOL("x"), Nan::New(event.motion.x));
		evt->Set(NANX_SYMBOL("y"), Nan::New(event.motion.y));
		evt->Set(NANX_SYMBOL("xrel"), Nan::New(event.motion.xrel));
		evt->Set(NANX_SYMBOL("yrel"), Nan::New(event.motion.yrel));
		{
			int w = 0, h = 0;
			SDL_GetWindowSize(SDL_GetWindowFromID(event.motion.windowID), &w, &h);
			evt->Set(NANX_SYMBOL("nx"), Nan::New((2.0f * float(event.motion.x) / w) - 1.0f));
			evt->Set(NANX_SYMBOL("ny"), Nan::New(1.0f - (2.0f * float(event.motion.y) / h)));
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		evt->Set(NANX_SYMBOL("windowID"), Nan::New(event.button.windowID));
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.button.which));
		evt->Set(NANX_SYMBOL("button"), Nan::New(event.button.button));
		evt->Set(NANX_SYMBOL("state"), Nan::New(event.button.state));
		evt->Set(NANX_SYMBOL("x"), Nan::New(event.button.x));
		evt->Set(NANX_SYMBOL("y"), Nan::New(event.button.y));
		{
			int w = 0, h = 0;
			SDL_GetWindowSize(SDL_GetWindowFromID(event.motion.windowID), &w, &h);
			evt->Set(NANX_SYMBOL("nx"), Nan::New((2.0f * float(event.motion.x) / w) - 1.0f));
			evt->Set(NANX_SYMBOL("ny"), Nan::New(1.0f - (2.0f * float(event.motion.y) / h)));
		}
		break;
	case SDL_MOUSEWHEEL:
		evt->Set(NANX_SYMBOL("windowID"), Nan::New(event.wheel.windowID));
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.wheel.which));
		evt->Set(NANX_SYMBOL("x"), Nan::New(event.wheel.x));
		evt->Set(NANX_SYMBOL("y"), Nan::New(event.wheel.y));
		break;
	case SDL_JOYAXISMOTION:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.jaxis.which));
		evt->Set(NANX_SYMBOL("axis"), Nan::New(event.jaxis.axis));
		evt->Set(NANX_SYMBOL("value"), Nan::New(event.jaxis.value));
		break;
	case SDL_JOYBALLMOTION:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.jball.which));
		evt->Set(NANX_SYMBOL("ball"), Nan::New(event.jball.ball));
		evt->Set(NANX_SYMBOL("xrel"), Nan::New(event.jball.xrel));
		evt->Set(NANX_SYMBOL("yrel"), Nan::New(event.jball.yrel));
		break;
	case SDL_JOYHATMOTION:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.jhat.which));
		evt->Set(NANX_SYMBOL("hat"), Nan::New(event.jhat.hat));
		evt->Set(NANX_SYMBOL("value"), Nan::New(event.jhat.value));
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.jbutton.which));
		evt->Set(NANX_SYMBOL("button"), Nan::New(event.jbutton.button));
		evt->Set(NANX_SYMBOL("state"), Nan::New(event.jbutton.state));
		break;
	case SDL_JOYDEVICEADDED:
	case SDL_JOYDEVICEREMOVED:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.jdevice.which));
		break;
	case SDL_CONTROLLERAXISMOTION:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.caxis.which));
		evt->Set(NANX_SYMBOL("axis"), Nan::New(event.caxis.axis));
		evt->Set(NANX_SYMBOL("value"), Nan::New(event.caxis.value));
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.cbutton.which));
		evt->Set(NANX_SYMBOL("button"), Nan::New(event.cbutton.button));
		evt->Set(NANX_SYMBOL("state"), Nan::New(event.cbutton.state));
		break;
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
	case SDL_CONTROLLERDEVICEREMAPPED:
		evt->Set(NANX_SYMBOL("which"), Nan::New(event.cdevice.which));
		break;
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
	case SDL_FINGERMOTION:
		evt->Set(NANX_SYMBOL("touchId"), Nan::New((int32_t) event.tfinger.touchId)); // TODO: 64 bit integer
		evt->Set(NANX_SYMBOL("fingerId"), Nan::New((int32_t) event.tfinger.fingerId)); // TODO: 64 bit integer
		evt->Set(NANX_SYMBOL("x"), Nan::New(event.tfinger.x));
		evt->Set(NANX_SYMBOL("y"), Nan::New(event.tfinger.y));
		evt->Set(NANX_SYMBOL("dx"), Nan::New(event.tfinger.dx));
		evt->Set(NANX_SYMBOL("dy"), Nan::New(event.tfinger.dy));
		evt->Set(NANX_SYMBOL("pressure"), Nan::New(event.tfinger.pressure));
		{
			evt->Set(NANX_SYMBOL("nx"), Nan::New((2.0f * float(event.tfinger.x)) - 1.0f));
			evt->Set(NANX_SYMBOL("ny"), Nan::New(1.0f - (2.0f * float(event.tfinger.y))));
		}
		break;
	case SDL_DOLLARGESTURE:
	case SDL_DOLLARRECORD:
	case SDL_MULTIGESTURE:
	case SDL_CLIPBOARDUPDATE:
	case SDL_DROPFILE:
	#if SDL_VERSION_ATLEAST(2,0,4)
    case SDL_AUDIODEVICEADDED:
    case SDL_AUDIODEVICEREMOVED:
    case SDL_RENDER_TARGETS_RESET:
    case SDL_RENDER_DEVICE_RESET:
    #endif
	case SDL_USEREVENT:
		// TODO
		break;
	default:
		break;
	}

	info.GetReturnValue().Set(evt);
}

// SDL_gamecontroller.h
// SDL_gesture.h
// SDL_haptic.h

// SDL_hints.h

NANX_EXPORT(SDL_SetHintWithPriority)
{
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(info[0]);
	v8::Local<v8::String> value = v8::Local<v8::String>::Cast(info[1]);
	SDL_HintPriority priority = NANX_SDL_HintPriority(info[2]);
	SDL_bool ret = SDL_SetHintWithPriority(*v8::String::Utf8Value(name), *v8::String::Utf8Value(value), priority);
	info.GetReturnValue().Set(Nan::New(ret != SDL_FALSE));
}

NANX_EXPORT(SDL_SetHint)
{
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(info[0]);
	v8::Local<v8::String> value = v8::Local<v8::String>::Cast(info[1]);
	SDL_bool ret = SDL_SetHint(*v8::String::Utf8Value(name), *v8::String::Utf8Value(value));
	info.GetReturnValue().Set(Nan::New(ret != SDL_FALSE));
}

NANX_EXPORT(SDL_GetHint)
{
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(info[0]);
	const char* value = SDL_GetHint(*v8::String::Utf8Value(name));
	info.GetReturnValue().Set(NANX_STRING(value));
}

// TODO: typedef void (*SDL_HintCallback)(void *userdata, const char *name, const char *oldValue, const char *newValue);
// TODO: extern DECLSPEC void SDLCALL SDL_AddHintCallback(const char *name, SDL_HintCallback callback, void *userdata);
// TODO: extern DECLSPEC void SDLCALL SDL_DelHintCallback(const char *name, SDL_HintCallback callback, void *userdata);

NANX_EXPORT(SDL_ClearHints)
{
	SDL_ClearHints();
}

// SDL_joystick.h

NANX_EXPORT(SDL_NumJoysticks)
{
	info.GetReturnValue().Set(Nan::New(SDL_NumJoysticks()));
}

NANX_EXPORT(SDL_JoystickNameForIndex)
{
	int device_index = NANX_int(info[0]);
	const char* name = SDL_JoystickNameForIndex(device_index);
	info.GetReturnValue().Set(NANX_STRING(name));
}

NANX_EXPORT(SDL_JoystickOpen)
{
	int device_index = NANX_int(info[0]);
	SDL_Joystick* joystick = SDL_JoystickOpen(device_index);
	info.GetReturnValue().Set(WrapJoystick::Hold(joystick));
}

NANX_EXPORT(SDL_JoystickName)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	const char* name = SDL_JoystickName(joystick);
	info.GetReturnValue().Set(NANX_STRING(name));
}

// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetDeviceGUID(int device_index);
// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUID(SDL_Joystick * joystick);
// TODO: extern DECLSPEC void SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char *pszGUID, int cbGUID);
// TODO: extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUIDFromString(const char *pchGUID);

NANX_EXPORT(SDL_JoystickGetAttached)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	SDL_bool attached = SDL_JoystickGetAttached(joystick);
	info.GetReturnValue().Set(Nan::New(attached != SDL_FALSE));
}

NANX_EXPORT(SDL_JoystickInstanceID)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
	info.GetReturnValue().Set(Nan::New(id));
}

NANX_EXPORT(SDL_JoystickNumAxes)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int num = SDL_JoystickNumAxes(joystick);
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(SDL_JoystickNumBalls)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int num = SDL_JoystickNumBalls(joystick);
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(SDL_JoystickNumHats)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int num = SDL_JoystickNumHats(joystick);
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(SDL_JoystickNumButtons)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int num = SDL_JoystickNumButtons(joystick);
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(SDL_JoystickUpdate)
{
	SDL_JoystickUpdate();
}

NANX_EXPORT(SDL_JoystickEventState)
{
	int state = NANX_int(info[0]);
	int err = SDL_JoystickEventState(state);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_JoystickGetAxis)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int index = NANX_int(info[1]);
	Sint16 value = SDL_JoystickGetAxis(joystick, index);
	info.GetReturnValue().Set(Nan::New(value));
}

NANX_EXPORT(SDL_JoystickGetBall)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int index = NANX_int(info[1]);
	int dx = 0; // TODO
	int dy = 0; // TODO
	int value = SDL_JoystickGetBall(joystick, index, &dx, &dy);
	info.GetReturnValue().Set(Nan::New(value));
}

NANX_EXPORT(SDL_JoystickGetHat)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int index = NANX_int(info[1]);
	Uint8 value = SDL_JoystickGetHat(joystick, index);
	info.GetReturnValue().Set(Nan::New(value));
}

NANX_EXPORT(SDL_JoystickGetButton)
{
	SDL_Joystick* joystick = WrapJoystick::Peek(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	int index = NANX_int(info[1]);
	Uint8 value = SDL_JoystickGetButton(joystick, index);
	info.GetReturnValue().Set(Nan::New(value));
}

NANX_EXPORT(SDL_JoystickClose)
{
	SDL_Joystick* joystick = WrapJoystick::Drop(info[0]); if (!joystick) { return Nan::ThrowError("null SDL_Joystick object"); }
	SDL_JoystickClose(joystick);
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

NANX_EXPORT(SDL_PIXELFLAG)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_PIXELFLAG(format)));
}

NANX_EXPORT(SDL_PIXELTYPE)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_PIXELTYPE(format)));
}

NANX_EXPORT(SDL_PIXELORDER)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_PIXELORDER(format)));
}

NANX_EXPORT(SDL_PIXELLAYOUT)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_PIXELLAYOUT(format)));
}

NANX_EXPORT(SDL_BITSPERPIXEL)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_BITSPERPIXEL(format)));
}

NANX_EXPORT(SDL_BYTESPERPIXEL)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_BYTESPERPIXEL(format)));
}

NANX_EXPORT(SDL_ISPIXELFORMAT_INDEXED)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_ISPIXELFORMAT_INDEXED(format) != SDL_FALSE));
}

NANX_EXPORT(SDL_ISPIXELFORMAT_ALPHA)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_ISPIXELFORMAT_ALPHA(format) != SDL_FALSE));
}

NANX_EXPORT(SDL_ISPIXELFORMAT_FOURCC)
{
	::Uint32 format = NANX_Uint32(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_ISPIXELFORMAT_FOURCC(format) != SDL_FALSE));
}

NANX_EXPORT(SDL_GetPixelFormatName)
{
	::Uint32 format = NANX_Uint32(info[0]);
	const char* name = SDL_GetPixelFormatName(format);
	info.GetReturnValue().Set(NANX_STRING(name));
}

NANX_EXPORT(SDL_PixelFormatEnumToMasks)
{
	::Uint32 format = NANX_Uint32(info[0]);
	int bpp = 0;
	::Uint32 Rmask = 0;
	::Uint32 Gmask = 0;
	::Uint32 Bmask = 0;
	::Uint32 Amask = 0;
	SDL_bool success = SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
	if (info[1]->IsObject())
	{
		v8::Local<v8::Object> masks = v8::Local<v8::Object>::Cast(info[1]);
		masks->Set(NANX_SYMBOL("bpp"), Nan::New(bpp));
		masks->Set(NANX_SYMBOL("Rmask"), Nan::New(Rmask));
		masks->Set(NANX_SYMBOL("Gmask"), Nan::New(Gmask));
		masks->Set(NANX_SYMBOL("Bmask"), Nan::New(Bmask));
		masks->Set(NANX_SYMBOL("Amask"), Nan::New(Amask));
	}
	info.GetReturnValue().Set(Nan::New(success != SDL_FALSE));
}

NANX_EXPORT(SDL_MasksToPixelFormatEnum)
{
	int bpp = NANX_int(info[0]);
	::Uint32 Rmask = NANX_Uint32(info[1]);
	::Uint32 Gmask = NANX_Uint32(info[2]);
	::Uint32 Bmask = NANX_Uint32(info[3]);
	::Uint32 Amask = NANX_Uint32(info[4]);
	::Uint32 format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
	info.GetReturnValue().Set(Nan::New(format));
}

// extern DECLSPEC SDL_PixelFormat * SDLCALL SDL_AllocFormat(Uint32 pixel_format);
NANX_EXPORT(SDL_AllocFormat)
{
	::Uint32 pixel_format = NANX_Uint32(info[0]);
	SDL_PixelFormat* format = SDL_AllocFormat(pixel_format);
	info.GetReturnValue().Set(WrapPixelFormat::Hold(format));
}

// extern DECLSPEC void SDLCALL SDL_FreeFormat(SDL_PixelFormat *format);
NANX_EXPORT(SDL_FreeFormat)
{
	SDL_PixelFormat* format = WrapPixelFormat::Drop(info[0]); if (!format) { return Nan::ThrowError("null SDL_PixelFormat object"); }
	SDL_FreeFormat(format); format = NULL;
}

// TODO: extern DECLSPEC SDL_Palette *SDLCALL SDL_AllocPalette(int ncolors);
// TODO: extern DECLSPEC int SDLCALL SDL_SetPixelFormatPalette(SDL_PixelFormat * format, SDL_Palette *palette);
// TODO: extern DECLSPEC int SDLCALL SDL_SetPaletteColors(SDL_Palette * palette, const SDL_Color * colors, int firstcolor, int ncolors);
// TODO: extern DECLSPEC void SDLCALL SDL_FreePalette(SDL_Palette * palette);

// extern DECLSPEC Uint32 SDLCALL SDL_MapRGB(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b);
NANX_EXPORT(SDL_MapRGB)
{
	SDL_PixelFormat* format = WrapPixelFormat::Peek(info[0]); if (!format) { return Nan::ThrowError("null SDL_PixelFormat object"); }
	::Uint8 r = NANX_Uint8(info[1]);
	::Uint8 g = NANX_Uint8(info[2]);
	::Uint8 b = NANX_Uint8(info[3]);
	::Uint32 pixel = SDL_MapRGB(format, r, g, b);
	info.GetReturnValue().Set(Nan::New(pixel));
}

// extern DECLSPEC Uint32 SDLCALL SDL_MapRGBA(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
NANX_EXPORT(SDL_MapRGBA)
{
	SDL_PixelFormat* format = WrapPixelFormat::Peek(info[0]); if (!format) { return Nan::ThrowError("null SDL_PixelFormat object"); }
	::Uint8 r = NANX_Uint8(info[1]);
	::Uint8 g = NANX_Uint8(info[2]);
	::Uint8 b = NANX_Uint8(info[3]);
	::Uint8 a = NANX_Uint8(info[4]);
	::Uint32 pixel = SDL_MapRGBA(format, r, g, b, a);
	info.GetReturnValue().Set(Nan::New(pixel));
}

// TODO: extern DECLSPEC void SDLCALL SDL_GetRGB(Uint32 pixel, const SDL_PixelFormat * format, Uint8 * r, Uint8 * g, Uint8 * b);
// TODO: extern DECLSPEC void SDLCALL SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat * format, Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a);
// TODO: extern DECLSPEC void SDLCALL SDL_CalculateGammaRamp(float gamma, Uint16 * ramp);

// SDL_platform.h

// extern DECLSPEC const char * SDLCALL SDL_GetPlatform (void);
NANX_EXPORT(SDL_GetPlatform)
{
	info.GetReturnValue().Set(NANX_STRING(SDL_GetPlatform()));
}

// SDL_power.h

// extern DECLSPEC SDL_PowerState SDLCALL SDL_GetPowerInfo(int *secs, int *pct);
NANX_EXPORT(SDL_GetPowerInfo)
{
	int secs = 0;
	int pct = 0;
	SDL_PowerState power_state = SDL_GetPowerInfo(&secs, &pct);
	if (info[1]->IsObject())
	{
		// var info = { secs: -1, pct: -1 };
		// var power_state = sdl.SDL_GetPowerInfo(info);
		v8::Local<v8::Object> _info = v8::Local<v8::Object>::Cast(info[1]);
		_info->Set(NANX_SYMBOL("secs"), Nan::New(secs));
		_info->Set(NANX_SYMBOL("pct"), Nan::New(pct));
	}
	else
	{
		// var a_secs = [ -1 ];
		// var a_pct = [ -1 ];
		// var power_state = sdl.SDL_GetPowerInfo(a_secs, a_pct);
		// var secs = a_secs[0];
		// var pct = a_pcs[0];
		if (info[1]->IsArray())
		{
			v8::Local<v8::Object> a = v8::Local<v8::Object>::Cast(info[1]);
			a->Set(0, Nan::New(secs));
		}
		if (info[2]->IsArray())
		{
			v8::Local<v8::Object> a = v8::Local<v8::Object>::Cast(info[2]);
			a->Set(0, Nan::New(pct));
		}
	}
	info.GetReturnValue().Set(Nan::New(power_state));
}

// SDL_quit.h

NANX_EXPORT(SDL_QuitRequested)
{
	info.GetReturnValue().Set(Nan::New(SDL_QuitRequested() != SDL_FALSE));
}

// SDL_rect.h

// SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
NANX_EXPORT(SDL_RectEmpty)
{
	WrapRect* r = WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	info.GetReturnValue().Set(Nan::New(SDL_RectEmpty(&r->GetRect()) != SDL_FALSE));
}

// SDL_FORCE_INLINE SDL_bool SDL_RectEquals(const SDL_Rect *a, const SDL_Rect *b)
NANX_EXPORT(SDL_RectEquals)
{
	WrapRect* a = WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapRect* b = WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	info.GetReturnValue().Set(Nan::New(SDL_RectEquals(&a->GetRect(), &b->GetRect()) != SDL_FALSE));
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
NANX_EXPORT(SDL_CreateRenderer)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	int index = NANX_int(info[1]);
	::Uint32 flags = NANX_Uint32(info[2]);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, index, flags);
	info.GetReturnValue().Set(WrapRenderer::Hold(renderer));
}

// extern DECLSPEC SDL_Renderer * SDLCALL SDL_CreateSoftwareRenderer(SDL_Surface * surface);
NANX_EXPORT(SDL_CreateSoftwareRenderer)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
	info.GetReturnValue().Set(WrapRenderer::Hold(renderer));
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
NANX_EXPORT(SDL_RenderTargetSupported)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_bool supported = SDL_RenderTargetSupported(renderer);
	info.GetReturnValue().Set(Nan::New(supported != SDL_FALSE));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);
// TODO: extern DECLSPEC SDL_Texture * SDLCALL SDL_GetRenderTarget(SDL_Renderer *renderer);

// extern DECLSPEC int SDLCALL SDL_RenderSetLogicalSize(SDL_Renderer * renderer, int w, int h);
NANX_EXPORT(SDL_RenderSetLogicalSize)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	int w = NANX_int(info[1]);
	int h = NANX_int(info[2]);
	int err = SDL_RenderSetLogicalSize(renderer, w, h);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC void SDLCALL SDL_RenderGetLogicalSize(SDL_Renderer * renderer, int *w, int *h);

// extern DECLSPEC int SDLCALL SDL_RenderSetViewport(SDL_Renderer * renderer, const SDL_Rect * rect);
NANX_EXPORT(SDL_RenderSetViewport)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	int err = SDL_RenderSetViewport(renderer, rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC void SDLCALL SDL_RenderGetViewport(SDL_Renderer * renderer, SDL_Rect * rect);
NANX_EXPORT(SDL_RenderGetViewport)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	SDL_RenderGetViewport(renderer, rect);
}

// extern DECLSPEC int SDLCALL SDL_RenderSetClipRect(SDL_Renderer * renderer, const SDL_Rect * rect);
NANX_EXPORT(SDL_RenderSetClipRect)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	int err = SDL_RenderSetClipRect(renderer, rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC void SDLCALL SDL_RenderGetClipRect(SDL_Renderer * renderer, SDL_Rect * rect);
NANX_EXPORT(SDL_RenderGetClipRect)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	SDL_RenderGetClipRect(renderer, rect);
}

// extern DECLSPEC int SDLCALL SDL_RenderSetScale(SDL_Renderer * renderer, float scaleX, float scaleY);
NANX_EXPORT(SDL_RenderSetScale)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	float scaleX = NANX_float(info[1]);
	float scaleY = NANX_float(info[2]);
	int err = SDL_RenderSetScale(renderer, scaleX, scaleY);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC void SDLCALL SDL_RenderGetScale(SDL_Renderer * renderer, float *scaleX, float *scaleY);

// extern DECLSPEC int SDL_SetRenderDrawColor(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
NANX_EXPORT(SDL_SetRenderDrawColor)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	::Uint8 r = NANX_Uint8(info[1]);
	::Uint8 g = NANX_Uint8(info[2]);
	::Uint8 b = NANX_Uint8(info[3]);
	::Uint8 a = NANX_Uint8(info[4]);
	int err = SDL_SetRenderDrawColor(renderer, r, g, b, a);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDL_GetRenderDrawColor(SDL_Renderer * renderer, Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a);

// extern DECLSPEC int SDLCALL SDL_SetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode blendMode);
NANX_EXPORT(SDL_SetRenderDrawBlendMode)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_BlendMode mode = NANX_SDL_BlendMode(info[1]);
	int err = SDL_SetRenderDrawBlendMode(renderer, mode);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode *blendMode);

// extern DECLSPEC int SDLCALL SDL_RenderClear(SDL_Renderer * renderer);
NANX_EXPORT(SDL_RenderClear)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	int err = SDL_RenderClear(renderer);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC int SDLCALL SDL_RenderDrawPoint(SDL_Renderer * renderer, int x, int y);
NANX_EXPORT(SDL_RenderDrawPoint)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	int x = NANX_int(info[1]);
	int y = NANX_int(info[2]);
	int err = SDL_RenderDrawPoint(renderer, x, y);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points, int count);

// extern DECLSPEC int SDLCALL SDL_RenderDrawLine(SDL_Renderer * renderer, int x1, int y1, int x2, int y2);
NANX_EXPORT(SDL_RenderDrawLine)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	int x1 = NANX_int(info[1]);
	int y1 = NANX_int(info[2]);
	int x2 = NANX_int(info[3]);
	int y2 = NANX_int(info[4]);
	int err = SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points, int count);

// extern DECLSPEC int SDLCALL SDL_RenderDrawRect(SDL_Renderer * renderer, const SDL_Rect * rect);
NANX_EXPORT(SDL_RenderDrawRect)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	int err = SDL_RenderDrawRect(renderer, rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count);

// extern DECLSPEC int SDLCALL SDL_RenderFillRect(SDL_Renderer * renderer, const SDL_Rect * rect);
NANX_EXPORT(SDL_RenderFillRect)
{
	SDL_Renderer* renderer = WrapRenderer::Peek(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	int err = SDL_RenderFillRect(renderer, rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const double angle, const SDL_Point *center, const SDL_RendererFlip flip);
// TODO: extern DECLSPEC int SDLCALL SDL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 format, void *pixels, int pitch);

// extern DECLSPEC void SDLCALL SDL_RenderPresent(SDL_Renderer * renderer);
NANX_EXPORT(SDL_RenderPresent)
{
	SDL_Renderer* renderer = WrapRenderer::Drop(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_RenderPresent(renderer);
}

// TODO: extern DECLSPEC void SDLCALL SDL_DestroyTexture(SDL_Texture * texture);

// extern DECLSPEC void SDLCALL SDL_DestroyRenderer(SDL_Renderer * renderer);
NANX_EXPORT(SDL_DestroyRenderer)
{
	SDL_Renderer* renderer = WrapRenderer::Drop(info[0]); if (!renderer) { return Nan::ThrowError("null SDL_Renderer object"); }
	SDL_DestroyRenderer(renderer);
}

// TODO: extern DECLSPEC int SDLCALL SDL_GL_BindTexture(SDL_Texture *texture, float *texw, float *texh);
// TODO: extern DECLSPEC int SDLCALL SDL_GL_UnbindTexture(SDL_Texture *texture);

// SDL_rwops.h

NANX_EXPORT(SDL_AllocRW)
{
	SDL_RWops* rwops = SDL_AllocRW();
	info.GetReturnValue().Set(WrapRWops::Hold(rwops));
}

NANX_EXPORT(SDL_FreeRW)
{
	SDL_RWops* rwops = WrapRWops::Drop(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	if (rwops && (rwops->type != SDL_RWOPS_UNKNOWN)) { SDL_RWclose(rwops); rwops = NULL; }
	if (rwops) { SDL_FreeRW(rwops); rwops = NULL; }
}

NANX_EXPORT(SDL_RWFromFile)
{
	v8::Local<v8::String> file = v8::Local<v8::String>::Cast(info[0]);
	v8::Local<v8::String> mode = v8::Local<v8::String>::Cast(info[1]);
	SDL_RWops* rwops = SDL_RWFromFile(*v8::String::Utf8Value(file), *v8::String::Utf8Value(mode));
	printf("SDL_RWFromFile: %s %s -> %p\n", *v8::String::Utf8Value(file), *v8::String::Utf8Value(mode), rwops);
	info.GetReturnValue().Set(WrapRWops::Hold(rwops));
}

NANX_EXPORT(SDL_RWsize)
{
	SDL_RWops* rwops = WrapRWops::Peek(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	Sint64 size = SDL_RWsize(rwops);
	info.GetReturnValue().Set(Nan::New((int32_t) size)); // TODO: 64 bit integer
}

NANX_EXPORT(SDL_RWseek)
{
	SDL_RWops* rwops = WrapRWops::Peek(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	Sint64 offset = (Sint64) info[1]->IntegerValue();
	int whence = NANX_int(info[2]);
	Sint64 pos = SDL_RWseek(rwops, offset, whence);
	info.GetReturnValue().Set(Nan::New((int32_t) pos)); // TODO: 64 bit integer
}

NANX_EXPORT(SDL_RWtell)
{
	SDL_RWops* rwops = WrapRWops::Peek(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	Sint64 pos = SDL_RWtell(rwops);
	info.GetReturnValue().Set(Nan::New((int32_t) pos)); // TODO: 64 bit integer
}

NANX_EXPORT(SDL_RWread)
{
	SDL_RWops* rwops = WrapRWops::Peek(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	#if NODE_VERSION_AT_LEAST(4, 0, 0)
	v8::Local<v8::TypedArray> _ptr = v8::Local<v8::TypedArray>::Cast(info[1]);
	void* ptr = static_cast<char*>(_ptr->Buffer()->GetContents().Data()) + _ptr->ByteOffset();
	size_t byte_length = _ptr->ByteLength();
	#else
	v8::Local<v8::Object> _ptr = v8::Local<v8::Object>::Cast(info[1]);
	void* ptr = (void*) _ptr->GetIndexedPropertiesExternalArrayData();
	int byte_length = _ptr->GetIndexedPropertiesExternalArrayDataLength();
	#endif
	size_t size = NANX_size_t(info[2]);
	size_t maxnum = NANX_size_t(info[3]);
	size_t num = 0;
	if ((size * maxnum) <= byte_length)
	{
		num = SDL_RWread(rwops, ptr, size, maxnum);
	}
	info.GetReturnValue().Set(Nan::New((uint32_t) num));
}

NANX_EXPORT(SDL_RWwrite)
{
	SDL_RWops* rwops = WrapRWops::Peek(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	#if NODE_VERSION_AT_LEAST(4, 0, 0)
	v8::Local<v8::TypedArray> _ptr = v8::Local<v8::TypedArray>::Cast(info[1]);
	void* ptr = static_cast<char*>(_ptr->Buffer()->GetContents().Data()) + _ptr->ByteOffset();
	size_t byte_length = _ptr->ByteLength();
	#else
	v8::Local<v8::Object> _ptr = v8::Local<v8::Object>::Cast(info[1]);
	void* ptr = (void*) _ptr->GetIndexedPropertiesExternalArrayData();
	int byte_length = _ptr->GetIndexedPropertiesExternalArrayDataLength();
	#endif
	size_t size = NANX_size_t(info[2]);
	size_t maxnum = NANX_size_t(info[3]);
	size_t num = 0;
	if ((size * maxnum) <= byte_length)
	{
		num = SDL_RWwrite(rwops, ptr, size, maxnum);
	}
	info.GetReturnValue().Set(Nan::New((uint32_t) num));
}

NANX_EXPORT(SDL_RWclose)
{
	SDL_RWops* rwops = WrapRWops::Drop(info[0]); if (!rwops) { return Nan::ThrowError("null SDL_RWops object"); }
	int err = SDL_RWclose(rwops);
	info.GetReturnValue().Set(Nan::New(err));
}

// SDL_scancode.h
// SDL_shape.h
// SDL_stdinc.h

NANX_EXPORT(SDL_getenv)
{
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(info[0]);
	char* env = SDL_getenv(*v8::String::Utf8Value(name));
	info.GetReturnValue().Set(NANX_STRING(env));
}

NANX_EXPORT(SDL_setenv)
{
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(info[0]);
	v8::Local<v8::String> value = v8::Local<v8::String>::Cast(info[1]);
	int overwrite = NANX_int(info[2]);
	int err = SDL_setenv(*v8::String::Utf8Value(name), *v8::String::Utf8Value(value), overwrite);
	info.GetReturnValue().Set(Nan::New(err));
}

// SDL_surface.h

NANX_EXPORT(SDL_CreateRGBSurface)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	int width = NANX_int(info[1]);
	int height = NANX_int(info[2]);
	int depth = NANX_int(info[3]);
	::Uint32 Rmask = NANX_Uint32(info[4]);
	::Uint32 Gmask = NANX_Uint32(info[5]);
	::Uint32 Bmask = NANX_Uint32(info[6]);
	::Uint32 Amask = NANX_Uint32(info[7]);
	SDL_Surface* surface = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
	info.GetReturnValue().Set(WrapSurface::Hold(surface));
}

NANX_EXPORT(SDL_CreateRGBSurfaceFrom)
{
	#if NODE_VERSION_AT_LEAST(4, 0, 0)
	v8::Local<v8::TypedArray> _pixels = v8::Local<v8::TypedArray>::Cast(info[0]);
	void* pixels = static_cast<char*>(_pixels->Buffer()->GetContents().Data()) + _pixels->ByteOffset();
	//size_t byte_length = _pixels->ByteLength();
	#else
	v8::Local<v8::Object> _pixels = v8::Local<v8::Object>::Cast(info[0]);
	void* pixels = (void*) _pixels->GetIndexedPropertiesExternalArrayData();
	//int byte_length = _pixels->GetIndexedPropertiesExternalArrayDataLength();
	#endif
	int width = NANX_int(info[1]);
	int height = NANX_int(info[2]);
	int depth = NANX_int(info[3]);
	int pitch = NANX_int(info[4]);
	::Uint32 Rmask = NANX_Uint32(info[5]);
	::Uint32 Gmask = NANX_Uint32(info[6]);
	::Uint32 Bmask = NANX_Uint32(info[7]);
	::Uint32 Amask = NANX_Uint32(info[8]);
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);
	info.GetReturnValue().Set(WrapSurface::Hold(surface));
}

NANX_EXPORT(SDL_FreeSurface)
{
	SDL_Surface* surface = WrapSurface::Drop(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_FreeSurface(surface);
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfacePalette(SDL_Surface * surface, SDL_Palette * palette);
// TODO: extern DECLSPEC int SDLCALL SDL_LockSurface(SDL_Surface * surface);
// TODO: extern DECLSPEC void SDLCALL SDL_UnlockSurface(SDL_Surface * surface);

NANX_EXPORT(SDL_LoadBMP)
{
	v8::Local<v8::String> file = v8::Local<v8::String>::Cast(info[0]);
	v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[1]);
	int err = Nanx::SimpleTask::Run(new TaskLoadBMP(file, callback));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_SaveBMP)
{
	v8::Local<v8::Value> surface = info[0];
	v8::Local<v8::String> file = v8::Local<v8::String>::Cast(info[1]);
	v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[2]);
	int err = Nanx::SimpleTask::Run(new TaskSaveBMP(surface, file, callback));
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceRLE(SDL_Surface * surface, int flag);
// TODO: extern DECLSPEC int SDLCALL SDL_SetColorKey(SDL_Surface * surface, int flag, Uint32 key);
// TODO: extern DECLSPEC int SDLCALL SDL_GetColorKey(SDL_Surface * surface, Uint32 * key);
// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceColorMod(SDL_Surface * surface, Uint8 r, Uint8 g, Uint8 b);
// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceColorMod(SDL_Surface * surface, Uint8 * r, Uint8 * g, Uint8 * b);
// TODO: extern DECLSPEC int SDLCALL SDL_SetSurfaceAlphaMod(SDL_Surface * surface, Uint8 alpha);
// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceAlphaMod(SDL_Surface * surface, Uint8 * alpha);

// extern DECLSPEC int SDLCALL SDL_SetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode blendMode);
NANX_EXPORT(SDL_SetSurfaceBlendMode)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_BlendMode mode = NANX_SDL_BlendMode(info[1]);
	int err = SDL_SetSurfaceBlendMode(surface, mode);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode *blendMode);
// TODO: extern DECLSPEC SDL_bool SDLCALL SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect);
// TODO: extern DECLSPEC void SDLCALL SDL_GetClipRect(SDL_Surface * surface, SDL_Rect * rect);
// TODO: extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurface (SDL_Surface * src, SDL_PixelFormat * fmt, Uint32 flags);

// extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurfaceFormat (SDL_Surface * src, Uint32 pixel_format, Uint32 flags);
NANX_EXPORT(SDL_ConvertSurfaceFormat)
{
	SDL_Surface* src_surface = WrapSurface::Peek(info[0]); if (!src_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	::Uint32 format = NANX_Uint32(info[1]);
	::Uint32 flags = NANX_Uint32(info[2]);
	SDL_Surface* dst_surface = SDL_ConvertSurfaceFormat(src_surface, format, flags);
	info.GetReturnValue().Set(WrapSurface::Hold(dst_surface));
}

// TODO: extern DECLSPEC int SDLCALL SDL_ConvertPixels(int width, int height, Uint32 src_format, const void * src, int src_pitch, Uint32 dst_format, void * dst, int dst_pitch);

// extern DECLSPEC int SDLCALL SDL_FillRect(SDL_Surface * dst, const SDL_Rect * rect, Uint32 color);
NANX_EXPORT(SDL_FillRect)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	::Uint32 color = NANX_Uint32(info[2]);
	int err = SDL_FillRect(surface, rect, color);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_FillRects(SDL_Surface * dst, const SDL_Rect * rects, int count, Uint32 color);

// #define SDL_BlitSurface SDL_UpperBlit
// extern DECLSPEC int SDLCALL SDL_UpperBlit(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
// extern DECLSPEC int SDLCALL SDL_LowerBlit(SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
NANX_EXPORT(SDL_BlitSurface)
{
	SDL_Surface* src_surface = WrapSurface::Peek(info[0]); if (!src_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* src_rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(info[2]); if (!dst_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* dst_rect = (info[3]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[3]))->GetRect()));
	int err = SDL_BlitSurface(src_surface, src_rect, dst_surface, dst_rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC int SDLCALL SDL_SoftStretch(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, const SDL_Rect * dstrect);
NANX_EXPORT(SDL_SoftStretch)
{
	SDL_Surface* src_surface = WrapSurface::Peek(info[0]); if (!src_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* src_rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(info[2]); if (!dst_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* dst_rect = (info[3]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[3]))->GetRect()));
	int err = SDL_SoftStretch(src_surface, src_rect, dst_surface, dst_rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// #define SDL_BlitScaled SDL_UpperBlitScaled
// extern DECLSPEC int SDLCALL SDL_UpperBlitScaled(SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
// extern DECLSPEC int SDLCALL SDL_LowerBlitScaled(SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect);
NANX_EXPORT(SDL_BlitScaled)
{
	SDL_Surface* src_surface = WrapSurface::Peek(info[0]); if (!src_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* src_rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	SDL_Surface* dst_surface = WrapSurface::Peek(info[2]); if (!dst_surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_Rect* dst_rect = (info[3]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[3]))->GetRect()));
	int err = SDL_BlitScaled(src_surface, src_rect, dst_surface, dst_rect);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_GetPixel)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	int x = NANX_int(info[1]);
	int y = NANX_int(info[2]);
	::Uint32 pixel = _SDL_GetPixel(surface, x, y);
	info.GetReturnValue().Set(Nan::New(pixel));
}

NANX_EXPORT(SDL_PutPixel)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	int x = NANX_int(info[1]);
	int y = NANX_int(info[2]);
	::Uint32 pixel = NANX_Uint32(info[3]);
	_SDL_PutPixel(surface, x, y, pixel);
}

// SDL_system.h

#if defined(__ANDROID__)

NANX_EXPORT(SDL_AndroidGetInternalStoragePath)
{
	info.GetReturnValue().Set(NANX_STRING(SDL_AndroidGetInternalStoragePath()));
}

NANX_EXPORT(SDL_AndroidGetExternalStorageState)
{
	info.GetReturnValue().Set(Nan::New(SDL_AndroidGetExternalStorageState()));
}

NANX_EXPORT(SDL_AndroidGetExternalStoragePath)
{
	info.GetReturnValue().Set(NANX_STRING(SDL_AndroidGetExternalStoragePath()));
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

NANX_EXPORT(SDL_GetTicks)
{
	::Uint32 ticks = SDL_GetTicks();
	info.GetReturnValue().Set(Nan::New(ticks));
}

NANX_EXPORT(SDL_Delay)
{
	::Uint32 ms = NANX_Uint32(info[0]);
	SDL_Delay(ms);
}

// SDL_touch.h

// TODO: extern DECLSPEC int SDLCALL SDL_GetNumTouchDevices(void);
// TODO: extern DECLSPEC SDL_TouchID SDLCALL SDL_GetTouchDevice(int index);
// TODO: extern DECLSPEC int SDLCALL SDL_GetNumTouchFingers(SDL_TouchID touchID);
// TODO: extern DECLSPEC SDL_Finger * SDLCALL SDL_GetTouchFinger(SDL_TouchID touchID, int index);

// SDL_types.h
// SDL_version.h

NANX_EXPORT(SDL_GetRevisionNumber)
{
	info.GetReturnValue().Set(Nan::New(SDL_GetRevisionNumber()));
}

// SDL_video.h

// extern DECLSPEC int SDLCALL SDL_GetNumVideoDrivers(void);
NANX_EXPORT(SDL_GetNumVideoDrivers)
{
	info.GetReturnValue().Set(Nan::New(SDL_GetNumVideoDrivers()));
}

// extern DECLSPEC const char *SDLCALL SDL_GetVideoDriver(int index);
NANX_EXPORT(SDL_GetVideoDriver)
{
	int index = NANX_int(info[0]);
	info.GetReturnValue().Set(NANX_STRING(SDL_GetVideoDriver(index)));
}

// extern DECLSPEC int SDLCALL SDL_VideoInit(const char *driver_name);
NANX_EXPORT(SDL_VideoInit)
{
	v8::Local<v8::String> driver_name = v8::Local<v8::String>::Cast(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_VideoInit(*v8::String::Utf8Value(driver_name))));
}

// extern DECLSPEC void SDLCALL SDL_VideoQuit(void);
NANX_EXPORT(SDL_VideoQuit)
{
	SDL_VideoQuit();
}

// extern DECLSPEC const char *SDLCALL SDL_GetCurrentVideoDriver(void);
NANX_EXPORT(SDL_GetCurrentVideoDriver)
{
	info.GetReturnValue().Set(NANX_STRING(SDL_GetCurrentVideoDriver()));
}

// extern DECLSPEC int SDLCALL SDL_GetNumVideoDisplays(void);
NANX_EXPORT(SDL_GetNumVideoDisplays)
{
	info.GetReturnValue().Set(Nan::New(SDL_GetNumVideoDisplays()));
}

// extern DECLSPEC const char * SDLCALL SDL_GetDisplayName(int displayIndex);
NANX_EXPORT(SDL_GetDisplayName)
{
	int index = NANX_int(info[0]);
	info.GetReturnValue().Set(NANX_STRING(SDL_GetDisplayName(index)));
}

// extern DECLSPEC int SDLCALL SDL_GetDisplayBounds(int displayIndex, SDL_Rect * rect);
NANX_EXPORT(SDL_GetDisplayBounds)
{
	int index = NANX_int(info[0]);
	SDL_Rect* rect = (info[1]->IsNull())?(NULL):(&(WrapRect::Unwrap(v8::Local<v8::Object>::Cast(info[1]))->GetRect()));
	int err = SDL_GetDisplayBounds(index, rect);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC int SDLCALL SDL_GetNumDisplayModes(int displayIndex);
// TODO: extern DECLSPEC int SDLCALL SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode * mode);
// TODO: extern DECLSPEC int SDLCALL SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode * mode);

// TODO: extern DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode * mode);
NANX_EXPORT(SDL_GetCurrentDisplayMode)
{
	int displayIndex = NANX_int(info[0]);
	//v8::Local<v8::Object> _mode = v8::Local<v8::Object>::Cast(info[1]);
	//SDL_DisplayMode mode;
	//int err = SDL_GetCurrentDisplayMode(displayIndex, &mode);
	//_mode->Set(NANX_SYMBOL("format"), Nan::New(mode.format));
	//_mode->Set(NANX_SYMBOL("w"), Nan::New(mode.w));
	//_mode->Set(NANX_SYMBOL("h"), Nan::New(mode.h));
	//_mode->Set(NANX_SYMBOL("refresh_rate"), Nan::New(mode.refresh_rate));
	WrapDisplayMode* mode = WrapDisplayMode::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	if (mode != NULL)
	{
		int err = SDL_GetCurrentDisplayMode(displayIndex, &(mode->GetDisplayMode()));
		info.GetReturnValue().Set(Nan::New(err));
	}
	else
	{
		info.GetReturnValue().Set(Nan::New(-1));
	}
}

// TODO: extern DECLSPEC SDL_DisplayMode * SDLCALL SDL_GetClosestDisplayMode(int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest);

// extern DECLSPEC int SDLCALL SDL_GetWindowDisplayIndex(SDL_Window * window);
NANX_EXPORT(SDL_GetWindowDisplayIndex)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	info.GetReturnValue().Set(Nan::New(SDL_GetWindowDisplayIndex(window)));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetWindowDisplayMode(SDL_Window * window, const SDL_DisplayMode * mode);
// TODO: extern DECLSPEC int SDLCALL SDL_GetWindowDisplayMode(SDL_Window * window, SDL_DisplayMode * mode);

// extern DECLSPEC Uint32 SDLCALL SDL_GetWindowPixelFormat(SDL_Window * window);
NANX_EXPORT(SDL_GetWindowPixelFormat)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	info.GetReturnValue().Set(Nan::New(SDL_GetWindowPixelFormat(window)));
}

NANX_EXPORT(SDL_CreateWindow)
{
	v8::Local<v8::String> title = v8::Local<v8::String>::Cast(info[0]);
	int x = NANX_int(info[1]);
	int y = NANX_int(info[2]);
	int w = NANX_int(info[3]);
	int h = NANX_int(info[4]);
	::Uint32 flags = NANX_Uint32(info[5]);
	SDL_Window* window = SDL_CreateWindow(*v8::String::Utf8Value(title), x, y, w, h, flags);
	info.GetReturnValue().Set(WrapWindow::Hold(window));
}

// TODO: extern DECLSPEC SDL_Window * SDLCALL SDL_CreateWindowFrom(const void *data);
// TODO: extern DECLSPEC Uint32 SDLCALL SDL_GetWindowID(SDL_Window * window);
// TODO: extern DECLSPEC SDL_Window * SDLCALL SDL_GetWindowFromID(Uint32 id);

// extern DECLSPEC Uint32 SDLCALL SDL_GetWindowFlags(SDL_Window * window);
NANX_EXPORT(SDL_GetWindowFlags)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	info.GetReturnValue().Set(Nan::New(SDL_GetWindowFlags(window)));
}

NANX_EXPORT(SDL_SetWindowTitle)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::String> title = v8::Local<v8::String>::Cast(info[1]);
	SDL_SetWindowTitle(window, *v8::String::Utf8Value(title));
}

NANX_EXPORT(SDL_GetWindowTitle)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	const char* title = SDL_GetWindowTitle(window);
	info.GetReturnValue().Set(NANX_STRING(title));
}

NANX_EXPORT(SDL_SetWindowIcon)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_Surface* surface = WrapSurface::Peek(info[1]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }
	SDL_SetWindowIcon(window, surface);
}

// TODO: extern DECLSPEC void* SDLCALL SDL_SetWindowData(SDL_Window * window, const char *name, void *userdata);
// TODO: extern DECLSPEC void *SDLCALL SDL_GetWindowData(SDL_Window * window, const char *name);

// extern DECLSPEC void SDLCALL SDL_SetWindowPosition(SDL_Window * window, int x, int y);
NANX_EXPORT(SDL_SetWindowPosition)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	int x = NANX_int(info[1]);
	int y = NANX_int(info[2]);
	SDL_SetWindowPosition(window, x, y);
}

// extern DECLSPEC void SDLCALL SDL_GetWindowPosition(SDL_Window * window, int *x, int *y);
NANX_EXPORT(SDL_GetWindowPosition)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::Object> position = v8::Local<v8::Object>::Cast(info[1]);
	int x = 0;
	int y = 0;
	SDL_GetWindowPosition(window, &x, &y);
	position->Set(NANX_SYMBOL("x"), Nan::New(x));
	position->Set(NANX_SYMBOL("y"), Nan::New(y));
}

// extern DECLSPEC void SDLCALL SDL_SetWindowSize(SDL_Window * window, int w, int h);
NANX_EXPORT(SDL_SetWindowSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	int w = NANX_int(info[1]);
	int h = NANX_int(info[2]);
	SDL_SetWindowSize(window, w, h);
}

NANX_EXPORT(SDL_GetWindowSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::Object> size = v8::Local<v8::Object>::Cast(info[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(window, &w, &h);
	size->Set(NANX_SYMBOL("w"), Nan::New(w));
	size->Set(NANX_SYMBOL("h"), Nan::New(h));
}

// extern DECLSPEC void SDLCALL SDL_SetWindowMinimumSize(SDL_Window * window, int min_w, int min_h);
NANX_EXPORT(SDL_SetWindowMinimumSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	int w = NANX_int(info[1]);
	int h = NANX_int(info[2]);
	SDL_SetWindowMinimumSize(window, w, h);
}

// extern DECLSPEC void SDLCALL SDL_GetWindowMinimumSize(SDL_Window * window, int *w, int *h);
NANX_EXPORT(SDL_GetWindowMinimumSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::Object> size = v8::Local<v8::Object>::Cast(info[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowMinimumSize(window, &w, &h);
	size->Set(NANX_SYMBOL("w"), Nan::New(w));
	size->Set(NANX_SYMBOL("h"), Nan::New(h));
}

// extern DECLSPEC void SDLCALL SDL_SetWindowMaximumSize(SDL_Window * window, int max_w, int max_h);
NANX_EXPORT(SDL_SetWindowMaximumSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	int w = NANX_int(info[1]);
	int h = NANX_int(info[2]);
	SDL_SetWindowMaximumSize(window, w, h);
}

// extern DECLSPEC void SDLCALL SDL_GetWindowMaximumSize(SDL_Window * window, int *w, int *h);
NANX_EXPORT(SDL_GetWindowMaximumSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::Object> size = v8::Local<v8::Object>::Cast(info[1]);
	int w = 0;
	int h = 0;
	SDL_GetWindowMaximumSize(window, &w, &h);
	size->Set(NANX_SYMBOL("w"), Nan::New(w));
	size->Set(NANX_SYMBOL("h"), Nan::New(h));
}

// extern DECLSPEC void SDLCALL SDL_SetWindowBordered(SDL_Window * window, SDL_bool bordered);
NANX_EXPORT(SDL_SetWindowBordered)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_bool bordered = NANX_SDL_bool(info[1]);
	SDL_SetWindowBordered(window, bordered);
}

// extern DECLSPEC void SDLCALL SDL_ShowWindow(SDL_Window * window);
NANX_EXPORT(SDL_ShowWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_ShowWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_HideWindow(SDL_Window * window);
NANX_EXPORT(SDL_HideWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_HideWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_RaiseWindow(SDL_Window * window);
NANX_EXPORT(SDL_RaiseWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_RaiseWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_MaximizeWindow(SDL_Window * window);
NANX_EXPORT(SDL_MaximizeWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_MaximizeWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_MinimizeWindow(SDL_Window * window);
NANX_EXPORT(SDL_MinimizeWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_MinimizeWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_RestoreWindow(SDL_Window * window);
NANX_EXPORT(SDL_RestoreWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_RestoreWindow(window);
}

// extern DECLSPEC int SDLCALL SDL_SetWindowFullscreen(SDL_Window * window, Uint32 flags);
NANX_EXPORT(SDL_SetWindowFullscreen)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	::Uint32 flags = NANX_Uint32(info[1]);
	int err = SDL_SetWindowFullscreen(window, flags);
	info.GetReturnValue().Set(Nan::New(err));
}

// TODO: extern DECLSPEC SDL_Surface * SDLCALL SDL_GetWindowSurface(SDL_Window * window);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateWindowSurface(SDL_Window * window);
// TODO: extern DECLSPEC int SDLCALL SDL_UpdateWindowSurfaceRects(SDL_Window * window, const SDL_Rect * rects, int numrects);

// extern DECLSPEC void SDLCALL SDL_SetWindowGrab(SDL_Window * window, SDL_bool grabbed);
NANX_EXPORT(SDL_SetWindowGrab)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_bool grab = NANX_SDL_bool(info[1]);
	SDL_SetWindowGrab(window, grab);
}

// extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowGrab(SDL_Window * window);
NANX_EXPORT(SDL_GetWindowGrab)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_bool grab = SDL_GetWindowGrab(window);
	info.GetReturnValue().Set(Nan::New(grab != SDL_FALSE));
}

// extern DECLSPEC int SDLCALL SDL_SetWindowBrightness(SDL_Window * window, float brightness);
NANX_EXPORT(SDL_SetWindowBrightness)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	float brightness = NANX_float(info[1]);
	SDL_SetWindowBrightness(window, brightness);
}

// extern DECLSPEC float SDLCALL SDL_GetWindowBrightness(SDL_Window * window);
NANX_EXPORT(SDL_GetWindowBrightness)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	float brightness = SDL_GetWindowBrightness(window);
	info.GetReturnValue().Set(Nan::New(brightness));
}

// TODO: extern DECLSPEC int SDLCALL SDL_SetWindowGammaRamp(SDL_Window * window, const Uint16 * red, const Uint16 * green, const Uint16 * blue);
// TODO: extern DECLSPEC int SDLCALL SDL_GetWindowGammaRamp(SDL_Window * window, Uint16 * red, Uint16 * green, Uint16 * blue);

// extern DECLSPEC void SDLCALL SDL_DestroyWindow(SDL_Window * window);
NANX_EXPORT(SDL_DestroyWindow)
{
	SDL_Window* window = WrapWindow::Drop(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_DestroyWindow(window);
}

// extern DECLSPEC SDL_bool SDLCALL SDL_IsScreenSaverEnabled(void);
NANX_EXPORT(SDL_IsScreenSaverEnabled)
{
	info.GetReturnValue().Set(Nan::New(SDL_IsScreenSaverEnabled() != SDL_FALSE));
}

// extern DECLSPEC void SDLCALL SDL_EnableScreenSaver(void);
NANX_EXPORT(SDL_EnableScreenSaver)
{
	SDL_EnableScreenSaver();
}

// extern DECLSPEC void SDLCALL SDL_DisableScreenSaver(void);
NANX_EXPORT(SDL_DisableScreenSaver)
{
	SDL_DisableScreenSaver();
}

// TODO: extern DECLSPEC int SDLCALL SDL_GL_LoadLibrary(const char *path);
// TODO: extern DECLSPEC void *SDLCALL SDL_GL_GetProcAddress(const char *proc);
// TODO: extern DECLSPEC void SDLCALL SDL_GL_UnloadLibrary(void);

// extern DECLSPEC SDL_bool SDLCALL SDL_GL_ExtensionSupported(const char *extension);
NANX_EXPORT(SDL_GL_ExtensionSupported)
{
	v8::Local<v8::String> extension = v8::Local<v8::String>::Cast(info[0]);
	info.GetReturnValue().Set(Nan::New(SDL_GL_ExtensionSupported(*v8::String::Utf8Value(extension)) != SDL_FALSE));
}

// extern DECLSPEC int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value);
NANX_EXPORT(SDL_GL_SetAttribute)
{
	SDL_GLattr attr = NANX_SDL_GLattr(info[0]);
	int value = NANX_int(info[1]);
	int err = SDL_GL_SetAttribute(attr, value);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int *value);
NANX_EXPORT(SDL_GL_GetAttribute)
{
	SDL_GLattr attr = NANX_SDL_GLattr(info[0]);
	v8::Local<v8::Array> ret_value = v8::Local<v8::Array>::Cast(info[1]);
	int value = 0;
	int err = SDL_GL_GetAttribute(attr, &value);
	ret_value->Set(0, Nan::New(value));
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window * window);
NANX_EXPORT(SDL_GL_CreateContext)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	//SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GLContext* gl_context = new SDL_GLContext(); *gl_context = SDL_GL_CreateContext(window);
	info.GetReturnValue().Set(WrapGLContext::Hold(gl_context));
}

// extern DECLSPEC int SDLCALL SDL_GL_MakeCurrent(SDL_Window * window, SDL_GLContext context);
NANX_EXPORT(SDL_GL_MakeCurrent)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	if (info[1]->IsNull())
	{
		int err = SDL_GL_MakeCurrent(window, NULL);
		_gl_current_window.Reset();
		_gl_current_context.Reset();
		info.GetReturnValue().Set(Nan::New(err));
	}
	else
	{
		SDL_GLContext* gl_context = WrapGLContext::Peek(info[1]); if (!gl_context) { return Nan::ThrowError("null SDL_GLContext object"); }
		int err = SDL_GL_MakeCurrent(window, *gl_context);
		_gl_current_window.Reset(info[0]);
		_gl_current_context.Reset(info[1]);
		info.GetReturnValue().Set(Nan::New(err));
	}
}

// extern DECLSPEC SDL_Window* SDLCALL SDL_GL_GetCurrentWindow(void);
NANX_EXPORT(SDL_GL_GetCurrentWindow)
{
	if (_gl_current_window.IsEmpty())
	{
		SDL_assert(SDL_GL_GetCurrentWindow() == NULL);
		info.GetReturnValue().SetNull();
	}
	else
	{
		SDL_assert(SDL_GL_GetCurrentWindow() == WrapWindow::Peek(Nan::New<v8::Value>(_gl_current_window)));
		info.GetReturnValue().Set(Nan::New(_gl_current_window));
	}
}

// extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_GetCurrentContext(void);
NANX_EXPORT(SDL_GL_GetCurrentContext)
{
	if (_gl_current_context.IsEmpty())
	{
		SDL_assert(SDL_GL_GetCurrentContext() == NULL);
		info.GetReturnValue().SetNull();
	}
	else
	{
		SDL_assert(SDL_GL_GetCurrentContext() == WrapWindow::Peek(Nan::New<v8::Value>(_gl_current_context)));
		info.GetReturnValue().Set(Nan::New(_gl_current_context));
	}
}

// extern DECLSPEC void SDLCALL SDL_GL_GetDrawableSize(SDL_Window * window, int *w, int *h);
NANX_EXPORT(SDL_GL_GetDrawableSize)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	v8::Local<v8::Object> size = v8::Local<v8::Object>::Cast(info[1]);
	int w = 0;
	int h = 0;
	SDL_GL_GetDrawableSize(window, &w, &h);
	size->Set(NANX_SYMBOL("w"), Nan::New(w));
	size->Set(NANX_SYMBOL("h"), Nan::New(h));
}

// extern DECLSPEC int SDLCALL SDL_GL_SetSwapInterval(int interval);
NANX_EXPORT(SDL_GL_SetSwapInterval)
{
	int interval = NANX_int(info[0]);
	int err = SDL_GL_SetSwapInterval(interval);
	info.GetReturnValue().Set(Nan::New(err));
}

// extern DECLSPEC int SDLCALL SDL_GL_GetSwapInterval(void);
NANX_EXPORT(SDL_GL_GetSwapInterval)
{
	int interval = SDL_GL_GetSwapInterval();
	info.GetReturnValue().Set(Nan::New(interval));
}

// extern DECLSPEC void SDLCALL SDL_GL_SwapWindow(SDL_Window * window);
NANX_EXPORT(SDL_GL_SwapWindow)
{
	SDL_Window* window = WrapWindow::Peek(info[0]); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
	SDL_GL_SwapWindow(window);
}

// extern DECLSPEC void SDLCALL SDL_GL_DeleteContext(SDL_GLContext context);
NANX_EXPORT(SDL_GL_DeleteContext)
{
	if (WrapGLContext::Peek(info[0]) == WrapGLContext::Peek(Nan::New<v8::Value>(_gl_current_context)))
	{
		SDL_Window* window = WrapWindow::Peek(Nan::New<v8::Value>(_gl_current_window)); if (!window) { return Nan::ThrowError("null SDL_Window object"); }
		SDL_GL_MakeCurrent(window, NULL);
		_gl_current_window.Reset();
		_gl_current_context.Reset();
	}
	SDL_GLContext* gl_context = WrapGLContext::Drop(info[0]); if (!gl_context) { return Nan::ThrowError("null SDL_GLContext object"); }
	SDL_GL_DeleteContext(*gl_context); delete gl_context; gl_context = NULL;
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

class SurfaceToImageDataTask : public Nanx::SimpleTask
{
public:
	Nan::Persistent<v8::Value> m_hold_surface;
	Nan::Persistent<v8::Function> m_callback;
	Nan::Persistent<v8::Object> m_image_data;
	SDL_Surface* m_surface;
	int m_length;
	void* m_pixels;
public:
	SurfaceToImageDataTask(v8::Local<v8::Value> surface, v8::Local<v8::Function> callback) :
		m_surface(WrapSurface::Peek(surface)),
		m_length(0),
		m_pixels(NULL)
	{

		m_hold_surface.Reset(surface);
		m_callback.Reset(callback);
		m_image_data.Reset(Nan::New<v8::Object>());

        static const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format

		int w = (m_surface)?(m_surface->w):(0);
		int h = (m_surface)?(m_surface->h):(0);
		m_length = w * h * SDL_BYTESPERPIXEL(format);

		// find the Uint8ClampedArray constructor and create a buffer
		v8::Local<v8::Object> global = Nan::GetCurrentContext()->Global();
		v8::Local<v8::Function> ctor = v8::Local<v8::Function>::Cast(global->Get(NANX_SYMBOL("Uint8ClampedArray")));
		v8::Local<v8::Value> argv[] = { Nan::New(m_length) };
		v8::Local<v8::Object> data = ctor->NewInstance(countof(argv), argv);

		Nan::New<v8::Object>(m_image_data)->ForceSet(NANX_SYMBOL("width"), Nan::New(w), v8::ReadOnly);
		Nan::New<v8::Object>(m_image_data)->ForceSet(NANX_SYMBOL("height"), Nan::New(h), v8::ReadOnly);
		Nan::New<v8::Object>(m_image_data)->ForceSet(NANX_SYMBOL("data"), data, v8::ReadOnly);

		#if NODE_VERSION_AT_LEAST(4, 0, 0)
		v8::Local<v8::TypedArray> _pixels = v8::Local<v8::TypedArray>::Cast(data);
		m_pixels = static_cast<char*>(_pixels->Buffer()->GetContents().Data()) + _pixels->ByteOffset();
		#else
		v8::Local<v8::Object> _pixels = v8::Local<v8::Object>::Cast(data);
		m_pixels = _pixels->GetIndexedPropertiesExternalArrayData();
		#endif
	}
	~SurfaceToImageDataTask()
	{
		m_hold_surface.Reset();
		m_callback.Reset();
		m_image_data.Reset();
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
		Nan::HandleScope scope;
		v8::Local<v8::Value> argv[] = { Nan::New<v8::Object>(m_image_data) };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<v8::Function>(m_callback), countof(argv), argv);
	}
};

NANX_EXPORT(SDL_EXT_SurfaceToImageDataAsync)
{
	v8::Local<v8::Value> surface = info[0];
	v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[1]);
	int err = Nanx::SimpleTask::Run(new SurfaceToImageDataTask(surface, callback));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(SDL_EXT_SurfaceToImageData)
{
	SDL_Surface* surface = WrapSurface::Peek(info[0]); if (!surface) { return Nan::ThrowError("null SDL_Surface object"); }

	const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format

	int length = surface->w * surface->h * SDL_BYTESPERPIXEL(format);

	// find the Uint8ClampedArray constructor and create a buffer
	v8::Local<v8::Object> global = Nan::GetCurrentContext()->Global();
	v8::Local<v8::Function> ctor = v8::Local<v8::Function>::Cast(global->Get(NANX_SYMBOL("Uint8ClampedArray")));
	v8::Local<v8::Value> argv[] = { Nan::New(length) };
	v8::Local<v8::Object> data = ctor->NewInstance(countof(argv), argv);

	v8::Local<v8::Object> image_data = Nan::New<v8::Object>();
	image_data->ForceSet(NANX_SYMBOL("width"), Nan::New(surface->w), v8::ReadOnly);
	image_data->ForceSet(NANX_SYMBOL("height"), Nan::New(surface->h), v8::ReadOnly);
	image_data->ForceSet(NANX_SYMBOL("data"), data, v8::ReadOnly);

	#if NODE_VERSION_AT_LEAST(4, 0, 0)
	v8::Local<v8::TypedArray> _pixels = v8::Local<v8::TypedArray>::Cast(data);
	void* pixels = static_cast<char*>(_pixels->Buffer()->GetContents().Data()) + _pixels->ByteOffset();
	#else
	v8::Local<v8::Object> _pixels = v8::Local<v8::Object>::Cast(data);
	void* pixels = (void*) _pixels->GetIndexedPropertiesExternalArrayData();
	#endif

    SurfaceToImageData(surface, pixels, length);

	info.GetReturnValue().Set(image_data);
}

NANX_EXPORT(SDL_EXT_ImageDataToSurface)
{
	// TODO: make this async?
	v8::Local<v8::Object> image_data = v8::Local<v8::Object>::Cast(info[0]);
	v8::Local<v8::Integer> width = v8::Local<v8::Integer>::Cast(image_data->Get(NANX_SYMBOL("width")));
	v8::Local<v8::Integer> height = v8::Local<v8::Integer>::Cast(image_data->Get(NANX_SYMBOL("height")));
	v8::Local<v8::Object> data = v8::Local<v8::Object>::Cast(image_data->Get(NANX_SYMBOL("data")));
	const ::Uint32 format = SDL_PIXELFORMAT_ABGR8888; // ImageData pixel format
	#if NODE_VERSION_AT_LEAST(4, 0, 0)
	v8::Local<v8::TypedArray> _pixels = v8::Local<v8::TypedArray>::Cast(data);
	void* pixels = static_cast<char*>(_pixels->Buffer()->GetContents().Data()) + _pixels->ByteOffset();
	#else
	v8::Local<v8::Object> _pixels = v8::Local<v8::Object>::Cast(data);
	void* pixels = (void*) _pixels->GetIndexedPropertiesExternalArrayData();
	#endif
	int w = NANX_int(width);
	int h = NANX_int(height);
	int depth = 0;
	int pitch = w * SDL_BYTESPERPIXEL(format);
	::Uint32 Rmask = 0, Gmask = 0, Bmask = 0, Amask = 0;
	SDL_PixelFormatEnumToMasks(format, &depth, &Rmask, &Gmask, &Bmask, &Amask);
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, w, h, depth, pitch, Rmask, Gmask, Bmask, Amask);
	info.GetReturnValue().Set(WrapSurface::Hold(surface));
}

NAN_MODULE_INIT(init)
{
	#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
	SDL_SetMainReady();
	#endif

	WrapDisplayMode::Init(target);
	WrapColor::Init(target);
	WrapPoint::Init(target);
	WrapRect::Init(target);

	// SDL.h

	NANX_CONSTANT(target, SDL_INIT_TIMER);
	NANX_CONSTANT(target, SDL_INIT_AUDIO);
	NANX_CONSTANT(target, SDL_INIT_VIDEO);
	NANX_CONSTANT(target, SDL_INIT_JOYSTICK);
	NANX_CONSTANT(target, SDL_INIT_HAPTIC);
	NANX_CONSTANT(target, SDL_INIT_GAMECONTROLLER);
	NANX_CONSTANT(target, SDL_INIT_EVENTS);
	NANX_CONSTANT(target, SDL_INIT_NOPARACHUTE);
	NANX_CONSTANT(target, SDL_INIT_EVERYTHING);

	NANX_EXPORT_APPLY(target, SDL_Init);
	NANX_EXPORT_APPLY(target, SDL_InitSubSystem);
	NANX_EXPORT_APPLY(target, SDL_QuitSubSystem);
	NANX_EXPORT_APPLY(target, SDL_WasInit);
	NANX_EXPORT_APPLY(target, SDL_Quit);

	// SDL_assert.h
	// SDL_atomic.h
	// SDL_audio.h

	NANX_CONSTANT(target, AUDIO_U8);
	NANX_CONSTANT(target, AUDIO_S8);
	NANX_CONSTANT(target, AUDIO_U16LSB);
	NANX_CONSTANT(target, AUDIO_S16LSB);
	NANX_CONSTANT(target, AUDIO_U16MSB);
	NANX_CONSTANT(target, AUDIO_S16MSB);
	NANX_CONSTANT(target, AUDIO_U16);
	NANX_CONSTANT(target, AUDIO_S16);

	NANX_CONSTANT(target, AUDIO_S32LSB);
	NANX_CONSTANT(target, AUDIO_S32MSB);
	NANX_CONSTANT(target, AUDIO_S32);

	NANX_CONSTANT(target, AUDIO_F32LSB);
	NANX_CONSTANT(target, AUDIO_F32MSB);
	NANX_CONSTANT(target, AUDIO_F32);

	NANX_CONSTANT(target, AUDIO_U16SYS);
	NANX_CONSTANT(target, AUDIO_S16SYS);
	NANX_CONSTANT(target, AUDIO_S32SYS);
	NANX_CONSTANT(target, AUDIO_F32SYS);

	NANX_CONSTANT(target, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	NANX_CONSTANT(target, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	NANX_CONSTANT(target, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
	NANX_CONSTANT(target, SDL_AUDIO_ALLOW_ANY_CHANGE);

	NANX_CONSTANT(target, SDL_MIX_MAXVOLUME);

	// SDL_bits.h

	// SDL_blendmode.h

	// SDL_BlendMode
	v8::Local<v8::Object> BlendMode = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_BlendMode"), BlendMode);
	NANX_CONSTANT(BlendMode, SDL_BLENDMODE_NONE);
	NANX_CONSTANT(BlendMode, SDL_BLENDMODE_BLEND);
	NANX_CONSTANT(BlendMode, SDL_BLENDMODE_ADD);
	NANX_CONSTANT(BlendMode, SDL_BLENDMODE_MOD);

	// SDL_clipboard.h

	NANX_EXPORT_APPLY(target, SDL_SetClipboardText);
	NANX_EXPORT_APPLY(target, SDL_GetClipboardText);
	NANX_EXPORT_APPLY(target, SDL_HasClipboardText);

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

	NANX_CONSTANT(target, SDL_LIL_ENDIAN);
	NANX_CONSTANT(target, SDL_BIG_ENDIAN);
	NANX_CONSTANT(target, SDL_BYTEORDER);

	// SDL_error.h

	NANX_EXPORT_APPLY(target, SDL_GetError);
	NANX_EXPORT_APPLY(target, SDL_ClearError);

	// SDL_events.h

	NANX_CONSTANT(target, SDL_RELEASED);
	NANX_CONSTANT(target, SDL_PRESSED);

	// SDL_EventType
	v8::Local<v8::Object> EventType = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_EventType"), EventType);
	NANX_CONSTANT(EventType, SDL_FIRSTEVENT);
	NANX_CONSTANT(EventType, SDL_QUIT);
	NANX_CONSTANT(EventType, SDL_APP_TERMINATING);
	NANX_CONSTANT(EventType, SDL_APP_LOWMEMORY);
	NANX_CONSTANT(EventType, SDL_APP_WILLENTERBACKGROUND);
	NANX_CONSTANT(EventType, SDL_APP_DIDENTERBACKGROUND);
	NANX_CONSTANT(EventType, SDL_APP_WILLENTERFOREGROUND);
	NANX_CONSTANT(EventType, SDL_APP_DIDENTERFOREGROUND);
	NANX_CONSTANT(EventType, SDL_WINDOWEVENT);
	NANX_CONSTANT(EventType, SDL_SYSWMEVENT);
	NANX_CONSTANT(EventType, SDL_KEYDOWN);
	NANX_CONSTANT(EventType, SDL_KEYUP);
	NANX_CONSTANT(EventType, SDL_TEXTEDITING);
	NANX_CONSTANT(EventType, SDL_TEXTINPUT);
	NANX_CONSTANT(EventType, SDL_MOUSEMOTION);
	NANX_CONSTANT(EventType, SDL_MOUSEBUTTONDOWN);
	NANX_CONSTANT(EventType, SDL_MOUSEBUTTONUP);
	NANX_CONSTANT(EventType, SDL_JOYAXISMOTION);
	NANX_CONSTANT(EventType, SDL_JOYBALLMOTION);
	NANX_CONSTANT(EventType, SDL_JOYHATMOTION);
	NANX_CONSTANT(EventType, SDL_JOYBUTTONDOWN);
	NANX_CONSTANT(EventType, SDL_JOYBUTTONUP);
	NANX_CONSTANT(EventType, SDL_JOYDEVICEADDED);
	NANX_CONSTANT(EventType, SDL_JOYDEVICEREMOVED);
	NANX_CONSTANT(EventType, SDL_CONTROLLERAXISMOTION);
	NANX_CONSTANT(EventType, SDL_CONTROLLERBUTTONDOWN);
	NANX_CONSTANT(EventType, SDL_CONTROLLERBUTTONUP);
	NANX_CONSTANT(EventType, SDL_CONTROLLERDEVICEADDED);
	NANX_CONSTANT(EventType, SDL_CONTROLLERDEVICEREMOVED);
	NANX_CONSTANT(EventType, SDL_CONTROLLERDEVICEREMAPPED);
	NANX_CONSTANT(EventType, SDL_FINGERDOWN);
	NANX_CONSTANT(EventType, SDL_FINGERUP);
	NANX_CONSTANT(EventType, SDL_FINGERMOTION);
	NANX_CONSTANT(EventType, SDL_DOLLARGESTURE);
	NANX_CONSTANT(EventType, SDL_DOLLARRECORD);
	NANX_CONSTANT(EventType, SDL_MULTIGESTURE);
	NANX_CONSTANT(EventType, SDL_CLIPBOARDUPDATE);
	NANX_CONSTANT(EventType, SDL_DROPFILE);
	#if SDL_VERSION_ATLEAST(2,0,4)
    NANX_CONSTANT(EventType, SDL_AUDIODEVICEADDED);
    NANX_CONSTANT(EventType, SDL_AUDIODEVICEREMOVED);
    NANX_CONSTANT(EventType, SDL_RENDER_TARGETS_RESET);
    NANX_CONSTANT(EventType, SDL_RENDER_DEVICE_RESET);
    #endif
	NANX_CONSTANT(EventType, SDL_USEREVENT);
	NANX_CONSTANT(EventType, SDL_LASTEVENT);

	NANX_EXPORT_APPLY(target, SDL_PollEvent);

	NANX_CONSTANT(target, SDL_QUERY);
	NANX_CONSTANT(target, SDL_IGNORE);
	NANX_CONSTANT(target, SDL_DISABLE);
	NANX_CONSTANT(target, SDL_ENABLE);

	// SDL_gamecontroller.h
	// SDL_gesture.h
	// SDL_haptic.h

	// SDL_hints.h

	NANX_CONSTANT_STRING(target, SDL_HINT_FRAMEBUFFER_ACCELERATION);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_DRIVER);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_OPENGL_SHADERS);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_DIRECT3D_THREADSAFE);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_DIRECT3D11_DEBUG);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_SCALE_QUALITY);
	NANX_CONSTANT_STRING(target, SDL_HINT_RENDER_VSYNC);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_ALLOW_SCREENSAVER);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_X11_XVIDMODE);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_X11_XINERAMA);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_X11_XRANDR);
	//NANX_CONSTANT_STRING(target, SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN);
	//NANX_CONSTANT_STRING(target, SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP);
	NANX_CONSTANT_STRING(target, SDL_HINT_GRAB_KEYBOARD);
	NANX_CONSTANT_STRING(target, SDL_HINT_MOUSE_RELATIVE_MODE_WARP);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
	NANX_CONSTANT_STRING(target, SDL_HINT_IDLE_TIMER_DISABLED);
	NANX_CONSTANT_STRING(target, SDL_HINT_ORIENTATIONS);
	NANX_CONSTANT_STRING(target, SDL_HINT_ACCELEROMETER_AS_JOYSTICK);
	NANX_CONSTANT_STRING(target, SDL_HINT_XINPUT_ENABLED);
	//NANX_CONSTANT_STRING(target, SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING);
	NANX_CONSTANT_STRING(target, SDL_HINT_GAMECONTROLLERCONFIG);
	NANX_CONSTANT_STRING(target, SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS);
	NANX_CONSTANT_STRING(target, SDL_HINT_ALLOW_TOPMOST);
	NANX_CONSTANT_STRING(target, SDL_HINT_TIMER_RESOLUTION);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_HIGHDPI_DISABLED);
	NANX_CONSTANT_STRING(target, SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_WIN_D3DCOMPILER);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT);
	NANX_CONSTANT_STRING(target, SDL_HINT_WINRT_PRIVACY_POLICY_URL);
	NANX_CONSTANT_STRING(target, SDL_HINT_WINRT_PRIVACY_POLICY_LABEL);
	NANX_CONSTANT_STRING(target, SDL_HINT_WINRT_HANDLE_BACK_BUTTON);
	NANX_CONSTANT_STRING(target, SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES);
	//NANX_CONSTANT_STRING(target, SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION);
	//NANX_CONSTANT_STRING(target, SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION);
	//NANX_CONSTANT_STRING(target, SDL_HINT_IME_INTERNAL_EDITING);
	//NANX_CONSTANT_STRING(target, SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH);
	//NANX_CONSTANT_STRING(target, SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT);
	//NANX_CONSTANT_STRING(target, SDL_HINT_NO_SIGNAL_HANDLERS);

	// SDL_HintPriority
	v8::Local<v8::Object> HintPriority = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_HintPriority"), HintPriority);
	NANX_CONSTANT(HintPriority, SDL_HINT_DEFAULT);
	NANX_CONSTANT(HintPriority, SDL_HINT_NORMAL);
	NANX_CONSTANT(HintPriority, SDL_HINT_OVERRIDE);

	NANX_EXPORT_APPLY(target, SDL_SetHintWithPriority);
	NANX_EXPORT_APPLY(target, SDL_SetHint);
	NANX_EXPORT_APPLY(target, SDL_GetHint);
	NANX_EXPORT_APPLY(target, SDL_ClearHints);

	// SDL_joystick.h

	NANX_CONSTANT(target, SDL_HAT_CENTERED);
	NANX_CONSTANT(target, SDL_HAT_UP);
	NANX_CONSTANT(target, SDL_HAT_RIGHT);
	NANX_CONSTANT(target, SDL_HAT_DOWN);
	NANX_CONSTANT(target, SDL_HAT_LEFT);
	NANX_CONSTANT(target, SDL_HAT_RIGHTUP);
	NANX_CONSTANT(target, SDL_HAT_RIGHTDOWN);
	NANX_CONSTANT(target, SDL_HAT_LEFTUP);
	NANX_CONSTANT(target, SDL_HAT_LEFTDOWN);

	NANX_EXPORT_APPLY(target, SDL_NumJoysticks);
	NANX_EXPORT_APPLY(target, SDL_JoystickNameForIndex);
	NANX_EXPORT_APPLY(target, SDL_JoystickOpen);
	NANX_EXPORT_APPLY(target, SDL_JoystickName);
	NANX_EXPORT_APPLY(target, SDL_JoystickGetAttached);
	NANX_EXPORT_APPLY(target, SDL_JoystickInstanceID);
	NANX_EXPORT_APPLY(target, SDL_JoystickNumAxes);
	NANX_EXPORT_APPLY(target, SDL_JoystickNumBalls);
	NANX_EXPORT_APPLY(target, SDL_JoystickNumHats);
	NANX_EXPORT_APPLY(target, SDL_JoystickNumButtons);
	NANX_EXPORT_APPLY(target, SDL_JoystickUpdate);
	NANX_EXPORT_APPLY(target, SDL_JoystickEventState);
	NANX_EXPORT_APPLY(target, SDL_JoystickGetAxis);
	NANX_EXPORT_APPLY(target, SDL_JoystickGetBall);
	NANX_EXPORT_APPLY(target, SDL_JoystickGetHat);
	NANX_EXPORT_APPLY(target, SDL_JoystickGetButton);
	NANX_EXPORT_APPLY(target, SDL_JoystickClose);

	// SDL_keyboard.h
	// SDL_keycode.h

	// SDL_Keycode
	v8::Local<v8::Object> Keycode = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_Keycode"), Keycode);
	NANX_CONSTANT(Keycode, SDLK_UNKNOWN);
	NANX_CONSTANT(Keycode, SDLK_RETURN);
	NANX_CONSTANT(Keycode, SDLK_ESCAPE);
	NANX_CONSTANT(Keycode, SDLK_BACKSPACE);
	NANX_CONSTANT(Keycode, SDLK_TAB);
	NANX_CONSTANT(Keycode, SDLK_SPACE);
	NANX_CONSTANT(Keycode, SDLK_EXCLAIM);
	NANX_CONSTANT(Keycode, SDLK_QUOTEDBL);
	NANX_CONSTANT(Keycode, SDLK_HASH);
	NANX_CONSTANT(Keycode, SDLK_PERCENT);
	NANX_CONSTANT(Keycode, SDLK_DOLLAR);
	NANX_CONSTANT(Keycode, SDLK_AMPERSAND);
	NANX_CONSTANT(Keycode, SDLK_QUOTE);
	NANX_CONSTANT(Keycode, SDLK_LEFTPAREN);
	NANX_CONSTANT(Keycode, SDLK_RIGHTPAREN);
	NANX_CONSTANT(Keycode, SDLK_ASTERISK);
	NANX_CONSTANT(Keycode, SDLK_PLUS);
	NANX_CONSTANT(Keycode, SDLK_COMMA);
	NANX_CONSTANT(Keycode, SDLK_MINUS);
	NANX_CONSTANT(Keycode, SDLK_PERIOD);
	NANX_CONSTANT(Keycode, SDLK_SLASH);
	NANX_CONSTANT(Keycode, SDLK_0);
	NANX_CONSTANT(Keycode, SDLK_1);
	NANX_CONSTANT(Keycode, SDLK_2);
	NANX_CONSTANT(Keycode, SDLK_3);
	NANX_CONSTANT(Keycode, SDLK_4);
	NANX_CONSTANT(Keycode, SDLK_5);
	NANX_CONSTANT(Keycode, SDLK_6);
	NANX_CONSTANT(Keycode, SDLK_7);
	NANX_CONSTANT(Keycode, SDLK_8);
	NANX_CONSTANT(Keycode, SDLK_9);
	NANX_CONSTANT(Keycode, SDLK_COLON);
	NANX_CONSTANT(Keycode, SDLK_SEMICOLON);
	NANX_CONSTANT(Keycode, SDLK_LESS);
	NANX_CONSTANT(Keycode, SDLK_EQUALS);
	NANX_CONSTANT(Keycode, SDLK_GREATER);
	NANX_CONSTANT(Keycode, SDLK_QUESTION);
	NANX_CONSTANT(Keycode, SDLK_AT);
	NANX_CONSTANT(Keycode, SDLK_LEFTBRACKET);
	NANX_CONSTANT(Keycode, SDLK_BACKSLASH);
	NANX_CONSTANT(Keycode, SDLK_RIGHTBRACKET);
	NANX_CONSTANT(Keycode, SDLK_CARET);
	NANX_CONSTANT(Keycode, SDLK_UNDERSCORE);
	NANX_CONSTANT(Keycode, SDLK_BACKQUOTE);
	NANX_CONSTANT(Keycode, SDLK_a);
	NANX_CONSTANT(Keycode, SDLK_b);
	NANX_CONSTANT(Keycode, SDLK_c);
	NANX_CONSTANT(Keycode, SDLK_d);
	NANX_CONSTANT(Keycode, SDLK_e);
	NANX_CONSTANT(Keycode, SDLK_f);
	NANX_CONSTANT(Keycode, SDLK_g);
	NANX_CONSTANT(Keycode, SDLK_h);
	NANX_CONSTANT(Keycode, SDLK_i);
	NANX_CONSTANT(Keycode, SDLK_j);
	NANX_CONSTANT(Keycode, SDLK_k);
	NANX_CONSTANT(Keycode, SDLK_l);
	NANX_CONSTANT(Keycode, SDLK_m);
	NANX_CONSTANT(Keycode, SDLK_n);
	NANX_CONSTANT(Keycode, SDLK_o);
	NANX_CONSTANT(Keycode, SDLK_p);
	NANX_CONSTANT(Keycode, SDLK_q);
	NANX_CONSTANT(Keycode, SDLK_r);
	NANX_CONSTANT(Keycode, SDLK_s);
	NANX_CONSTANT(Keycode, SDLK_t);
	NANX_CONSTANT(Keycode, SDLK_u);
	NANX_CONSTANT(Keycode, SDLK_v);
	NANX_CONSTANT(Keycode, SDLK_w);
	NANX_CONSTANT(Keycode, SDLK_x);
	NANX_CONSTANT(Keycode, SDLK_y);
	NANX_CONSTANT(Keycode, SDLK_z);
	NANX_CONSTANT(Keycode, SDLK_CAPSLOCK);
	NANX_CONSTANT(Keycode, SDLK_F1);
	NANX_CONSTANT(Keycode, SDLK_F2);
	NANX_CONSTANT(Keycode, SDLK_F3);
	NANX_CONSTANT(Keycode, SDLK_F4);
	NANX_CONSTANT(Keycode, SDLK_F5);
	NANX_CONSTANT(Keycode, SDLK_F6);
	NANX_CONSTANT(Keycode, SDLK_F7);
	NANX_CONSTANT(Keycode, SDLK_F8);
	NANX_CONSTANT(Keycode, SDLK_F9);
	NANX_CONSTANT(Keycode, SDLK_F10);
	NANX_CONSTANT(Keycode, SDLK_F11);
	NANX_CONSTANT(Keycode, SDLK_F12);
	NANX_CONSTANT(Keycode, SDLK_PRINTSCREEN);
	NANX_CONSTANT(Keycode, SDLK_SCROLLLOCK);
	NANX_CONSTANT(Keycode, SDLK_PAUSE);
	NANX_CONSTANT(Keycode, SDLK_INSERT);
	NANX_CONSTANT(Keycode, SDLK_HOME);
	NANX_CONSTANT(Keycode, SDLK_PAGEUP);
	NANX_CONSTANT(Keycode, SDLK_DELETE);
	NANX_CONSTANT(Keycode, SDLK_END);
	NANX_CONSTANT(Keycode, SDLK_PAGEDOWN);
	NANX_CONSTANT(Keycode, SDLK_RIGHT);
	NANX_CONSTANT(Keycode, SDLK_LEFT);
	NANX_CONSTANT(Keycode, SDLK_DOWN);
	NANX_CONSTANT(Keycode, SDLK_UP);
	NANX_CONSTANT(Keycode, SDLK_NUMLOCKCLEAR);
	NANX_CONSTANT(Keycode, SDLK_KP_DIVIDE);
	NANX_CONSTANT(Keycode, SDLK_KP_MULTIPLY);
	NANX_CONSTANT(Keycode, SDLK_KP_MINUS);
	NANX_CONSTANT(Keycode, SDLK_KP_PLUS);
	NANX_CONSTANT(Keycode, SDLK_KP_ENTER);
	NANX_CONSTANT(Keycode, SDLK_KP_1);
	NANX_CONSTANT(Keycode, SDLK_KP_2);
	NANX_CONSTANT(Keycode, SDLK_KP_3);
	NANX_CONSTANT(Keycode, SDLK_KP_4);
	NANX_CONSTANT(Keycode, SDLK_KP_5);
	NANX_CONSTANT(Keycode, SDLK_KP_6);
	NANX_CONSTANT(Keycode, SDLK_KP_7);
	NANX_CONSTANT(Keycode, SDLK_KP_8);
	NANX_CONSTANT(Keycode, SDLK_KP_9);
	NANX_CONSTANT(Keycode, SDLK_KP_0);
	NANX_CONSTANT(Keycode, SDLK_KP_PERIOD);
	NANX_CONSTANT(Keycode, SDLK_APPLICATION);
	NANX_CONSTANT(Keycode, SDLK_POWER);
	NANX_CONSTANT(Keycode, SDLK_KP_EQUALS);
	NANX_CONSTANT(Keycode, SDLK_F13);
	NANX_CONSTANT(Keycode, SDLK_F14);
	NANX_CONSTANT(Keycode, SDLK_F15);
	NANX_CONSTANT(Keycode, SDLK_F16);
	NANX_CONSTANT(Keycode, SDLK_F17);
	NANX_CONSTANT(Keycode, SDLK_F18);
	NANX_CONSTANT(Keycode, SDLK_F19);
	NANX_CONSTANT(Keycode, SDLK_F20);
	NANX_CONSTANT(Keycode, SDLK_F21);
	NANX_CONSTANT(Keycode, SDLK_F22);
	NANX_CONSTANT(Keycode, SDLK_F23);
	NANX_CONSTANT(Keycode, SDLK_F24);
	NANX_CONSTANT(Keycode, SDLK_EXECUTE);
	NANX_CONSTANT(Keycode, SDLK_HELP);
	NANX_CONSTANT(Keycode, SDLK_MENU);
	NANX_CONSTANT(Keycode, SDLK_SELECT);
	NANX_CONSTANT(Keycode, SDLK_STOP);
	NANX_CONSTANT(Keycode, SDLK_AGAIN);
	NANX_CONSTANT(Keycode, SDLK_UNDO);
	NANX_CONSTANT(Keycode, SDLK_CUT);
	NANX_CONSTANT(Keycode, SDLK_COPY);
	NANX_CONSTANT(Keycode, SDLK_PASTE);
	NANX_CONSTANT(Keycode, SDLK_FIND);
	NANX_CONSTANT(Keycode, SDLK_MUTE);
	NANX_CONSTANT(Keycode, SDLK_VOLUMEUP);
	NANX_CONSTANT(Keycode, SDLK_VOLUMEDOWN);
	NANX_CONSTANT(Keycode, SDLK_KP_COMMA);
	NANX_CONSTANT(Keycode, SDLK_KP_EQUALSAS400);
	NANX_CONSTANT(Keycode, SDLK_ALTERASE);
	NANX_CONSTANT(Keycode, SDLK_SYSREQ);
	NANX_CONSTANT(Keycode, SDLK_CANCEL);
	NANX_CONSTANT(Keycode, SDLK_CLEAR);
	NANX_CONSTANT(Keycode, SDLK_PRIOR);
	NANX_CONSTANT(Keycode, SDLK_RETURN2);
	NANX_CONSTANT(Keycode, SDLK_SEPARATOR);
	NANX_CONSTANT(Keycode, SDLK_OUT);
	NANX_CONSTANT(Keycode, SDLK_OPER);
	NANX_CONSTANT(Keycode, SDLK_CLEARAGAIN);
	NANX_CONSTANT(Keycode, SDLK_CRSEL);
	NANX_CONSTANT(Keycode, SDLK_EXSEL);
	NANX_CONSTANT(Keycode, SDLK_KP_00);
	NANX_CONSTANT(Keycode, SDLK_KP_000);
	NANX_CONSTANT(Keycode, SDLK_THOUSANDSSEPARATOR);
	NANX_CONSTANT(Keycode, SDLK_DECIMALSEPARATOR);
	NANX_CONSTANT(Keycode, SDLK_CURRENCYUNIT);
	NANX_CONSTANT(Keycode, SDLK_CURRENCYSUBUNIT);
	NANX_CONSTANT(Keycode, SDLK_KP_LEFTPAREN);
	NANX_CONSTANT(Keycode, SDLK_KP_RIGHTPAREN);
	NANX_CONSTANT(Keycode, SDLK_KP_LEFTBRACE);
	NANX_CONSTANT(Keycode, SDLK_KP_RIGHTBRACE);
	NANX_CONSTANT(Keycode, SDLK_KP_TAB);
	NANX_CONSTANT(Keycode, SDLK_KP_BACKSPACE);
	NANX_CONSTANT(Keycode, SDLK_KP_A);
	NANX_CONSTANT(Keycode, SDLK_KP_B);
	NANX_CONSTANT(Keycode, SDLK_KP_C);
	NANX_CONSTANT(Keycode, SDLK_KP_D);
	NANX_CONSTANT(Keycode, SDLK_KP_E);
	NANX_CONSTANT(Keycode, SDLK_KP_F);
	NANX_CONSTANT(Keycode, SDLK_KP_XOR);
	NANX_CONSTANT(Keycode, SDLK_KP_POWER);
	NANX_CONSTANT(Keycode, SDLK_KP_PERCENT);
	NANX_CONSTANT(Keycode, SDLK_KP_LESS);
	NANX_CONSTANT(Keycode, SDLK_KP_GREATER);
	NANX_CONSTANT(Keycode, SDLK_KP_AMPERSAND);
	NANX_CONSTANT(Keycode, SDLK_KP_DBLAMPERSAND);
	NANX_CONSTANT(Keycode, SDLK_KP_VERTICALBAR);
	NANX_CONSTANT(Keycode, SDLK_KP_DBLVERTICALBAR);
	NANX_CONSTANT(Keycode, SDLK_KP_COLON);
	NANX_CONSTANT(Keycode, SDLK_KP_HASH);
	NANX_CONSTANT(Keycode, SDLK_KP_SPACE);
	NANX_CONSTANT(Keycode, SDLK_KP_AT);
	NANX_CONSTANT(Keycode, SDLK_KP_EXCLAM);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMSTORE);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMRECALL);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMCLEAR);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMADD);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMSUBTRACT);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMMULTIPLY);
	NANX_CONSTANT(Keycode, SDLK_KP_MEMDIVIDE);
	NANX_CONSTANT(Keycode, SDLK_KP_PLUSMINUS);
	NANX_CONSTANT(Keycode, SDLK_KP_CLEAR);
	NANX_CONSTANT(Keycode, SDLK_KP_CLEARENTRY);
	NANX_CONSTANT(Keycode, SDLK_KP_BINARY);
	NANX_CONSTANT(Keycode, SDLK_KP_OCTAL);
	NANX_CONSTANT(Keycode, SDLK_KP_DECIMAL);
	NANX_CONSTANT(Keycode, SDLK_KP_HEXADECIMAL);
	NANX_CONSTANT(Keycode, SDLK_LCTRL);
	NANX_CONSTANT(Keycode, SDLK_LSHIFT);
	NANX_CONSTANT(Keycode, SDLK_LALT);
	NANX_CONSTANT(Keycode, SDLK_LGUI);
	NANX_CONSTANT(Keycode, SDLK_RCTRL);
	NANX_CONSTANT(Keycode, SDLK_RSHIFT);
	NANX_CONSTANT(Keycode, SDLK_RALT);
	NANX_CONSTANT(Keycode, SDLK_RGUI);
	NANX_CONSTANT(Keycode, SDLK_MODE);
	NANX_CONSTANT(Keycode, SDLK_AUDIONEXT);
	NANX_CONSTANT(Keycode, SDLK_AUDIOPREV);
	NANX_CONSTANT(Keycode, SDLK_AUDIOSTOP);
	NANX_CONSTANT(Keycode, SDLK_AUDIOPLAY);
	NANX_CONSTANT(Keycode, SDLK_AUDIOMUTE);
	NANX_CONSTANT(Keycode, SDLK_MEDIASELECT);
	NANX_CONSTANT(Keycode, SDLK_WWW);
	NANX_CONSTANT(Keycode, SDLK_MAIL);
	NANX_CONSTANT(Keycode, SDLK_CALCULATOR);
	NANX_CONSTANT(Keycode, SDLK_COMPUTER);
	NANX_CONSTANT(Keycode, SDLK_AC_SEARCH);
	NANX_CONSTANT(Keycode, SDLK_AC_HOME);
	NANX_CONSTANT(Keycode, SDLK_AC_BACK);
	NANX_CONSTANT(Keycode, SDLK_AC_FORWARD);
	NANX_CONSTANT(Keycode, SDLK_AC_STOP);
	NANX_CONSTANT(Keycode, SDLK_AC_REFRESH);
	NANX_CONSTANT(Keycode, SDLK_AC_BOOKMARKS);
	NANX_CONSTANT(Keycode, SDLK_BRIGHTNESSDOWN);
	NANX_CONSTANT(Keycode, SDLK_BRIGHTNESSUP);
	NANX_CONSTANT(Keycode, SDLK_DISPLAYSWITCH);
	NANX_CONSTANT(Keycode, SDLK_KBDILLUMTOGGLE);
	NANX_CONSTANT(Keycode, SDLK_KBDILLUMDOWN);
	NANX_CONSTANT(Keycode, SDLK_KBDILLUMUP);
	NANX_CONSTANT(Keycode, SDLK_EJECT);
	NANX_CONSTANT(Keycode, SDLK_SLEEP);

	// SDL_Keymod
	v8::Local<v8::Object> Keymod = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_Keymod"), Keymod);
	NANX_CONSTANT(Keymod, KMOD_NONE);
	NANX_CONSTANT(Keymod, KMOD_LSHIFT);
	NANX_CONSTANT(Keymod, KMOD_RSHIFT);
	NANX_CONSTANT(Keymod, KMOD_LCTRL);
	NANX_CONSTANT(Keymod, KMOD_RCTRL);
	NANX_CONSTANT(Keymod, KMOD_LALT);
	NANX_CONSTANT(Keymod, KMOD_RALT);
	NANX_CONSTANT(Keymod, KMOD_LGUI);
	NANX_CONSTANT(Keymod, KMOD_RGUI);
	NANX_CONSTANT(Keymod, KMOD_NUM);
	NANX_CONSTANT(Keymod, KMOD_CAPS);
	NANX_CONSTANT(Keymod, KMOD_MODE);
	NANX_CONSTANT(Keymod, KMOD_RESERVED);
	NANX_CONSTANT(Keymod, KMOD_CTRL);
	NANX_CONSTANT(Keymod, KMOD_SHIFT);
	NANX_CONSTANT(Keymod, KMOD_ALT);
	NANX_CONSTANT(Keymod, KMOD_GUI);

	// SDL_loadso.h
	// SDL_log.h
	// SDL_main.h
	// SDL_messagebox.h
	// SDL_mouse.h

	NANX_CONSTANT(target, SDL_BUTTON_LEFT);
	NANX_CONSTANT(target, SDL_BUTTON_MIDDLE);
	NANX_CONSTANT(target, SDL_BUTTON_RIGHT);
	NANX_CONSTANT(target, SDL_BUTTON_X1);
	NANX_CONSTANT(target, SDL_BUTTON_X2);
	NANX_CONSTANT(target, SDL_BUTTON_LMASK);
	NANX_CONSTANT(target, SDL_BUTTON_MMASK);
	NANX_CONSTANT(target, SDL_BUTTON_RMASK);
	NANX_CONSTANT(target, SDL_BUTTON_X1MASK);
	NANX_CONSTANT(target, SDL_BUTTON_X2MASK);

	// SDL_mutex.h
	// SDL_name.h
	// SDL_opengl.h
	// SDL_opengles.h
	// SDL_opengles2.h
	// SDL_pixels.h

	NANX_CONSTANT(target, SDL_ALPHA_OPAQUE);
	NANX_CONSTANT(target, SDL_ALPHA_TRANSPARENT);

	NANX_CONSTANT(target, SDL_PIXELTYPE_UNKNOWN);
	NANX_CONSTANT(target, SDL_PIXELTYPE_INDEX1);
	NANX_CONSTANT(target, SDL_PIXELTYPE_INDEX4);
	NANX_CONSTANT(target, SDL_PIXELTYPE_INDEX8);
	NANX_CONSTANT(target, SDL_PIXELTYPE_PACKED8);
	NANX_CONSTANT(target, SDL_PIXELTYPE_PACKED16);
	NANX_CONSTANT(target, SDL_PIXELTYPE_PACKED32);
	NANX_CONSTANT(target, SDL_PIXELTYPE_ARRAYU8);
	NANX_CONSTANT(target, SDL_PIXELTYPE_ARRAYU16);
	NANX_CONSTANT(target, SDL_PIXELTYPE_ARRAYU32);
	NANX_CONSTANT(target, SDL_PIXELTYPE_ARRAYF16);
	NANX_CONSTANT(target, SDL_PIXELTYPE_ARRAYF32);

	NANX_CONSTANT(target, SDL_BITMAPORDER_NONE);
	NANX_CONSTANT(target, SDL_BITMAPORDER_4321);
	NANX_CONSTANT(target, SDL_BITMAPORDER_1234);

	NANX_CONSTANT(target, SDL_PACKEDORDER_NONE);
	NANX_CONSTANT(target, SDL_PACKEDORDER_XRGB);
	NANX_CONSTANT(target, SDL_PACKEDORDER_RGBX);
	NANX_CONSTANT(target, SDL_PACKEDORDER_ARGB);
	NANX_CONSTANT(target, SDL_PACKEDORDER_RGBA);
	NANX_CONSTANT(target, SDL_PACKEDORDER_XBGR);
	NANX_CONSTANT(target, SDL_PACKEDORDER_BGRX);
	NANX_CONSTANT(target, SDL_PACKEDORDER_ABGR);
	NANX_CONSTANT(target, SDL_PACKEDORDER_BGRA);

	NANX_CONSTANT(target, SDL_ARRAYORDER_NONE);
	NANX_CONSTANT(target, SDL_ARRAYORDER_RGB);
	NANX_CONSTANT(target, SDL_ARRAYORDER_RGBA);
	NANX_CONSTANT(target, SDL_ARRAYORDER_ARGB);
	NANX_CONSTANT(target, SDL_ARRAYORDER_BGR);
	NANX_CONSTANT(target, SDL_ARRAYORDER_BGRA);
	NANX_CONSTANT(target, SDL_ARRAYORDER_ABGR);

	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_NONE);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_332);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_4444);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_1555);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_5551);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_565);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_8888);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_2101010);
	NANX_CONSTANT(target, SDL_PACKEDLAYOUT_1010102);

	NANX_CONSTANT(target, SDL_PIXELFORMAT_UNKNOWN);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_INDEX1LSB);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_INDEX1MSB);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_INDEX4LSB);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_INDEX4MSB);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_INDEX8);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB332);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB444);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB555);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGR555);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ARGB4444);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGBA4444);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ABGR4444);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGRA4444);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ARGB1555);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGBA5551);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ABGR1555);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGRA5551);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB565);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGR565);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB24);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGR24);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGB888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGBX8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGR888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGRX8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ARGB8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_RGBA8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ABGR8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_BGRA8888);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_ARGB2101010);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_YV12);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_IYUV);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_YUY2);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_UYVY);
	NANX_CONSTANT(target, SDL_PIXELFORMAT_YVYU);

	NANX_EXPORT_APPLY(target, SDL_PIXELFLAG);
	NANX_EXPORT_APPLY(target, SDL_PIXELTYPE);
	NANX_EXPORT_APPLY(target, SDL_PIXELORDER);
	NANX_EXPORT_APPLY(target, SDL_PIXELLAYOUT);
	NANX_EXPORT_APPLY(target, SDL_BITSPERPIXEL);
	NANX_EXPORT_APPLY(target, SDL_BYTESPERPIXEL);
	NANX_EXPORT_APPLY(target, SDL_ISPIXELFORMAT_INDEXED);
	NANX_EXPORT_APPLY(target, SDL_ISPIXELFORMAT_ALPHA);
	NANX_EXPORT_APPLY(target, SDL_ISPIXELFORMAT_FOURCC);

	NANX_EXPORT_APPLY(target, SDL_GetPixelFormatName);
	NANX_EXPORT_APPLY(target, SDL_PixelFormatEnumToMasks);
	NANX_EXPORT_APPLY(target, SDL_MasksToPixelFormatEnum);
	NANX_EXPORT_APPLY(target, SDL_AllocFormat);
	NANX_EXPORT_APPLY(target, SDL_FreeFormat);
	NANX_EXPORT_APPLY(target, SDL_MapRGB);
	NANX_EXPORT_APPLY(target, SDL_MapRGBA);

	// SDL_platform.h

	NANX_EXPORT_APPLY(target, SDL_GetPlatform);

	// SDL_power.h

	// SDL_PowerState

	v8::Local<v8::Object> PowerState = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_PowerState"), PowerState);
	NANX_CONSTANT(PowerState, SDL_POWERSTATE_UNKNOWN);
	NANX_CONSTANT(PowerState, SDL_POWERSTATE_ON_BATTERY);
	NANX_CONSTANT(PowerState, SDL_POWERSTATE_NO_BATTERY);
	NANX_CONSTANT(PowerState, SDL_POWERSTATE_CHARGING);
	NANX_CONSTANT(PowerState, SDL_POWERSTATE_CHARGED);

	NANX_EXPORT_APPLY(target, SDL_GetPowerInfo);

	// SDL_quit.h

	NANX_EXPORT_APPLY(target, SDL_QuitRequested);

	// SDL_rect.h

	NANX_EXPORT_APPLY(target, SDL_RectEmpty);
	NANX_EXPORT_APPLY(target, SDL_RectEquals);
	// TODO: NANX_EXPORT_APPLY(target, SDL_HasIntersection);
	// TODO: NANX_EXPORT_APPLY(target, SDL_IntersectRect);
	// TODO: NANX_EXPORT_APPLY(target, SDL_UnionRect);
	// TODO: NANX_EXPORT_APPLY(target, SDL_EnclosePoints);
	// TODO: NANX_EXPORT_APPLY(target, SDL_IntersectRectAndLine);

	// SDL_render.h

	// SDL_RendererFlags
	v8::Local<v8::Object> RendererFlags = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_RendererFlags"), RendererFlags);
	NANX_CONSTANT(RendererFlags, SDL_RENDERER_SOFTWARE);
	NANX_CONSTANT(RendererFlags, SDL_RENDERER_ACCELERATED);
	NANX_CONSTANT(RendererFlags, SDL_RENDERER_PRESENTVSYNC);
	NANX_CONSTANT(RendererFlags, SDL_RENDERER_TARGETTEXTURE);

	// SDL_TextureAccess
	v8::Local<v8::Object> TextureAccess = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_TextureAccess"), TextureAccess);
	NANX_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_STATIC);
	NANX_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_STREAMING);
	NANX_CONSTANT(TextureAccess, SDL_TEXTUREACCESS_TARGET);

	// SDL_TextureModulate
	v8::Local<v8::Object> TextureModulate = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_TextureModulate"), TextureModulate);
	NANX_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_NONE);
	NANX_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_COLOR);
	NANX_CONSTANT(TextureModulate, SDL_TEXTUREMODULATE_ALPHA);

	// SDL_RendererFlip
	v8::Local<v8::Object> RendererFlip = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_RendererFlip"), RendererFlip);
	NANX_CONSTANT(RendererFlip, SDL_FLIP_NONE);
	NANX_CONSTANT(RendererFlip, SDL_FLIP_HORIZONTAL);
	NANX_CONSTANT(RendererFlip, SDL_FLIP_VERTICAL);

	// TODO: NANX_EXPORT_APPLY(target, SDL_GetNumRenderDrivers);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRenderDriverInfo);
	// TODO: NANX_EXPORT_APPLY(target, SDL_CreateWindowAndRenderer);
	NANX_EXPORT_APPLY(target, SDL_CreateRenderer);
	NANX_EXPORT_APPLY(target, SDL_CreateSoftwareRenderer);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRenderer);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRendererInfo);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRendererOutputSize);
	// TODO: NANX_EXPORT_APPLY(target, SDL_CreateTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_CreateTextureFromSurface);
	// TODO: NANX_EXPORT_APPLY(target, SDL_QueryTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_SetTextureColorMod);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetTextureColorMod);
	// TODO: NANX_EXPORT_APPLY(target, SDL_SetTextureAlphaMod);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetTextureAlphaMod);
	// TODO: NANX_EXPORT_APPLY(target, SDL_SetTextureBlendMode);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetTextureBlendMode);
	// TODO: NANX_EXPORT_APPLY(target, SDL_UpdateTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_UpdateYUVTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_LockTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_UnlockTexture);
	NANX_EXPORT_APPLY(target, SDL_RenderTargetSupported);
	// TODO: NANX_EXPORT_APPLY(target, SDL_SetRenderTarget);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRenderTarget);
	NANX_EXPORT_APPLY(target, SDL_RenderSetLogicalSize);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderGetLogicalSize);
	NANX_EXPORT_APPLY(target, SDL_RenderSetViewport);
	NANX_EXPORT_APPLY(target, SDL_RenderGetViewport);
	NANX_EXPORT_APPLY(target, SDL_RenderSetClipRect);
	NANX_EXPORT_APPLY(target, SDL_RenderGetClipRect);
	NANX_EXPORT_APPLY(target, SDL_RenderSetScale);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderGetScale);
	NANX_EXPORT_APPLY(target, SDL_SetRenderDrawColor);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRenderDrawColor);
	NANX_EXPORT_APPLY(target, SDL_SetRenderDrawBlendMode);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetRenderDrawBlendMode);
	NANX_EXPORT_APPLY(target, SDL_RenderClear);
	NANX_EXPORT_APPLY(target, SDL_RenderDrawPoint);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderDrawPoints);
	NANX_EXPORT_APPLY(target, SDL_RenderDrawLine);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderDrawLines);
	NANX_EXPORT_APPLY(target, SDL_RenderDrawRect);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderDrawRects);
	NANX_EXPORT_APPLY(target, SDL_RenderFillRect);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderFillRects);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderCopy);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderCopyEx);
	// TODO: NANX_EXPORT_APPLY(target, SDL_RenderReadPixels);
	NANX_EXPORT_APPLY(target, SDL_RenderPresent);
	// TODO: NANX_EXPORT_APPLY(target, SDL_DestroyTexture);
	NANX_EXPORT_APPLY(target, SDL_DestroyRenderer);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GL_BindTexture);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GL_UnbindTexture);

	// SDL_revision.h

	// TODO: NANX_CONSTANT_STRING(target, SDL_REVISION);
	// TODO: NANX_CONSTANT(target, SDL_REVISION_NUMBER);

	// SDL_rwops.h

	NANX_CONSTANT(target, RW_SEEK_SET);
	NANX_CONSTANT(target, RW_SEEK_CUR);
	NANX_CONSTANT(target, RW_SEEK_END);

	NANX_EXPORT_APPLY(target, SDL_AllocRW);
	NANX_EXPORT_APPLY(target, SDL_FreeRW);
	NANX_EXPORT_APPLY(target, SDL_RWFromFile);
	NANX_EXPORT_APPLY(target, SDL_RWsize);
	NANX_EXPORT_APPLY(target, SDL_RWseek);
	NANX_EXPORT_APPLY(target, SDL_RWtell);
	NANX_EXPORT_APPLY(target, SDL_RWread);
	NANX_EXPORT_APPLY(target, SDL_RWwrite);
	NANX_EXPORT_APPLY(target, SDL_RWclose);

	// SDL_scancode.h
	// SDL_shape.h
	// SDL_stdinc.h

	NANX_EXPORT_APPLY(target, SDL_getenv);
	NANX_EXPORT_APPLY(target, SDL_setenv);

	// SDL_surface.h

	NANX_EXPORT_APPLY(target, SDL_CreateRGBSurface);
	NANX_EXPORT_APPLY(target, SDL_CreateRGBSurfaceFrom);
	NANX_EXPORT_APPLY(target, SDL_FreeSurface);
	NANX_EXPORT_APPLY(target, SDL_LoadBMP);
	NANX_EXPORT_APPLY(target, SDL_SaveBMP);
	NANX_EXPORT_APPLY(target, SDL_SetSurfaceBlendMode);
	NANX_EXPORT_APPLY(target, SDL_ConvertSurfaceFormat);
	NANX_EXPORT_APPLY(target, SDL_FillRect);
	NANX_EXPORT_APPLY(target, SDL_BlitSurface);
	NANX_EXPORT_APPLY(target, SDL_SoftStretch);
	NANX_EXPORT_APPLY(target, SDL_BlitScaled);

	NANX_EXPORT_APPLY(target, SDL_GetPixel);
	NANX_EXPORT_APPLY(target, SDL_PutPixel);

	// SDL_system.h

	#if defined(__ANDROID__)

	NANX_CONSTANT(target, SDL_ANDROID_EXTERNAL_STORAGE_READ);
	NANX_CONSTANT(target, SDL_ANDROID_EXTERNAL_STORAGE_WRITE);

	NANX_EXPORT_APPLY(target, SDL_AndroidGetInternalStoragePath);
	NANX_EXPORT_APPLY(target, SDL_AndroidGetExternalStorageState);
	NANX_EXPORT_APPLY(target, SDL_AndroidGetExternalStoragePath);

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

	NANX_EXPORT_APPLY(target, SDL_GetTicks);
	NANX_EXPORT_APPLY(target, SDL_Delay);

	// SDL_touch.h

	// TODO: #define SDL_TOUCH_MOUSEID ((Uint32)-1)
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetNumTouchDevices);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetTouchDevice);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetNumTouchFingers);
	// TODO: NANX_EXPORT_APPLY(target, SDL_GetTouchFinger);

	// SDL_types.h
	// SDL_version.h

	NANX_CONSTANT(target, SDL_MAJOR_VERSION);
	NANX_CONSTANT(target, SDL_MINOR_VERSION);
	NANX_CONSTANT(target, SDL_PATCHLEVEL);

	NANX_EXPORT_APPLY(target, SDL_GetRevisionNumber);

	// SDL_video.h

	// SDL_WindowFlags
	v8::Local<v8::Object> WindowFlags = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_WindowFlags"), WindowFlags);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_FULLSCREEN);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_OPENGL);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_SHOWN);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_HIDDEN);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_BORDERLESS);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_RESIZABLE);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_MINIMIZED);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_MAXIMIZED);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_INPUT_GRABBED);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_INPUT_FOCUS);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_MOUSE_FOCUS);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_FULLSCREEN_DESKTOP);
	NANX_CONSTANT(WindowFlags, SDL_WINDOW_FOREIGN);

	NANX_CONSTANT(target, SDL_WINDOWPOS_UNDEFINED);
	NANX_CONSTANT(target, SDL_WINDOWPOS_CENTERED);

	// SDL_WindowEventID
	v8::Local<v8::Object> WindowEventID = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_WindowEventID"), WindowEventID);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_NONE);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_SHOWN);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_HIDDEN);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_EXPOSED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MOVED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_RESIZED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_SIZE_CHANGED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MINIMIZED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_MAXIMIZED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_RESTORED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_ENTER);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_LEAVE);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_FOCUS_GAINED);
	NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_FOCUS_LOST);
    NANX_CONSTANT(WindowEventID, SDL_WINDOWEVENT_CLOSE);

	// SDL_GLattr
	v8::Local<v8::Object> GLattr = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_GLattr"), GLattr);
	NANX_CONSTANT(GLattr, SDL_GL_RED_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_GREEN_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_BLUE_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_ALPHA_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_BUFFER_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_DOUBLEBUFFER);
	NANX_CONSTANT(GLattr, SDL_GL_DEPTH_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_STENCIL_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_ACCUM_RED_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_ACCUM_GREEN_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_ACCUM_BLUE_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_ACCUM_ALPHA_SIZE);
	NANX_CONSTANT(GLattr, SDL_GL_STEREO);
	NANX_CONSTANT(GLattr, SDL_GL_MULTISAMPLEBUFFERS);
	NANX_CONSTANT(GLattr, SDL_GL_MULTISAMPLESAMPLES);
	NANX_CONSTANT(GLattr, SDL_GL_ACCELERATED_VISUAL);
	NANX_CONSTANT(GLattr, SDL_GL_RETAINED_BACKING);
	NANX_CONSTANT(GLattr, SDL_GL_CONTEXT_MAJOR_VERSION);
	NANX_CONSTANT(GLattr, SDL_GL_CONTEXT_MINOR_VERSION);
	NANX_CONSTANT(GLattr, SDL_GL_CONTEXT_EGL);
	NANX_CONSTANT(GLattr, SDL_GL_CONTEXT_FLAGS);
	NANX_CONSTANT(GLattr, SDL_GL_CONTEXT_PROFILE_MASK);
	NANX_CONSTANT(GLattr, SDL_GL_SHARE_WITH_CURRENT_CONTEXT);

	// SDL_GLprofile
	v8::Local<v8::Object> GLprofile = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_GLprofile"), GLprofile);
	NANX_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_CORE);
	NANX_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	NANX_CONSTANT(GLprofile, SDL_GL_CONTEXT_PROFILE_ES);

	// SDL_GLcontextFlag
	v8::Local<v8::Object> GLcontextFlag = Nan::New<v8::Object>();
	target->Set(NANX_SYMBOL("SDL_GLcontextFlag"), GLcontextFlag);
	NANX_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_DEBUG_FLAG);
	NANX_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	NANX_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG);
	NANX_CONSTANT(GLcontextFlag, SDL_GL_CONTEXT_RESET_ISOLATION_FLAG);

	NANX_EXPORT_APPLY(target, SDL_GetNumVideoDrivers);
	NANX_EXPORT_APPLY(target, SDL_GetVideoDriver);
	NANX_EXPORT_APPLY(target, SDL_VideoInit);
	NANX_EXPORT_APPLY(target, SDL_VideoQuit);
	NANX_EXPORT_APPLY(target, SDL_GetCurrentVideoDriver);
	NANX_EXPORT_APPLY(target, SDL_GetNumVideoDisplays);
	NANX_EXPORT_APPLY(target, SDL_GetDisplayName);
	NANX_EXPORT_APPLY(target, SDL_GetDisplayBounds);
	NANX_EXPORT_APPLY(target, SDL_GetCurrentDisplayMode);
	NANX_EXPORT_APPLY(target, SDL_GetWindowDisplayIndex);
	NANX_EXPORT_APPLY(target, SDL_GetWindowPixelFormat);
	NANX_EXPORT_APPLY(target, SDL_CreateWindow);
	NANX_EXPORT_APPLY(target, SDL_GetWindowFlags);
	NANX_EXPORT_APPLY(target, SDL_SetWindowTitle);
	NANX_EXPORT_APPLY(target, SDL_GetWindowTitle);
	NANX_EXPORT_APPLY(target, SDL_SetWindowIcon);
	NANX_EXPORT_APPLY(target, SDL_SetWindowPosition);
	NANX_EXPORT_APPLY(target, SDL_GetWindowPosition);
	NANX_EXPORT_APPLY(target, SDL_SetWindowSize);
	NANX_EXPORT_APPLY(target, SDL_GetWindowSize);
	NANX_EXPORT_APPLY(target, SDL_SetWindowMinimumSize);
	NANX_EXPORT_APPLY(target, SDL_GetWindowMinimumSize);
	NANX_EXPORT_APPLY(target, SDL_SetWindowMaximumSize);
	NANX_EXPORT_APPLY(target, SDL_GetWindowMaximumSize);
	NANX_EXPORT_APPLY(target, SDL_SetWindowBordered);
	NANX_EXPORT_APPLY(target, SDL_ShowWindow);
	NANX_EXPORT_APPLY(target, SDL_HideWindow);
	NANX_EXPORT_APPLY(target, SDL_RaiseWindow);
	NANX_EXPORT_APPLY(target, SDL_MaximizeWindow);
	NANX_EXPORT_APPLY(target, SDL_MinimizeWindow);
	NANX_EXPORT_APPLY(target, SDL_RestoreWindow);
	NANX_EXPORT_APPLY(target, SDL_SetWindowFullscreen);
	NANX_EXPORT_APPLY(target, SDL_SetWindowGrab);
	NANX_EXPORT_APPLY(target, SDL_GetWindowGrab);
	NANX_EXPORT_APPLY(target, SDL_SetWindowBrightness);
	NANX_EXPORT_APPLY(target, SDL_GetWindowBrightness);
	NANX_EXPORT_APPLY(target, SDL_DestroyWindow);
	NANX_EXPORT_APPLY(target, SDL_IsScreenSaverEnabled);
	NANX_EXPORT_APPLY(target, SDL_EnableScreenSaver);
	NANX_EXPORT_APPLY(target, SDL_DisableScreenSaver);

	NANX_EXPORT_APPLY(target, SDL_GL_ExtensionSupported);
	NANX_EXPORT_APPLY(target, SDL_GL_SetAttribute);
	NANX_EXPORT_APPLY(target, SDL_GL_GetAttribute);
	NANX_EXPORT_APPLY(target, SDL_GL_CreateContext);
	NANX_EXPORT_APPLY(target, SDL_GL_MakeCurrent);
	NANX_EXPORT_APPLY(target, SDL_GL_GetCurrentWindow);
	NANX_EXPORT_APPLY(target, SDL_GL_GetCurrentContext);
	NANX_EXPORT_APPLY(target, SDL_GL_GetDrawableSize);
	NANX_EXPORT_APPLY(target, SDL_GL_SetSwapInterval);
	NANX_EXPORT_APPLY(target, SDL_GL_GetSwapInterval);
	NANX_EXPORT_APPLY(target, SDL_GL_SwapWindow);
	NANX_EXPORT_APPLY(target, SDL_GL_DeleteContext);

	NANX_EXPORT_APPLY(target, SDL_EXT_SurfaceToImageDataAsync);
	NANX_EXPORT_APPLY(target, SDL_EXT_SurfaceToImageData);
	NANX_EXPORT_APPLY(target, SDL_EXT_ImageDataToSurface);
}

} // namespace node_sdl2

NODE_MODULE(node_sdl2, node_sdl2::init)
