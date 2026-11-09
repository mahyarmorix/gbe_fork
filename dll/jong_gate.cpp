// Jong: a Jong library game runs only through Jong, as Steam's games through Steam. Its first Steam call asks Jong over
// its pipe (\\.\pipe\JongOverlay, gbepipe.js) whether Jong started it: JONG_LAUNCH, the token Jong puts in the
// environment of what it starts (a game's own child, a UE game's -Shipping.exe, inherits it).
//  - started by Jong: it runs; the pipe stays open, and when Jong is closed the game is closed too, with a message
//  - not started by Jong, Jong running: Jong starts it itself (with the member's name and picture) and this copy quits;
//    a game Jong doesn't have in its library: a message, and it quits
//  - Jong not running: Jong is started with jong://play?exe=… (it then starts the game) and this copy quits
//  - Jong not installed: a message with a button that downloads it, and it quits
// A Jong from before this (no answer to "gate") lets the game run.
#include "dll/dll.h"

#if defined(__WINDOWS__)

#include <shellapi.h>
#include <cctype>
#include <thread>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")

namespace {

constexpr const wchar_t *PIPE = L"\\\\.\\pipe\\JongOverlay";
constexpr const wchar_t *DOWNLOAD = L"https://thejong.top/app";  // the site's newest Windows installer

std::string json_str(const std::string &s)
{
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\';
        if ((unsigned char)c >= 0x20) o += c;
    }
    return o;
}

std::wstring url_part(const std::string &s)  // percent-encoded (UTF-8)
{
    static const char *hex = "0123456789ABCDEF";
    std::wstring o;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (wchar_t)c;
        else { o += L'%'; o += (wchar_t)hex[c >> 4]; o += (wchar_t)hex[c & 15]; }
    }
    return o;
}

// one overlapped read / write on h within ms (INFINITE: till the pipe breaks) -> bytes, or -1
long io(HANDLE h, bool rd, char *buf, DWORD n, DWORD ms)
{
    OVERLAPPED ov{};
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ov.hEvent) return -1;
    DWORD got = 0;
    long r = -1;
    BOOL ok = rd ? ReadFile(h, buf, n, NULL, &ov) : WriteFile(h, buf, n, NULL, &ov);
    if (ok || GetLastError() == ERROR_IO_PENDING) {
        if (WaitForSingleObject(ov.hEvent, ms) != WAIT_OBJECT_0) CancelIoEx(h, &ov);
        if (GetOverlappedResult(h, &ov, &got, TRUE)) r = (long)got;
    }
    CloseHandle(ov.hEvent);
    return r;
}

// Jong's program, from the jong:// link it registers (Windows asks it to open those links)
std::wstring jong_exe()
{
    wchar_t v[2048];
    DWORD n = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\jong\\shell\\open\\command", NULL, RRF_RT_REG_SZ, NULL, v, &n) != ERROR_SUCCESS) return L"";
    std::wstring c(v);
    size_t a = !c.empty() && c[0] == L'"' ? 1 : 0, b = c.find(a ? L'"' : L' ', a);
    std::wstring e = c.substr(a, b == std::wstring::npos ? std::wstring::npos : b - a);
    return !e.empty() && GetFileAttributesW(e.c_str()) != INVALID_FILE_ATTRIBUTES ? e : L"";
}

BOOL CALLBACK hide_one(HWND w, LPARAM)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(w)) ShowWindow(w, SW_HIDE);
    return TRUE;
}

HHOOK cbt = NULL;
LRESULT CALLBACK name_buttons(int code, WPARAM w, LPARAM l)  // MessageBox's OK / Cancel as «Download Jong» / «Close»
{
    if (code == HCBT_ACTIVATE) {
        SetDlgItemTextW((HWND)w, IDOK, L"Download Jong");
        SetDlgItemTextW((HWND)w, IDCANCEL, L"Close");
        UnhookWindowsHookEx(cbt);
        cbt = NULL;
    }
    return CallNextHookEx(NULL, code, w, l);
}

[[noreturn]] void quit(const wchar_t *text)
{
    if (text) {
        EnumWindows(hide_one, 0);  // a full-screen game would cover the message
        MessageBoxW(NULL, text, L"Jong", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    }
    ExitProcess(0);
}

void watch(HANDLE h)  // started by Jong: closed with it
{
    char b[256];
    while (io(h, true, b, sizeof(b), INFINITE) > 0) {}
    quit(L"Jong was closed, so this game was closed too.\n\nOpen Jong and start the game from it.");
}

void gate()
{
    wchar_t ex[32768];
    DWORD en = GetModuleFileNameW(NULL, ex, 32768);
    std::string exe = utf8_encode(std::wstring(ex, en));
    char tok[128] = "";
    GetEnvironmentVariableA("JONG_LAUNCH", tok, sizeof(tok));

    HANDLE h = CreateFileW(PIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PIPE_BUSY && WaitNamedPipeW(PIPE, 3000))
        h = CreateFileW(PIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (h != INVALID_HANDLE_VALUE) {  // Jong is running
        std::string q = "{\"op\":\"gate\",\"pid\":" + std::to_string(GetCurrentProcessId()) + ",\"tok\":\"" + json_str(tok) + "\",\"exe\":\"" + json_str(exe) + "\"}\n";
        std::string a{};
        if (io(h, false, q.data(), (DWORD)q.size(), 3000) == (long)q.size()) {
            char b[512];
            DWORD t0 = GetTickCount();
            while (a.find('\n') == std::string::npos && GetTickCount() - t0 < 5000) {
                long n = io(h, true, b, sizeof(b), 5000 - (GetTickCount() - t0));
                if (n <= 0) break;
                a.append(b, n);
            }
        }
        PRINT_DEBUG("Jong gate: %s", a.c_str());
        if (a.empty() || a.find("\"ok\":true") != std::string::npos) {  // ours, or a Jong that doesn't answer this
            std::thread(watch, h).detach();
            return;
        }
        CloseHandle(h);
        if (a.find("\"play\":true") != std::string::npos) quit(nullptr);  // Jong starts it itself
        quit(L"Start this game from Jong.\n\nIt isn't in Jong's library on this PC: install it from the library in Jong.");
    }

    std::wstring j = jong_exe();
    if (!j.empty()) {
        std::wstring arg = L"\"jong://play?exe=" + url_part(exe) + L"\"";
        ShellExecuteW(NULL, L"open", j.c_str(), arg.c_str(), NULL, SW_SHOWNORMAL);
        quit(nullptr);
    }

    cbt = SetWindowsHookExW(WH_CBT, name_buttons, NULL, GetCurrentThreadId());
    int r = MessageBoxW(NULL, L"Jong is not installed.\n\nThis game runs through the Jong launcher. Download Jong, install it, then start the game from Jong.",
                        L"Jong", MB_OKCANCEL | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    if (cbt) { UnhookWindowsHookEx(cbt); cbt = NULL; }
    if (r == IDOK) ShellExecuteW(NULL, L"open", DOWNLOAD, NULL, NULL, SW_SHOWNORMAL);
    quit(nullptr);
}

}

void jong_gate()
{
    static std::once_flag once{};
    std::call_once(once, gate);
}

#else // Jong is Windows only

void jong_gate() {}

#endif
