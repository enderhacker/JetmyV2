#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectPopupCascade(float time, float dt) {
    struct Message { const wchar_t* title; const wchar_t* body; const wchar_t* button; };
    static constexpr Message messages[] = {
        {L"File Explorer", L"The file is open in another program. Close the file and try again.", L"Retry"},
        {L"Display Settings", L"The display settings have been updated.", L"OK"},
        {L"Application Error", L"The application was unable to start correctly (0xc0000142).", L"OK"},
        {L"File Operation", L"The operation completed successfully.", L"Close"},
        {L"Network Connection", L"The network connection was interrupted. Check your connection and try again.", L"Retry"},
        {L"Printer Status", L"The printer is offline. Make sure it is turned on and connected.", L"OK"},
        {L"Notepad", L"Do you want to save changes to Untitled?", L"Cancel"},
        {L"Windows Search", L"Indexing is in progress. Search results may be incomplete.", L"OK"},
        {L"Bluetooth", L"Your device is paired and ready to use.", L"Done"},
        {L"Sound Settings", L"No audio output device was found. Check the device connection.", L"Close"},
        {L"File Explorer", L"The destination already contains a file with this name.", L"Skip"},
        {L"Extract Compressed Folder", L"The compressed folder is empty. There are no files to extract.", L"OK"},
        {L"Task Scheduler", L"The selected task has completed successfully.", L"Close"},
        {L"Device Setup", L"Device setup is complete. Your device is ready to use.", L"Done"},
        {L"Remote Desktop Connection", L"The remote computer could not be reached. Please try again later.", L"OK"},
        {L"Date and Time", L"The clock was successfully synchronized.", L"OK"},
        {L"Clipboard", L"The clipboard is currently in use by another application.", L"Retry"},
        {L"Font Viewer", L"This font is already installed.", L"Close"},
        {L"Windows Media Player", L"The file format is not supported or the required codec is unavailable.", L"Close"},
        {L"Safely Remove Hardware", L"This device is currently in use. Close any programs using the device.", L"OK"},
        {L"Folder Access", L"The folder is no longer available. Check the location and try again.", L"Retry"},
        {L"Print Queue", L"The document has been sent to the printer.", L"OK"},
        {L"Open With", L"No application is associated with this file type.", L"Close"},
        {L"Application Settings", L"Your preferences have been saved.", L"OK"}
    };
    constexpr int messageCount = int(sizeof(messages) / sizeof(messages[0]));
    struct Popup { int x = 0, y = 0, kind = 0; float life = 0; };
    static std::array<Popup, 10> popups{};
    static size_t next = 0;
    static float countdown = 0;
    static int previous = -1;
    if (time == 0) { popups = {}; next = 0; countdown = 0; previous = -1; }
    for (auto& popup : popups) popup.life = std::max(0.0f, popup.life - dt);
    int w = std::min(340, g_ScreenWidth), h = std::min(154, g_ScreenHeight);
    countdown -= dt;
    if (countdown <= 0) {
        int kind = FastRandInt(0, messageCount - (previous >= 0 ? 2 : 1));
        if (previous >= 0 && kind >= previous) ++kind;
        popups[next] = {FastRandInt(0, g_ScreenWidth - w), FastRandInt(0, g_ScreenHeight - h),
            kind, FastRandFloat(4, 8)};
        previous = kind;
        next = (next + 1) % popups.size(); countdown = FastRandFloat(0.45f, 0.95f);
    }
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SelectObject(g_hMemDC, GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, RGB(25, 29, 34));
    for (size_t i = 0; i < popups.size(); ++i) {
        const auto& popup = popups[(next + i) % popups.size()];
        if (popup.life <= 0) continue;
        int x = popup.x, y = popup.y;
        Tint(x + 5, y + 6, w, h, 0x000000, 0.35f);
        Fill(x, y, w, h, 0xD4D7DB); Fill(x + 1, y + 1, w - 2, h - 2, 0xF5F5F5);
        Fill(x + 1, y + 1, w - 2, 28, 0xE5E9EF);
        Fill(x + w - 88, y + h - 40, 72, 26, 0xDDE5F1);
        RECT title{x + 10, y + 6, x + w - 30, y + 26};
        RECT body{x + 18, y + 48, x + w - 18, y + h - 46};
        RECT button{x + w - 88, y + h - 36, x + w - 16, y + h - 15};
        const auto& message = messages[popup.kind];
        DrawTextW(g_hMemDC, message.title, -1, &title, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        DrawTextW(g_hMemDC, message.body, -1, &body, DT_WORDBREAK | DT_NOPREFIX);
        DrawTextW(g_hMemDC, message.button, -1, &button, DT_CENTER | DT_SINGLELINE);
        GdiFlush();
        Line(x + w - 20, y + 10, x + w - 12, y + 18, 0x424B58);
        Line(x + w - 12, y + 10, x + w - 20, y + 18, 0x424B58);
    }
    RestoreDC(g_hMemDC, saved);
}
} // namespace VisualPayloads
