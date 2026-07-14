#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QList>
#include <QPair>
#include "PlatformHotkey.h"

class LinuxPortalGlobalShortcuts;
class LinuxKGlobalAccelShortcuts;

class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    static HotkeyManager& instance();
    bool registerHotkey(int id, UINT modifiers, UINT virtualKey);
    void unregisterHotkey(int id);
    void unregisterAllHotkeys();
    bool reRegisterCaptureHotkey(UINT modifiers, UINT virtualKey);
    bool reRegisterRecordingHotkeys(UINT pauseModifiers, UINT pauseVirtualKey,
                                    UINT stopModifiers, UINT stopVirtualKey,
                                    UINT cancelModifiers, UINT cancelVirtualKey);
    bool reRegisterActionHotkeys(UINT instantModifiers, UINT instantVirtualKey,
                                 UINT gifModifiers, UINT gifVirtualKey,
                                 UINT videoModifiers, UINT videoVirtualKey);
    bool requestLinuxPortalShortcutRebind();
    UINT captureModifiers() const { return m_captureModifiers; }
    UINT captureVirtualKey() const { return m_captureVirtualKey; }
    QString recordingPauseShortcutText() const;
    QString recordingStopShortcutText() const;
    QString recordingCancelShortcutText() const;
    static bool isPlainPrintScreen(UINT modifiers, UINT virtualKey);
    static bool isWindowsPrintScreenSnippingEnabled();
    static bool setWindowsPrintScreenSnippingEnabled(bool enabled);
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void hotkeyTriggered(int id);
    void captureRequested();
    void recordingPauseRequested();
    void recordingStopRequested();
    void recordingCancelRequested();
    void instantCaptureRequested();
    void gifCaptureRequested();
    void videoCaptureRequested();

private:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager();
    HotkeyManager(const HotkeyManager&) = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    QList<int> m_registeredHotkeys;
    QHash<int, QPair<UINT, UINT>> m_registeredHotkeyDefs;
    UINT m_captureModifiers = 0;
    UINT m_captureVirtualKey = VK_SNAPSHOT;
    UINT m_recordingPauseModifiers = MOD_CONTROL | MOD_ALT;
    UINT m_recordingPauseVirtualKey = 'P';
    UINT m_recordingStopModifiers = MOD_CONTROL | MOD_ALT;
    UINT m_recordingStopVirtualKey = 'S';
    UINT m_recordingCancelModifiers = MOD_CONTROL | MOD_ALT;
    UINT m_recordingCancelVirtualKey = 'X';
    UINT m_instantCaptureModifiers = 0;
    UINT m_instantCaptureVirtualKey = 0;
    UINT m_gifCaptureModifiers = 0;
    UINT m_gifCaptureVirtualKey = 0;
    UINT m_videoCaptureModifiers = 0;
    UINT m_videoCaptureVirtualKey = 0;
    void *m_x11Display = nullptr;
    unsigned long m_x11RootWindow = 0;
    LinuxPortalGlobalShortcuts *m_portalShortcuts = nullptr;
    LinuxKGlobalAccelShortcuts *m_kdeShortcuts = nullptr;
    bool m_usePortalShortcuts = false;

    void emitHotkey(int id);
    void refreshPortalShortcuts();

public:
    static constexpr int HOTKEY_CAPTURE = 1;
    static constexpr int HOTKEY_RECORDING_PAUSE = 2;
    static constexpr int HOTKEY_RECORDING_STOP = 3;
    static constexpr int HOTKEY_RECORDING_CANCEL = 4;
    static constexpr int HOTKEY_INSTANT_CAPTURE = 5;
    static constexpr int HOTKEY_GIF_CAPTURE = 6;
    static constexpr int HOTKEY_VIDEO_CAPTURE = 7;
};

#endif
