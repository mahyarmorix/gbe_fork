// Jong: what ReFix's Photon emulator expects from the rest of ReFix: its log, and the steam_api module it
// reads Steam through (Goldberg itself, through its own flat exports).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include "jong_refix.h"
#include "refix_steam_flat.h"
#include "photon/pn_p2p.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HMODULE g_hOriginalDll = (HMODULE)&__ImageBase;

// %TEMP%\JongPhoton.log, new for each run, up to 4 MB
void ReFixLog(const char* fmt, ...) {
    static std::mutex m;
    static FILE* f = nullptr;
    static bool tried = false;
    static long size = 0;
    std::lock_guard<std::mutex> lock(m);
    if (!tried) {
        tried = true;
        char dir[MAX_PATH] = { 0 };
        if (GetTempPathA(MAX_PATH, dir)) f = fopen((std::string(dir) + "JongPhoton.log").c_str(), "w");
    }
    if (!f || size > (4 << 20)) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    size += fprintf(f, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    size += vfprintf(f, fmt, ap);
    va_end(ap);
    size += fprintf(f, "\n");
    fflush(f);
}

void jong_photon_init() { ReFixPhotonP2P::Initialize(); }

void jong_photon_service() { ReFixSteamFlat::ServiceMainThread(); }
