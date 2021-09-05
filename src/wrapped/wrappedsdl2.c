#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <dlfcn.h>
#include <stdarg.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "callback.h"
#include "librarian.h"
#include "librarian/library_private.h"
#include "emu/x86emu_private.h"
#include "box86context.h"
#include "sdl2rwops.h"
#include "myalign.h"
#include "threads.h"

#include "wrappedsdl2defs.h"

static void* my_glhandle = NULL;
// DL functions from wrappedlibdl.c
void* my_dlopen(x86emu_t* emu, void *filename, int flag);
int my_dlclose(x86emu_t* emu, void *handle);
void* my_dlsym(x86emu_t* emu, void *handle, void *symbol);

static int sdl_Yes() { return 1;}
static int sdl_No() { return 0;}
int EXPORT my2_SDL_Has3DNow() __attribute__((alias("sdl_No")));
int EXPORT my2_SDL_Has3DNowExt() __attribute__((alias("sdl_No")));
int EXPORT my2_SDL_HasAltiVec() __attribute__((alias("sdl_No")));
int EXPORT my2_SDL_HasMMX() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasMMXExt() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasNEON() __attribute__((alias("sdl_No")));   // No neon in x86 ;)
int EXPORT my2_SDL_HasRDTSC() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasSSE() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasSSE2() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasSSE3() __attribute__((alias("sdl_Yes")));
int EXPORT my2_SDL_HasSSE41() __attribute__((alias("sdl_No")));
int EXPORT my2_SDL_HasSSE42() __attribute__((alias("sdl_No")));

typedef struct {
  int32_t freq;
  uint16_t format;
  uint8_t channels;
  uint8_t silence;
  uint16_t samples;
  uint16_t padding;
  uint32_t size;
  void (*callback)(void *userdata, uint8_t *stream, int32_t len);
  void *userdata;
} SDL2_AudioSpec;

typedef struct {
    uint8_t data[16];
} SDL_JoystickGUID;

typedef union {
    SDL_JoystickGUID guid;
    uint32_t         u[4];
} SDL_JoystickGUID_Helper;

typedef struct
{
    uint32_t bindType;   // enum
    union
    {
        int button;
        int axis;
        struct {
            int hat;
            int hat_mask;
        } hat;
    } value;
} SDL_GameControllerButtonBind;

const char* sdl2Name = "libSDL2-2.0.so.0";
#define LIBNAME sdl2

typedef void    (*vFv_t)();
typedef void    (*vFiupp_t)(int32_t, uint32_t, void*, void*);
typedef int32_t (*iFpLpp_t)(void*, size_t, void*, void*);
typedef int32_t (*iFpupp_t)(void*, uint32_t, void*, void*);

#define ADDED_FUNCTIONS() \
    GO(SDL_Quit, vFv_t)           \
    GO(SDL_AllocRW, sdl2_allocrw) \
    GO(SDL_FreeRW, sdl2_freerw)   \
    GO(SDL_LogMessageV, vFiupp_t)
#include "generated/wrappedsdl2types.h"

typedef struct sdl2_my_s {
    #define GO(A, B)    B   A;
    SUPER()
    #undef GO
} sdl2_my_t;

void* getSDL2My(library_t* lib)
{
    sdl2_my_t* my = (sdl2_my_t*)calloc(1, sizeof(sdl2_my_t));
    #define GO(A, W) my->A = (W)dlsym(lib->priv.w.lib, #A);
    SUPER()
    #undef GO
    return my;
}

void freeSDL2My(void* lib)
{
    /*sdl2_my_t *my = (sdl2_my_t *)lib;*/
}
#undef SUPER

#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)   \
GO(4)

// Timer
#define GO(A)   \
static uintptr_t my_Timer_fct_##A = 0;                                      \
static uint32_t my_Timer_##A(uint32_t a, void* b)                           \
{                                                                           \
    return (uint32_t)RunFunction(my_context, my_Timer_fct_##A, 2, a, b);    \
}
SUPER()
#undef GO
static void* find_Timer_Fct(void* fct)
{
    if(!fct) return NULL;
    void* p;
    if((p = GetNativeFnc((uintptr_t)fct))) return p;
    #define GO(A) if(my_Timer_fct_##A == (uintptr_t)fct) return my_Timer_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_Timer_fct_##A == 0) {my_Timer_fct_##A = (uintptr_t)fct; return my_Timer_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for SDL2 Timer callback\n");
    return NULL;
    
}
// AudioCallback
#define GO(A)   \
static uintptr_t my_AudioCallback_fct_##A = 0;                      \
static void my_AudioCallback_##A(void* a, void* b, int c)           \
{                                                                   \
    RunFunction(my_context, my_AudioCallback_fct_##A, 3, a, b, c);  \
}
SUPER()
#undef GO
static void* find_AudioCallback_Fct(void* fct)
{
    if(!fct) return NULL;
    void* p;
    if((p = GetNativeFnc((uintptr_t)fct))) return p;
    #define GO(A) if(my_AudioCallback_fct_##A == (uintptr_t)fct) return my_AudioCallback_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_AudioCallback_fct_##A == 0) {my_AudioCallback_fct_##A = (uintptr_t)fct; return my_AudioCallback_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for SDL2 AudioCallback callback\n");
    return NULL;
    
}
// eventfilter
#define GO(A)   \
static uintptr_t my_eventfilter_fct_##A = 0;                                \
static int my_eventfilter_##A(void* userdata, void* event)                  \
{                                                                           \
    return (int)RunFunction(my_context, my_eventfilter_fct_##A, 2, userdata, event);    \
}
SUPER()
#undef GO
static void* find_eventfilter_Fct(void* fct)
{
    if(!fct) return NULL;
    void* p;
    if((p = GetNativeFnc((uintptr_t)fct))) return p;
    #define GO(A) if(my_eventfilter_fct_##A == (uintptr_t)fct) return my_eventfilter_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_eventfilter_fct_##A == 0) {my_eventfilter_fct_##A = (uintptr_t)fct; return my_eventfilter_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for SDL2 eventfilter callback\n");
    return NULL;
    
}
static void* reverse_eventfilter_Fct(void* fct)
{
    if(!fct) return fct;
    if(CheckBridged(my_context->sdl2lib->priv.w.bridge, fct))
        return (void*)CheckBridged(my_context->sdl2lib->priv.w.bridge, fct);
    #define GO(A) if(my_eventfilter_##A == fct) return (void*)my_eventfilter_fct_##A;
    SUPER()
    #undef GO
    return (void*)AddBridge(my_context->sdl2lib->priv.w.bridge, iFpp, fct, 0);
}

// LogOutput
#define GO(A)   \
static uintptr_t my_LogOutput_fct_##A = 0;                                  \
static void my_LogOutput_##A(void* a, int b, int c, void* d)                \
{                                                                           \
    RunFunction(my_context, my_LogOutput_fct_##A, 4, a, b, c, d);  \
}
SUPER()
#undef GO
static void* find_LogOutput_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_LogOutput_fct_##A == (uintptr_t)fct) return my_LogOutput_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_LogOutput_fct_##A == 0) {my_LogOutput_fct_##A = (uintptr_t)fct; return my_LogOutput_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for SDL2 LogOutput callback\n");
    return NULL;
}
static void* reverse_LogOutput_Fct(void* fct)
{
    if(!fct) return fct;
    if(CheckBridged(my_context->sdl2lib->priv.w.bridge, fct))
        return (void*)CheckBridged(my_context->sdl2lib->priv.w.bridge, fct);
    #define GO(A) if(my_LogOutput_##A == fct) return (void*)my_LogOutput_fct_##A;
    SUPER()
    #undef GO
    return (void*)AddBridge(my_context->sdl2lib->priv.w.bridge, vFpiip, fct, 0);
}

#undef SUPER

// TODO: track the memory for those callback
EXPORT int32_t my2_SDL_OpenAudio(x86emu_t* emu, void* d, void* o)
{
    SDL2_AudioSpec *desired = (SDL2_AudioSpec*)d;

    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // create a callback
    void *fnc = (void*)desired->callback;
    desired->callback = find_AudioCallback_Fct(fnc);
    int ret = my->SDL_OpenAudio(desired, (SDL2_AudioSpec*)o);
    if (ret!=0) {
        // error, clean the callback...
        desired->callback = fnc;
        return ret;
    }
    // put back stuff in place?
    desired->callback = fnc;

    return ret;
}

EXPORT int32_t my2_SDL_OpenAudioDevice(x86emu_t* emu, void* device, int32_t iscapture, void* d, void* o, int32_t allowed)
{
    SDL2_AudioSpec *desired = (SDL2_AudioSpec*)d;

    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // create a callback
    void *fnc = (void*)desired->callback;
    desired->callback = find_AudioCallback_Fct(fnc);
    int ret = my->SDL_OpenAudioDevice(device, iscapture, desired, (SDL2_AudioSpec*)o, allowed);
    if (ret<=0) {
        // error, clean the callback...
        desired->callback = fnc;
        return ret;
    }
    // put back stuff in place?
    desired->callback = fnc;

    return ret;
}

EXPORT void *my2_SDL_LoadFile_RW(x86emu_t* emu, void* a, void* b, int c)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    void* r = my->SDL_LoadFile_RW(rw, b, c);
    if(c==0)
        RWNativeEnd2(rw);
    return r;
}
EXPORT void *my2_SDL_LoadBMP_RW(x86emu_t* emu, void* a, int b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    void* r = my->SDL_LoadBMP_RW(rw, b);
    if(b==0)
        RWNativeEnd2(rw);
    return r;
}
EXPORT int32_t my2_SDL_SaveBMP_RW(x86emu_t* emu, void* a, void* b, int c)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    int32_t r = my->SDL_SaveBMP_RW(rw, b, c);
    if(c==0)
        RWNativeEnd2(rw);
    return r;
}
EXPORT void *my2_SDL_LoadWAV_RW(x86emu_t* emu, void* a, int b, void* c, void* d, void* e)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    void* r = my->SDL_LoadWAV_RW(rw, b, c, d, e);
    if(b==0)
        RWNativeEnd2(rw);
    return r;
}
EXPORT int32_t my2_SDL_GameControllerAddMappingsFromRW(x86emu_t* emu, void* a, int b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    int32_t r = my->SDL_GameControllerAddMappingsFromRW(rw, b);
    if(b==0)
        RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_ReadU8(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_ReadU8(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_ReadBE16(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_ReadBE16(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_ReadBE32(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_ReadBE32(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint64_t my2_SDL_ReadBE64(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint64_t r = my->SDL_ReadBE64(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_ReadLE16(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_ReadLE16(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_ReadLE32(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_ReadLE32(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint64_t my2_SDL_ReadLE64(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint64_t r = my->SDL_ReadLE64(rw);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteU8(x86emu_t* emu, void* a, uint8_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteU8(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteBE16(x86emu_t* emu, void* a, uint16_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteBE16(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteBE32(x86emu_t* emu, void* a, uint32_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteBE32(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteBE64(x86emu_t* emu, void* a, uint64_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteBE64(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteLE16(x86emu_t* emu, void* a, uint16_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteLE16(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteLE32(x86emu_t* emu, void* a, uint32_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteLE32(rw, v);
    RWNativeEnd2(rw);
    return r;
}
EXPORT uint32_t my2_SDL_WriteLE64(x86emu_t* emu, void* a, uint64_t v)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t r = my->SDL_WriteLE64(rw, v);
    RWNativeEnd2(rw);
    return r;
}

EXPORT void *my2_SDL_RWFromConstMem(x86emu_t* emu, void* a, int b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    void* r = my->SDL_RWFromConstMem(a, b);
    return AddNativeRW2(emu, (SDL2_RWops_t*)r);
}
EXPORT void *my2_SDL_RWFromFP(x86emu_t* emu, void* a, int b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    void* r = my->SDL_RWFromFP(a, b);
    return AddNativeRW2(emu, (SDL2_RWops_t*)r);
}
EXPORT void *my2_SDL_RWFromFile(x86emu_t* emu, void* a, void* b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    void* r = my->SDL_RWFromFile(a, b);
    return AddNativeRW2(emu, (SDL2_RWops_t*)r);
}
EXPORT void *my2_SDL_RWFromMem(x86emu_t* emu, void* a, int b)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    void* r = my->SDL_RWFromMem(a, b);
    return AddNativeRW2(emu, (SDL2_RWops_t*)r);
}

EXPORT int64_t my2_SDL_RWseek(x86emu_t* emu, void* a, int64_t offset, int32_t whence)
{
    //sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    int64_t ret = RWNativeSeek2(rw, offset, whence);
    RWNativeEnd2(rw);
    return ret;
}
EXPORT int64_t my2_SDL_RWtell(x86emu_t* emu, void* a)
{
    //sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    int64_t ret = RWNativeSeek2(rw, 0, 1);  //1 == RW_SEEK_CUR
    RWNativeEnd2(rw);
    return ret;
}
EXPORT uint32_t my2_SDL_RWread(x86emu_t* emu, void* a, void* ptr, uint32_t size, uint32_t maxnum)
{
    //sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t ret = RWNativeRead2(rw, ptr, size, maxnum);
    RWNativeEnd2(rw);
    return ret;
}
EXPORT uint32_t my2_SDL_RWwrite(x86emu_t* emu, void* a, const void* ptr, uint32_t size, uint32_t maxnum)
{
    //sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t ret = RWNativeWrite2(rw, ptr, size, maxnum);
    RWNativeEnd2(rw);
    return ret;
}
EXPORT int my2_SDL_RWclose(x86emu_t* emu, void* a)
{
    //sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    return RWNativeClose2(rw);
}

EXPORT int my2_SDL_SaveAllDollarTemplates(x86emu_t* emu, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t ret = my->SDL_SaveAllDollarTemplates(rw);
    RWNativeEnd2(rw);
    return ret;
}

EXPORT int my2_SDL_SaveDollarTemplate(x86emu_t* emu, int gesture, void* a)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    SDL2_RWops_t *rw = RWNativeStart2(emu, (SDL2_RWops_t*)a);
    uint32_t ret = my->SDL_SaveDollarTemplate(gesture, rw);
    RWNativeEnd2(rw);
    return ret;
}

EXPORT void *my2_SDL_AddTimer(x86emu_t* emu, uint32_t a, void* f, void* p)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    return my->SDL_AddTimer(a, find_Timer_Fct(f), p);
}

EXPORT int my2_SDL_RemoveTimer(x86emu_t* emu, void *t)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    return my->SDL_RemoveTimer(t);
}

EXPORT void my2_SDL_SetEventFilter(x86emu_t* emu, void* p, void* userdata)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    my->SDL_SetEventFilter(find_eventfilter_Fct(p), userdata);
}
EXPORT int my2_SDL_GetEventFilter(x86emu_t* emu, void** f, void* userdata)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    int ret = my->SDL_GetEventFilter(f, userdata);
    *f = reverse_eventfilter_Fct(*f);
    return ret;
}

EXPORT void my2_SDL_LogGetOutputFunction(x86emu_t* emu, void** f, void* arg)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    my->SDL_LogGetOutputFunction(f, arg);
    if(*f) *f = reverse_LogOutput_Fct(*f);
}
EXPORT void my2_SDL_LogSetOutputFunction(x86emu_t* emu, void* f, void* arg)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    my->SDL_LogSetOutputFunction(find_LogOutput_Fct(f), arg);
}

EXPORT int my2_SDL_vsnprintf(x86emu_t* emu, void* buff, uint32_t s, void * fmt, void * b) {
    #ifndef NOALIGN
    // need to align on arm
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    void* f = vsnprintf;
    return ((iFpLpp_t)f)(buff, s, fmt, VARARGS);
    #else
    return vsnprintf(buff, s, fmt, b);
    #endif
}

EXPORT void* my2_SDL_CreateThread(x86emu_t* emu, void* f, void* n, void* p)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    void* et = NULL;
    return my->SDL_CreateThread(my_prepare_thread(emu, f, p, 0, &et), n, et);
}

EXPORT int my2_SDL_snprintf(x86emu_t* emu, void* buff, uint32_t s, void * fmt, void * b) {
    #ifndef NOALIGN
    // need to align on arm
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    void* f = vsnprintf;
    return ((iFpLpp_t)f)(buff, s, fmt, VARARGS);
    #else
    return vsnprintf((char*)buff, s, (char*)fmt, b);
    #endif
}

static int get_sdl_priv(x86emu_t* emu, const char *sym_str, void **w, void **f)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    #define GO(sym, _w) \
    if (strcmp(#sym, sym_str) == 0) \
    { \
        *w = _w; \
        *f = dlsym(emu->context->box86lib, "my2_"#sym); \
        if (*f == NULL) \
            *f = dlsym(emu->context->box86lib, "my_"#sym); \
        if (*f == NULL) \
            *f = dlsym(emu->context->sdl2lib->priv.w.lib, #sym); \
        return *f != NULL; \
    }
    #define GOM(sym, _w, ...) GO(sym, _w)
    #define GO2(sym, _w, ...) GO(sym, _w)
    #define GOS(sym, _w, ...) GO(sym, _w)
    #define DATA
    if(0);
    #include "wrappedsdl2_private.h"

    #undef GO
    #undef GOM
    #undef GO2
    #undef GOS
    #undef DATA
    return 0;
}

int EXPORT my2_SDL_DYNAPI_entry(x86emu_t* emu, uint32_t version, uintptr_t *table, uint32_t tablesize)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    int i = 0;
    uintptr_t start, end;
    uintptr_t tab[tablesize];
    int r = my->SDL_DYNAPI_entry(version, tab, tablesize);

    #define SDL_DYNAPI_PROC(ret, sym, args, parms, ...) \
        if (i < tablesize) { \
            void *w = NULL; \
            void *f = NULL; \
            if (get_sdl_priv(emu, #sym, &w, &f)) { \
                table[i] = AddCheckBridge(my_context->sdl2lib->priv.w.bridge, w, f, 0); \
            } \
            else \
                table[i] = (uintptr_t)NULL; \
            printf_log(LOG_DEBUG, "SDL_DYNAPI_entry: %s => %p (%p)\n", #sym, (void*)table[i], f); \
            i++; \
        }

    #include "SDL_dynapi_procs.h"
    return 0;
}

EXPORT void *my2_SDL_CreateWindow(x86emu_t* emu, const char *title, int x, int y, int w, int h, uint32_t flags)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    void *win = NULL;

    // Set ES 3.1
    my->SDL_GL_SetAttribute(21, 4);
    my->SDL_GL_SetAttribute(17, 3); 
    my->SDL_GL_SetAttribute(18, 1); 

    win = my->SDL_CreateWindow(title, 0, 0, -1, -1, flags | 2);
    printf("my2_SDL_CreateWindow: %s %d %d %d %d %04X\n", title, x, y, w, h, flags | 2);
    return win;
}

char EXPORT *my2_SDL_GetBasePath(x86emu_t* emu) {
    char* p = strdup(emu->context->fullpath);
    char* b = strrchr(p, '/');
    if(b)
        *(b+1) = '\0';
    return p;
}

EXPORT void my2_SDL_LogCritical(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_CRITICAL == 6
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 6, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 6, fmt, b);
    #endif
}

EXPORT void my2_SDL_LogError(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_ERROR == 5
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 5, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 5, fmt, b);
    #endif
}

EXPORT void my2_SDL_LogWarn(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_WARN == 4
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 4, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 4, fmt, b);
    #endif
}

EXPORT void my2_SDL_LogInfo(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_INFO == 3
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 3, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 3, fmt, b);
    #endif
}

EXPORT void my2_SDL_LogDebug(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_DEBUG == 2
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 2, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 2, fmt, b);
    #endif
}

EXPORT void my2_SDL_LogVerbose(x86emu_t* emu, int32_t cat, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_VERBOSE == 1
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(cat, 1, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(cat, 1, fmt, b);
    #endif
}

EXPORT void my2_SDL_Log(x86emu_t* emu, void* fmt, void *b) {
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // SDL_LOG_PRIORITY_INFO == 3
    // SDL_LOG_CATEGORY_APPLICATION == 0
    #ifndef NOALIGN
    myStackAlign((const char*)fmt, b, emu->scratch);
    PREPARE_VALIST;
    my->SDL_LogMessageV(0, 3, fmt, VARARGS);
    #else
    my->SDL_LogMessageV(0, 3, fmt, b);
    #endif
}

EXPORT int my2_SDL_GL_SetAttribute(x86emu_t* emu, uint32_t attr, int value) 
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;    
    return my->SDL_GL_SetAttribute(attr, value);
}

// Cut down and adapted from SDL_video.c
EXPORT int my2_SDL_GL_ExtensionSupported(x86emu_t* emu, char *name) 
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    const char *(*glGetString_f)(uint32_t) = dlsym(my_glhandle, "glGetString");
    const char *extensions;
    const char *start;
    const char *where, *terminator;

    printf("SDL_GL_ExtensionSupported: %s?\n", name);

    /* Extension names should not have spaces. */
    where = strchr(name, ' ');
    if (where || *name == '\0') {
        return 0;
    }

    /* See if there's an environment variable override */
    start = getenv(name);
    if (start && *start == '0') {
        return 0;
    }

    if (!glGetString_f) {
        return 0;
    }

    #define GL_EXTENSIONS                     0x1F03
    extensions = (const char *) glGetString_f(GL_EXTENSIONS);
    if (!extensions) {
        return 0;
    }

    /*
     * It takes a bit of care to be fool-proof about parsing the OpenGL
     * extensions string. Don't be fooled by sub-strings, etc.
     */

    start = extensions;

    for (;;) {
        where = strstr(start, name);
        if (!where)
            break;

        terminator = where + strlen(name);
        if (where == extensions || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return 1;

        start = terminator;
    }
    
    return my->SDL_GL_ExtensionSupported(name);
}

EXPORT void* my_glXGetProcAddress(x86emu_t* emu, void* name);
void fillGLProcWrapper(box86context_t*);
extern char* libGL;
EXPORT void* my2_SDL_GL_GetProcAddress(x86emu_t* emu, void* name) 
{
    khint_t k;
    const char* rname = (const char*)name;
    if(dlsym_error && box86_log<LOG_DEBUG) printf_log(LOG_NONE, "Calling SDL_GL_GetProcAddress(\"%s\") => ", rname);
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    // check if glxprocaddress is filled, and search for lib and fill it if needed
    if(!emu->context->glxprocaddress)
        emu->context->glxprocaddress = (procaddess_t)my->SDL_GL_GetProcAddress;
    if(!emu->context->glwrappers) {
        fillGLProcWrapper(emu->context);
        // check if libGL is loaded, load it if not (helps DeadCells)
        if(!my_glhandle && !GetLibInternal(libGL?libGL:"libGL.so.1")) {
            // use a my_dlopen to actually open that lib, like SDL2 is doing...
            my_glhandle = my_dlopen(emu, libGL?libGL:"libGL.so.1", RTLD_LAZY|RTLD_GLOBAL);
        }
    }
    // get proc adress using actual glXGetProcAddress
    k = kh_get(symbolmap, emu->context->glmymap, rname);
    int is_my = (k==kh_end(emu->context->glmymap))?0:1;
    void* symbol;
    if(is_my) {
        // try again, by using custom "my_" now...
        char tmp[200];
        strcpy(tmp, "my_");
        strcat(tmp, rname);
        symbol = dlsym(emu->context->box86lib, tmp);
    } else {
        char buf[200] = {};
            symbol = my_glXGetProcAddress(emu, name);
            // Attempt first to load from glxGetProcAddress (e.g., would come from gl4es)
            if (symbol)
                return symbol;
            // Couldn't find any, try from SDL now
            else
                symbol = my->SDL_GL_GetProcAddress(name);
    }
    if(!symbol) {
        if(dlsym_error && box86_log<LOG_DEBUG) printf_log(LOG_NONE, "%p\n", NULL);
        return NULL;    // easy
    }
    // check if alread bridged
    uintptr_t ret = CheckBridged(emu->context->system, symbol);
    if(ret) {
        if(dlsym_error && box86_log<LOG_DEBUG) printf_log(LOG_NONE, "%p\n", (void*)ret);
        return (void*)ret; // already bridged
    }
    // get wrapper    
    k = kh_get(symbolmap, emu->context->glwrappers, rname);
    if(k==kh_end(emu->context->glwrappers) && strstr(rname, "ARB")==NULL) {
        // try again, adding ARB at the end if not present
        char tmp[200];
        strcpy(tmp, rname);
        strcat(tmp, "ARB");
        k = kh_get(symbolmap, emu->context->glwrappers, tmp);
    }
    if(k==kh_end(emu->context->glwrappers) && strstr(rname, "EXT")==NULL) {
        // try again, adding EXT at the end if not present
        char tmp[200];
        strcpy(tmp, rname);
        strcat(tmp, "EXT");
        k = kh_get(symbolmap, emu->context->glwrappers, tmp);
    }
    if(k==kh_end(emu->context->glwrappers)) {
        if(dlsym_error && box86_log<LOG_DEBUG) printf_log(LOG_NONE, "%p\n", NULL);
        if(dlsym_error && box86_log<LOG_INFO) printf_log(LOG_NONE, "Warning, no wrapper for %s\n", rname);
        return NULL;
    }
    AddOffsetSymbol(emu->context->maplib, symbol, rname);
    ret  = AddBridge(emu->context->system, kh_value(emu->context->glwrappers, k), symbol, 0);
    if(dlsym_error && box86_log<LOG_DEBUG) printf_log(LOG_NONE, "%p\n", (void*)ret);
    return (void*)ret;
}

#define nb_once	16
typedef void(*sdl2_tls_dtor)(void*);
static uintptr_t dtor_emu[nb_once] = {0};
static void tls_dtor_callback(int n, void* a)
{
	if(dtor_emu[n]) {
        RunFunction(my_context, dtor_emu[n], 1, a);
	}
}
#define GO(N) \
void tls_dtor_callback_##N(void* a) \
{ \
	tls_dtor_callback(N, a); \
}

GO(0)
GO(1)
GO(2)
GO(3)
GO(4)
GO(5)
GO(6)
GO(7)
GO(8)
GO(9)
GO(10)
GO(11)
GO(12)
GO(13)
GO(14)
GO(15)
#undef GO
static const sdl2_tls_dtor dtor_cb[nb_once] = {
	 tls_dtor_callback_0, tls_dtor_callback_1, tls_dtor_callback_2, tls_dtor_callback_3
	,tls_dtor_callback_4, tls_dtor_callback_5, tls_dtor_callback_6, tls_dtor_callback_7
	,tls_dtor_callback_8, tls_dtor_callback_9, tls_dtor_callback_10,tls_dtor_callback_11
	,tls_dtor_callback_12,tls_dtor_callback_13,tls_dtor_callback_14,tls_dtor_callback_15
};
EXPORT int32_t my2_SDL_TLSSet(x86emu_t* emu, uint32_t id, void* value, void* dtor)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

	if(!dtor)
		return my->SDL_TLSSet(id, value, NULL);
	int n = 0;
	while (n<nb_once) {
		if(!dtor_emu[n] || (dtor_emu[n])==((uintptr_t)dtor)) {
			dtor_emu[n] = (uintptr_t)dtor;
			return my->SDL_TLSSet(id, value, dtor_cb[n]);
		}
		++n;
	}
	printf_log(LOG_NONE, "Error: SDL2 SDL_TLSSet with destructor: no more slot!\n");
	//emu->quit = 1;
	return -1;
}

EXPORT void* my2_SDL_JoystickGetDeviceGUID(x86emu_t* emu, void* p, int32_t idx)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    *(SDL_JoystickGUID*)p = my->SDL_JoystickGetDeviceGUID(idx);
    return p;
}

EXPORT void* my2_SDL_JoystickGetGUID(x86emu_t* emu, void* p, void* joystick)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    *(SDL_JoystickGUID*)p = my->SDL_JoystickGetGUID(joystick);
    return p;
}

EXPORT void* my2_SDL_JoystickGetGUIDFromString(x86emu_t* emu, void* p, void* pchGUID)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    *(SDL_JoystickGUID*)p = my->SDL_JoystickGetGUIDFromString(pchGUID);
    return p;
}

EXPORT void* my2_SDL_GameControllerGetBindForAxis(x86emu_t* emu, void* p, void* controller, int32_t axis)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    *(SDL_GameControllerButtonBind*)p = my->SDL_GameControllerGetBindForAxis(controller, axis);
    return p;
}

EXPORT void* my2_SDL_GameControllerGetBindForButton(x86emu_t* emu, void* p, void* controller, int32_t button)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    *(SDL_GameControllerButtonBind*)p = my->SDL_GameControllerGetBindForButton(controller, button);
    return p;
}

EXPORT void my2_SDL_AddEventWatch(x86emu_t* emu, void* p, void* userdata)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    my->SDL_AddEventWatch(find_eventfilter_Fct(p), userdata);
}
EXPORT void my2_SDL_DelEventWatch(x86emu_t* emu, void* p, void* userdata)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    my->SDL_DelEventWatch(find_eventfilter_Fct(p), userdata);
}

EXPORT void* my2_SDL_LoadObject(x86emu_t* emu, void* sofile)
{
    return my_dlopen(emu, sofile, 0);   // TODO: check correct flag value...
}
EXPORT void my2_SDL_UnloadObject(x86emu_t* emu, void* handle)
{
    my_dlclose(emu, handle);
}
EXPORT void* my2_SDL_LoadFunction(x86emu_t* emu, void* handle, void* name)
{
    return my_dlsym(emu, handle, name);
}

EXPORT void my2_SDL_GetJoystickGUIDInfo(x86emu_t* emu, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint16_t* vendor, uint16_t* product, uint16_t* version)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_GetJoystickGUIDInfo) {
        SDL_JoystickGUID_Helper guid;
        guid.u[0] = a;
        guid.u[1] = b;
        guid.u[2] = c;
        guid.u[3] = d;
        my->SDL_GetJoystickGUIDInfo(guid.guid, vendor, product, version);
    } else {
        // dummy, set everything to "unknown"
        if(vendor)  *vendor = 0;
        if(product) *product = 0;
        if(version) *version = 0;
    }
}

EXPORT int32_t my2_SDL_IsJoystickPS4(x86emu_t* emu, uint16_t vendor, uint16_t product_id)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickPS4)
        return my->SDL_IsJoystickPS4(vendor, product_id);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickNintendoSwitchPro(x86emu_t* emu, uint16_t vendor, uint16_t product_id)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickNintendoSwitchPro)
        return my->SDL_IsJoystickNintendoSwitchPro(vendor, product_id);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickSteamController(x86emu_t* emu, uint16_t vendor, uint16_t product_id)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickSteamController)
        return my->SDL_IsJoystickSteamController(vendor, product_id);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickXbox360(x86emu_t* emu, uint16_t vendor, uint16_t product_id)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickXbox360)
        return my->SDL_IsJoystickXbox360(vendor, product_id);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickXboxOne(x86emu_t* emu, uint16_t vendor, uint16_t product_id)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickXboxOne)
        return my->SDL_IsJoystickXboxOne(vendor, product_id);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickXInput(x86emu_t* emu, SDL_JoystickGUID p)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickXInput)
        return my->SDL_IsJoystickXInput(p);
    // fallback
    return 0;
}
EXPORT int32_t my2_SDL_IsJoystickHIDAPI(x86emu_t* emu, SDL_JoystickGUID p)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;

    if(my->SDL_IsJoystickHIDAPI)
        return my->SDL_IsJoystickHIDAPI(p);
    // fallback
    return 0;
}

void* my_vkGetInstanceProcAddr(x86emu_t* emu, void* device, void* name);
EXPORT void* my2_SDL_Vulkan_GetVkGetInstanceProcAddr(x86emu_t* emu)
{
    sdl2_my_t *my = (sdl2_my_t *)emu->context->sdl2lib->priv.w.p2;
    
    if(!emu->context->vkprocaddress)
        emu->context->vkprocaddress = (vkprocaddess_t)my->SDL_Vulkan_GetVkGetInstanceProcAddr();

    if(emu->context->vkprocaddress)
        return (void*)AddCheckBridge(my_context->sdl2lib->priv.w.bridge, pFEpp, my_vkGetInstanceProcAddr, 0);
    return NULL;
}

#define CUSTOM_INIT \
    box86->sdl2lib = lib;                                           \
    lib->priv.w.p2 = getSDL2My(lib);                                \
    box86->sdl2allocrw = ((sdl2_my_t*)lib->priv.w.p2)->SDL_AllocRW; \
    box86->sdl2freerw  = ((sdl2_my_t*)lib->priv.w.p2)->SDL_FreeRW;  \
    lib->altmy = strdup("my2_");                                    \
    lib->priv.w.needed = 4;                                         \
    lib->priv.w.neededlibs = (char**)calloc(lib->priv.w.needed, sizeof(char*)); \
    lib->priv.w.neededlibs[0] = strdup("libdl.so.2");               \
    lib->priv.w.neededlibs[1] = strdup("libm.so.6");                \
    lib->priv.w.neededlibs[2] = strdup("librt.so.1");               \
    lib->priv.w.neededlibs[3] = strdup("libpthread.so.0");

#define CUSTOM_FINI \
    ((sdl2_my_t *)lib->priv.w.p2)->SDL_Quit();                  \
    if(my_glhandle) my_dlclose(thread_get_emu(), my_glhandle);  \
    my_glhandle = NULL;                                         \
    freeSDL2My(lib->priv.w.p2);                                 \
    free(lib->priv.w.p2);                                       \
    ((box86context_t*)(lib->context))->sdl2lib = NULL;          \
    ((box86context_t*)(lib->context))->sdl2allocrw = NULL;      \
    ((box86context_t*)(lib->context))->sdl2freerw = NULL;


#include "wrappedlib_init.h"
