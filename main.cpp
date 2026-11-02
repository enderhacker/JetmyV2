#include "runtime/app.h"
#include <shellapi.h>
#include <cwchar>
#include <exception>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    SetProcessDPIAware();
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args) return 1;
    bool menu = false, help = false, valid = true;
    for (int i = 1; i < count; ++i) {
        if (std::wcscmp(args[i], L"--menu") == 0) menu = true;
        else if (std::wcscmp(args[i], L"--help") == 0 || std::wcscmp(args[i], L"-h") == 0) help = true;
        else valid = false;
    }
    LocalFree(args);
    if (help || !valid) {
        MessageBoxW(nullptr, L"JetmyV2.exe\n  Start the 100-second show, followed by a 10-second simulated blue screen.\n\nJetmyV2.exe --menu\n  Open manual payload controls and the blue-screen preview.\n\nEmergency exit: Ctrl+Shift+Q or Ctrl+Shift+Esc.\nBlue-screen preview: Escape also exits early.",
            valid ? L"JetmyV2 - Usage" : L"JetmyV2 - Unknown argument", MB_OK | (valid ? MB_ICONINFORMATION : MB_ICONERROR));
        return valid ? 0 : 2;
    }
    try { return Runtime::RunApplication(instance, menu); }
    catch (const std::exception& error) {
        // Resource owners have already stopped the stream and removed the overlay.
        MessageBoxA(nullptr, error.what(), "JetmyV2", MB_OK | MB_ICONERROR);
        return 1;
    }
}
