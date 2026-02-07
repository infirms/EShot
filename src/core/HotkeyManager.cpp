#include "HotkeyManager.h"
#include <QApplication>
#include <QDebug>

HotkeyManager& HotkeyManager::instance()
{
    static HotkeyManager inst;
    return inst;
}

HotkeyManager::HotkeyManager(QObject *parent) : QObject(parent)
{
    qApp->installNativeEventFilter(this);
    if (registerHotkey(HOTKEY_CAPTURE, 0, VK_SNAPSHOT))
        qDebug() << "[HotkeyManager] PrtSc hotkey registered";
    else
        qWarning() << "[HotkeyManager] Failed to register PrtSc. Error:" << GetLastError();
}

HotkeyManager::~HotkeyManager()
{
    unregisterAllHotkeys();
    qApp->removeNativeEventFilter(this);
}

bool HotkeyManager::registerHotkey(int id, UINT modifiers, UINT virtualKey)
{
    if (RegisterHotKey(nullptr, id, modifiers, virtualKey)) {
        m_registeredHotkeys.append(id);
        return true;
    }
    return false;
}

void HotkeyManager::unregisterHotkey(int id)
{
    UnregisterHotKey(nullptr, id);
    m_registeredHotkeys.removeAll(id);
}

void HotkeyManager::unregisterAllHotkeys()
{
    for (int id : m_registeredHotkeys) UnregisterHotKey(nullptr, id);
    m_registeredHotkeys.clear();
}

bool HotkeyManager::reRegisterCaptureHotkey(UINT modifiers, UINT virtualKey)
{
    unregisterHotkey(HOTKEY_CAPTURE);
    bool ok = registerHotkey(HOTKEY_CAPTURE, modifiers, virtualKey);
    if (!ok) registerHotkey(HOTKEY_CAPTURE, 0, VK_SNAPSHOT);
    return ok;
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            emit hotkeyTriggered(id);
            if (id == HOTKEY_CAPTURE) emit captureRequested();
            return true;
        }
    }
    return false;
}