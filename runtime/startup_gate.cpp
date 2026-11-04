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
        const unsigned remaining = state.delay.RemainingSeconds(now);
        const wchar_t* footer = remaining >= 3 ? L"Please read the warning. Yes unlocks in 3 seconds."
            : remaining == 2 ? L"Please read the warning. Yes unlocks in 2 seconds."
            : remaining == 1 ? L"Please read the warning. Yes unlocks in 1 second."
            : L"You may now select Yes to continue. No or Escape exits.";
        SendMessageW(window, TDM_SET_ELEMENT_TEXT, TDE_FOOTER, reinterpret_cast<LPARAM>(footer));
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
        message = L"WARNING: Flashing visuals and harsh electronic audio.\n\n"
            L"Do not continue if you have photosensitivity or if these effects could put anyone at risk. "
            L"Save your work, lower your audio volume, and make sure you have the computer user's permission.\n\n"
            L"This simulator uses temporary windows and does not intentionally damage Windows or files.\n\n"
            L"Have you read and accepted these warnings?";
    } else {
        message = menuMode
            ? L"Open the manual control panel? All effects and audio start OFF. You can enable them individually.\n\n"
            : L"Start the automatic show NOW? Flashing visuals and audio will begin after approval, "
              L"followed by a simulated blue-screen finale.\n\n";
        message += L"Exit: Ctrl+Shift+Q or Ctrl+Shift+Esc.\n"
            L"During the finale, Escape also exits.\n\n"
            L"Select Yes only if you want to proceed.";
    }

    ConfirmationState state;
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.dwFlags = TDF_CALLBACK_TIMER | TDF_ALLOW_DIALOG_CANCELLATION;
    dialog.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    dialog.nDefaultButton = IDNO;
    dialog.pszWindowTitle = step == 1 ? L"JetmyV2 - Safety confirmation (1 of 2)"
        : L"JetmyV2 - Final confirmation (2 of 2)";
    dialog.pszMainIcon = TD_WARNING_ICON;
    dialog.pszMainInstruction = step == 1 ? L"Read before continuing" : L"Are you ready to continue?";
    dialog.pszContent = message.c_str();
    dialog.pszFooter = L"Please read the warning. Yes unlocks in 3 seconds.";
    dialog.pfCallback = ConfirmationCallback;
    dialog.lpCallbackData = reinterpret_cast<LONG_PTR>(&state);
    dialog.cxWidth = 360;

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
