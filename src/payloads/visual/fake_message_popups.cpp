#include "visual.h"
#include "popup_style.h"
#include <array>

namespace VisualPayloads {
void EffectPopupCascade(float time, float dt) {
    struct Message { const wchar_t* title; const wchar_t* body; const wchar_t* button; int icon = 1; };
    static constexpr Message messages[] = {
        {L"File Explorer", L"The file is open in another program. Close the file and try again.", L"Retry", 0},
        {L"Display Settings", L"The display settings have been updated.", L"OK"},
        {L"Application Error", L"The application was unable to start correctly (0xc0000142).", L"OK", 2},
        {L"File Operation", L"The operation completed successfully.", L"Close"},
        {L"Network Connection", L"The network connection was interrupted. Check your connection and try again.", L"Retry", 0},
        {L"Printer Status", L"The printer is offline. Make sure it is turned on and connected.", L"OK", 0},
        {L"Notepad", L"Do you want to save changes to Untitled?", L"Cancel"},
        {L"Windows Search", L"Indexing is in progress. Search results may be incomplete.", L"OK"},
        {L"Bluetooth", L"Your device is paired and ready to use.", L"Done"},
        {L"Sound Settings", L"No audio output device was found. Check the device connection.", L"Close", 0},
        {L"File Explorer", L"The destination already contains a file with this name.", L"Skip", 0},
        {L"Extract Compressed Folder", L"The compressed folder is empty. There are no files to extract.", L"OK"},
        {L"Task Scheduler", L"The selected task has completed successfully.", L"Close"},
        {L"Device Setup", L"Device setup is complete. Your device is ready to use.", L"Done"},
        {L"Remote Desktop Connection", L"The remote computer could not be reached. Please try again later.", L"OK", 2},
        {L"Date and Time", L"The clock was successfully synchronized.", L"OK"},
        {L"Clipboard", L"The clipboard is currently in use by another application.", L"Retry", 0},
        {L"Font Viewer", L"This font is already installed.", L"Close"},
        {L"Windows Media Player", L"The file format is not supported or the required codec is unavailable.", L"Close", 2},
        {L"Safely Remove Hardware", L"This device is currently in use. Close any programs using the device.", L"OK", 0},
        {L"Folder Access", L"The folder is no longer available. Check the location and try again.", L"Retry", 0},
        {L"Print Queue", L"The document has been sent to the printer.", L"OK"},
        {L"Open With", L"No application is associated with this file type.", L"Close"},
        {L"Application Settings", L"Your preferences have been saved.", L"OK"}
    };
    constexpr int messageCount = int(sizeof(messages) / sizeof(messages[0]));
    struct Popup { int x = 0, y = 0, w = 0, h = 0, kind = 0; float life = 0; };
    static std::array<Popup, 10> popups{};
    static size_t next = 0;
    static float countdown = 0;
    static int previous = -1;
    if (time == 0) { popups = {}; next = 0; countdown = 0; previous = -1; }
    for (auto& popup : popups) popup.life = std::max(0.0f, popup.life - dt);
    countdown -= dt;
    if (countdown <= 0) {
        int kind = FastRandInt(0, messageCount - (previous >= 0 ? 2 : 1));
        if (previous >= 0 && kind >= previous) ++kind;
        SIZE size = PopupDimensions(messages[kind].body);
        popups[next] = {FastRandInt(0, g_ScreenWidth - size.cx), FastRandInt(0, g_ScreenHeight - size.cy),
            int(size.cx), int(size.cy), kind, FastRandFloat(4, 8)};
        previous = kind;
        next = (next + 1) % popups.size(); countdown = FastRandFloat(0.45f, 0.95f);
    }
    for (size_t i = 0; i < popups.size(); ++i) {
        const auto& popup = popups[(next + i) % popups.size()];
        if (popup.life <= 0) continue;
        const auto& message = messages[popup.kind];
        DrawPopup(popup.x, popup.y, popup.w, popup.h, message.title, message.body, message.button, message.icon);
    }
}
} // namespace VisualPayloads
