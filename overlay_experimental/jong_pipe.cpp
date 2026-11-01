#include "overlay/jong_pipe.h"

#include <deque>
#include <mutex>

#if defined(_WIN32)

#include <thread>
#include <windows.h>

namespace jong_pipe {

static std::mutex mtx{};
static HANDLE pipe = INVALID_HANDLE_VALUE;
static std::deque<std::string> lines{};

static bool write_locked(HANDLE h, const std::string &s)
{
    OVERLAPPED ov{};
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ov.hEvent) return false;
    DWORD n = 0;
    bool ok = WriteFile(h, s.data(), (DWORD)s.size(), NULL, &ov) || GetLastError() == ERROR_IO_PENDING;
    if (ok) {
        if (WaitForSingleObject(ov.hEvent, 1000) != WAIT_OBJECT_0) CancelIoEx(h, &ov);  // never hold the game up
        ok = GetOverlappedResult(h, &ov, &n, TRUE) && n == s.size();  // (waits for the cancel too: ov is on this stack)
    }
    CloseHandle(ov.hEvent);
    return ok;
}

static void push(std::string s)
{
    std::lock_guard lock(mtx);
    if (lines.size() < 64) lines.push_back(std::move(s));
}

static void run()
{
    for (;;) {
        HANDLE h = CreateFileW(L"\\\\.\\pipe\\JongOverlay", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (h == INVALID_HANDLE_VALUE) { Sleep(3000); continue; }
        {
            std::lock_guard lock(mtx);
            pipe = h;
            write_locked(h, "{\"op\":\"hello\",\"pid\":" + std::to_string(GetCurrentProcessId()) + "}\n");
        }

        OVERLAPPED ov{};
        ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        std::string acc{};
        char buf[1024];
        while (ov.hEvent) {
            DWORD n = 0;
            ResetEvent(ov.hEvent);
            if (!ReadFile(h, buf, sizeof(buf), NULL, &ov) && GetLastError() != ERROR_IO_PENDING) break;
            if (!GetOverlappedResult(h, &ov, &n, TRUE) || !n) break;
            acc.append(buf, n);
            for (size_t i; (i = acc.find('\n')) != std::string::npos; acc.erase(0, i + 1)) push(acc.substr(0, i));
            if (acc.size() > 65536) break;
        }
        if (ov.hEvent) CloseHandle(ov.hEvent);

        {
            std::lock_guard lock(mtx);
            pipe = INVALID_HANDLE_VALUE;
        }
        CloseHandle(h);
        push("{\"ev\":\"closed\"}");  // Jong went away: the game isn't left thinking the overlay is up
        Sleep(3000);
    }
}

void start()
{
    static std::once_flag once{};
    std::call_once(once, [] { std::thread(run).detach(); });
}

bool send(const std::string &line)
{
    std::lock_guard lock(mtx);
    return pipe != INVALID_HANDLE_VALUE && write_locked(pipe, line + "\n");
}

bool poll(std::string &line)
{
    std::lock_guard lock(mtx);
    if (lines.empty()) return false;
    line = std::move(lines.front());
    lines.pop_front();
    return true;
}

}

#else // Jong is Windows only

namespace jong_pipe {
void start() {}
bool send(const std::string &) { return false; }
bool poll(std::string &) { return false; }
}

#endif
