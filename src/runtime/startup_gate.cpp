#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include "app.h"
#include "startup_gate.h"
#include <commctrl.h>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Runtime {
namespace {
struct ConfirmationState {
    ConfirmationDelay delay;
    bool accepted = false;
};

HRESULT CALLBACK ConfirmationCallback(HWND window, UINT notification, WPARAM wParam, LPARAM, LONG_PTR data) {
    auto& state = *reinterpret_cast<ConfirmationState*>(data);
    if (notification == TDN_DIALOG_CONSTRUCTED) {
        // Disable before the dialog is displayed, not after its first timer tick.
        SendMessageW(window, TDM_ENABLE_BUTTON, IDYES, FALSE);
    } else if (notification == TDN_CREATED) {
        state.delay.Start(GetTickCount64());
        SendMessageW(window, TDM_ENABLE_BUTTON, IDYES, FALSE);
    } else if (notification == TDN_TIMER) {
        const uint64_t now = GetTickCount64();
        SendMessageW(window, TDM_ENABLE_BUTTON, IDYES, state.delay.Ready(now));
    } else if (notification == TDN_BUTTON_CLICKED) {
        // Enforce the delay even for keyboard activation or a synthetic command.
        if (wParam == IDYES && !state.delay.Ready(GetTickCount64())) return S_FALSE;
        state.accepted = wParam == IDYES;
    }
    return S_OK;
}

bool Confirm(int step, bool menuMode, void*) {
    std::wstring message;
    if (step == 1) {
        message = L"This file is malicious. This is meant to harm your system irreversibly and permanently destroy all your data.\n"
            L"Only click 'Yes' if you are running this in a virtual machine and understand the consequences of running virus/malware in your device.";
    } else {
        message = L"You've clicked 'Yes' to proceed with the malicious software. Confirm again if you want to run malicious code in your system that will cause irreversible damage to your computer and data.\nClick 'yes' to execute the malicious code or 'no' to exit the program safely.";
    }

    ConfirmationState state;
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.dwFlags = TDF_CALLBACK_TIMER | TDF_ALLOW_DIALOG_CANCELLATION;
    dialog.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    dialog.nDefaultButton = IDNO;
    dialog.pszWindowTitle = L"JetmyV2";
    dialog.pszMainIcon = TD_WARNING_ICON;
    dialog.pszContent = message.c_str();
    dialog.pfCallback = ConfirmationCallback;
    dialog.lpCallbackData = reinterpret_cast<LONG_PTR>(&state);
    dialog.cxWidth = 260;

    // Resolve the v6 common-control API under the executable's manifest. There is
    // deliberately no fallback that could skip the enforced wait on API failure.
    const HMODULE controls = LoadLibraryW(L"comctl32.dll");
    if (!controls) throw std::runtime_error("Cannot load startup confirmation controls.");
    using TaskDialogFunction = HRESULT (WINAPI *)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);
    const FARPROC address = GetProcAddress(controls, "TaskDialogIndirect");
    TaskDialogFunction taskDialog = nullptr;
    static_assert(sizeof(taskDialog) == sizeof(address), "Windows function pointer sizes must match.");
    std::memcpy(&taskDialog, &address, sizeof(taskDialog));
    if (!taskDialog) {
        FreeLibrary(controls);
        throw std::runtime_error("Cannot create timed confirmations. Windows common controls v6 are required.");
    }
    int button = 0;
    const HRESULT result = taskDialog(&dialog, &button, nullptr, nullptr);
    FreeLibrary(controls);
    if (FAILED(result)) throw std::runtime_error("Cannot display the startup confirmation. No effects were started.");
    return button == IDYES && state.accepted;
}
}

StartupResult EvaluateStartup(bool menuMode, StartupConfirmation confirmation, void* context) {
    if (!confirmation || !confirmation(1, menuMode, context)) return StartupResult::Declined;
    if (!confirmation(2, menuMode, context)) return StartupResult::Declined;
    return StartupResult::Allowed;
}

StartupResult CheckStartup(bool menuMode) {
    return EvaluateStartup(menuMode, Confirm, nullptr);
}
}
