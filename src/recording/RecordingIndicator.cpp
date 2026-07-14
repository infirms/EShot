#include "RecordingIndicator.h"

#include "../core/TranslationManager.h"

#include <QAction>
#include <QEnterEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QRegion>
#include <QScreen>
#include <QStyle>
#include <QToolButton>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
#endif

namespace {
constexpr int ControlHeight = 42;
constexpr int PreferredControlWidth = 324;
constexpr int CompactControlWidth = 214;
constexpr int WindowPadding = 6;

QString controlBarStyle()
{
    return QStringLiteral(R"(
        QFrame#recordingControlBar {
            background-color: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 10px;
        }
        QLabel#recordingStatusLabel {
            color: #F4F6F8;
            background: transparent;
            border: none;
            font-size: 12px;
            font-weight: 600;
        }
        QToolButton {
            background-color: #3a3a3a;
            border: 1px solid #505050;
            border-radius: 8px;
            padding: 0px;
        }
        QToolButton:hover {
            background-color: #4a4a4a;
            border-color: #606060;
        }
        QToolButton:pressed {
            background-color: #333333;
            border-color: #505050;
        }
        QPushButton#recordingStopButton {
            color: white;
            background-color: #E5484D;
            border: 1px solid #F05B60;
            border-radius: 7px;
            padding: 0 9px;
            font-size: 11px;
            font-weight: 700;
        }
        QPushButton#recordingStopButton:hover {
            background-color: #F05257;
            border-color: #FF7377;
        }
        QPushButton#recordingStopButton:pressed {
            background-color: #C93B40;
            border-color: #DD4A4F;
        }
        QToolButton#recordingCancelButton {
            background-color: #3a3a3a;
            border: 1px solid #505050;
        }
        QMenu {
            color: #F4F6F8;
            background-color: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 8px;
            padding: 5px;
        }
        QMenu::item {
            padding: 6px 14px;
            border-radius: 5px;
        }
        QMenu::item:disabled {
            color: #d6d6d6;
        }
    )");
}
}

RecordingIndicator::RecordingIndicator(const QRect &captureRect, QWidget *parent,
                                       int borderWidth, bool supportsPause,
                                       RecordingIndicatorMode mode)
    : QWidget(parent),
      m_captureRect(captureRect),
      m_mode(mode),
      m_supportsPause(supportsPause),
      m_borderWidth(qMax(1, borderWidth))
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint
                   | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setProperty("recordingBorderWidth", m_borderWidth);

    QRect screenRect = QGuiApplication::primaryScreen()
        ? QGuiApplication::primaryScreen()->geometry()
        : captureRect;
    if (QScreen *screen = QGuiApplication::screenAt(captureRect.center()))
        screenRect = screen->geometry();

    m_layout = recordingControlLayout(captureRect, screenRect,
                                      QSize(PreferredControlWidth, ControlHeight),
                                      QSize(CompactControlWidth, ControlHeight));
    m_compact = m_layout.compact;
#ifdef Q_OS_WIN
    constexpr bool platformCanExcludeOverlay = true;
#else
    constexpr bool platformCanExcludeOverlay = false;
#endif
    m_visibilityPolicy = recordingOverlayVisibilityPolicy(
        m_layout.placement, platformCanExcludeOverlay);
    setProperty("captureSafeInside", requiresCaptureSafePresentation());

    const QRect globalWindowRect = captureRect.adjusted(-WindowPadding, -WindowPadding,
                                                        WindowPadding, WindowPadding)
        .united(m_layout.controlRect.adjusted(-WindowPadding, -WindowPadding,
                                              WindowPadding, WindowPadding));
    setGeometry(globalWindowRect);
    m_borderRect = captureRect.translated(-globalWindowRect.topLeft());
    m_controlRect = m_layout.controlRect.translated(-globalWindowRect.topLeft());

    createControlBar();
    updateControlMask();
    updateStatusLabel();
    updatePauseButton();

    show();
    if (QGuiApplication::platformName() != QStringLiteral("offscreen"))
        raise();
#ifdef Q_OS_WIN
    SetWindowDisplayAffinity(reinterpret_cast<HWND>(winId()), WDA_EXCLUDEFROMCAPTURE);
#endif
}

RecordingIndicator::~RecordingIndicator() = default;

void RecordingIndicator::createControlBar()
{
    m_controlBar = new QFrame(this);
    m_controlBar->setObjectName(QStringLiteral("recordingControlBar"));
    m_controlBar->setAttribute(Qt::WA_StyledBackground, true);
    m_controlBar->setGeometry(m_controlRect);
    m_controlBar->setStyleSheet(controlBarStyle());

    auto *layout = new QHBoxLayout(m_controlBar);
    layout->setContentsMargins(m_compact ? 6 : 8, 5, m_compact ? 6 : 8, 5);
    layout->setSpacing(4);

    m_statusLabel = new QLabel(m_controlBar);
    m_statusLabel->setObjectName(QStringLiteral("recordingStatusLabel"));
    m_statusLabel->setMinimumWidth(m_compact ? 58 : 92);
    m_statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    layout->addWidget(m_statusLabel, 1);

    m_pauseButton = new QToolButton(m_controlBar);
    m_pauseButton->setObjectName(QStringLiteral("recordingPauseButton"));
    m_pauseButton->setFixedSize(30, 30);
    m_pauseButton->setIconSize(QSize(18, 18));
    m_pauseButton->setCursor(Qt::PointingHandCursor);
    m_pauseButton->setToolTip(TranslationManager::recordingPauseResume());
    m_pauseButton->setVisible(m_supportsPause);
    connect(m_pauseButton, &QToolButton::clicked, this, [this]() {
        if (m_temporaryPause) {
            m_temporaryPause = false;
            updatePauseButton();
            return;
        }
        if (m_paused) {
            if (requiresCaptureSafePresentation()) {
                setOverlayVisible(false);
                QTimer::singleShot(120, this, [this]() { emit resumeRequested(); });
            } else {
                emit resumeRequested();
            }
        } else {
            emit pauseRequested();
        }
    });
    layout->addWidget(m_pauseButton);

    m_stopButton = new QPushButton(m_controlBar);
    m_stopButton->setObjectName(QStringLiteral("recordingStopButton"));
    m_stopButton->setFixedHeight(30);
    m_stopButton->setMinimumWidth(m_compact ? 30 : 64);
    m_stopButton->setMaximumWidth(m_compact ? 30 : 72);
    m_stopButton->setIcon(QIcon(QStringLiteral(":/icons/stop.svg")));
    m_stopButton->setIconSize(QSize(16, 16));
    m_stopButton->setText(m_compact ? QString() : TranslationManager::recordingStopShort());
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setToolTip(TranslationManager::recordingStop());
    connect(m_stopButton, &QPushButton::clicked, this, &RecordingIndicator::stopRequested);
    layout->addWidget(m_stopButton);

    m_cancelButton = new QToolButton(m_controlBar);
    m_cancelButton->setObjectName(QStringLiteral("recordingCancelButton"));
    m_cancelButton->setFixedSize(30, 30);
    m_cancelButton->setIcon(QIcon(QStringLiteral(":/icons/close.svg")));
    m_cancelButton->setIconSize(QSize(17, 17));
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setToolTip(TranslationManager::recordingCancel());
    connect(m_cancelButton, &QToolButton::clicked, this, &RecordingIndicator::cancelRequested);
    layout->addWidget(m_cancelButton);

    m_detailsMenu = new QMenu(m_controlBar);
    m_detailsButton = new QToolButton(m_controlBar);
    m_detailsButton->setObjectName(QStringLiteral("recordingDetailsButton"));
    m_detailsButton->setFixedSize(30, 30);
    m_detailsButton->setIcon(QIcon(QStringLiteral(":/icons/more.svg")));
    m_detailsButton->setIconSize(QSize(18, 18));
    m_detailsButton->setCursor(Qt::PointingHandCursor);
    m_detailsButton->setToolTip(TranslationManager::recordingDetails());
    m_detailsButton->setPopupMode(QToolButton::InstantPopup);
    m_detailsButton->setMenu(m_detailsMenu);
    layout->addWidget(m_detailsButton);

    setDetails({QStringLiteral("%1 × %2").arg(m_captureRect.width()).arg(m_captureRect.height())});
}

void RecordingIndicator::setFrameCount(int count)
{
    m_frameCount = qMax(0, count);
    updateStatusLabel();
}

void RecordingIndicator::setRemainingSeconds(int seconds)
{
    m_remainingSeconds = seconds;
    updateStatusLabel();
}

void RecordingIndicator::setElapsedSeconds(int seconds)
{
    m_elapsedSeconds = qMax(0, seconds);
    updateStatusLabel();
}

void RecordingIndicator::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    if (requiresCaptureSafePresentation() && !m_temporaryPause)
        setOverlayVisible(paused);
    updateStatusLabel();
    updatePauseButton();
}

void RecordingIndicator::setDetails(const QStringList &details)
{
    if (!m_detailsMenu)
        return;
    m_detailsMenu->clear();
    for (const QString &detail : details) {
        QAction *action = m_detailsMenu->addAction(detail);
        action->setEnabled(false);
    }
    m_detailsButton->setVisible(!details.isEmpty());
}

void RecordingIndicator::stop()
{
    m_running = false;
    close();
}

bool RecordingIndicator::controlsInside() const
{
    return m_layout.placement == RecordingControlPlacement::Inside;
}

bool RecordingIndicator::requiresCaptureSafePresentation() const
{
    return m_visibilityPolicy == RecordingOverlayVisibility::RevealWhilePaused;
}

void RecordingIndicator::startCaptureSafePresentation()
{
    if (!requiresCaptureSafePresentation() || m_captureSafePresentationStarted)
        return;
    m_captureSafePresentationStarted = true;
    m_temporaryPause = true;
    setOverlayVisible(false);
    emit pauseRequested();
    QTimer::singleShot(80, this, [this]() {
        if (!m_temporaryPause)
            return;
        if (!m_paused) {
            m_temporaryPause = false;
            return;
        }
        setOverlayVisible(true);
        QTimer::singleShot(1100, this, &RecordingIndicator::hideAndResumeTemporaryReveal);
    });
}

void RecordingIndicator::updateStatusLabel()
{
    if (!m_statusLabel)
        return;
    const int minutes = m_elapsedSeconds / 60;
    const int seconds = m_elapsedSeconds % 60;
    const QString time = QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
    const QString mode = m_mode == RecordingIndicatorMode::Video
        ? QStringLiteral("VIDEO") : QStringLiteral("GIF");
    const bool visiblyPaused = m_paused && !m_temporaryPause;
    const QString dotColor = visiblyPaused ? QStringLiteral("#A3A9B3") : QStringLiteral("#FF525D");
    const QString text = m_compact
        ? QStringLiteral("<span style='color:%1'>●</span>&nbsp; %2").arg(dotColor, time)
        : QStringLiteral("<span style='color:%1'>●</span>&nbsp; %2&nbsp;&nbsp;%3")
              .arg(dotColor, mode, time);
    m_statusLabel->setText(text);
}

void RecordingIndicator::updatePauseButton()
{
    if (!m_pauseButton)
        return;
    const bool visiblyPaused = m_paused && !m_temporaryPause;
    m_pauseButton->setIcon(QIcon(visiblyPaused
        ? QStringLiteral(":/icons/resume.svg")
        : QStringLiteral(":/icons/pause.svg")));
    m_pauseButton->setProperty("recordingState", visiblyPaused ? QStringLiteral("resume")
                                                                : QStringLiteral("pause"));
    m_pauseButton->style()->unpolish(m_pauseButton);
    m_pauseButton->style()->polish(m_pauseButton);
}

void RecordingIndicator::updateControlMask()
{
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    QRegion region;
    if (!m_overlayVisible && requiresCaptureSafePresentation()) {
        region += m_controlRect;
        setMask(region);
        return;
    }
    const int hitWidth = qMax(6, m_borderWidth + 4);
    region += QRect(m_borderRect.left() - 2, m_borderRect.top() - 2,
                    m_borderRect.width() + 4, hitWidth);
    region += QRect(m_borderRect.left() - 2, m_borderRect.bottom() - hitWidth + 3,
                    m_borderRect.width() + 4, hitWidth);
    region += QRect(m_borderRect.left() - 2, m_borderRect.top() - 2,
                    hitWidth, m_borderRect.height() + 4);
    region += QRect(m_borderRect.right() - hitWidth + 3, m_borderRect.top() - 2,
                    hitWidth, m_borderRect.height() + 4);
    region += m_controlRect.adjusted(-3, -3, 3, 3);
    setMask(region);
}

void RecordingIndicator::setOverlayVisible(bool visible)
{
    if (m_overlayVisible == visible)
        return;
    m_overlayVisible = visible;
    if (m_controlBar)
        m_controlBar->setVisible(visible);
    updateControlMask();
    update();
}

void RecordingIndicator::beginTemporaryReveal()
{
    if (!requiresCaptureSafePresentation() || m_overlayVisible || m_temporaryPause)
        return;
    m_temporaryPause = true;
    emit pauseRequested();
    QTimer::singleShot(80, this, [this]() {
        if (m_temporaryPause)
            setOverlayVisible(true);
    });
}

void RecordingIndicator::hideAndResumeTemporaryReveal()
{
    if (!m_temporaryPause)
        return;
    if ((m_controlBar && m_controlBar->underMouse())
        || (m_detailsMenu && m_detailsMenu->isVisible())) {
        QTimer::singleShot(300, this, &RecordingIndicator::hideAndResumeTemporaryReveal);
        return;
    }
    setOverlayVisible(false);
    QTimer::singleShot(120, this, [this]() {
        if (!m_temporaryPause)
            return;
        m_temporaryPause = false;
        emit resumeRequested();
        updatePauseButton();
        updateStatusLabel();
    });
}

void RecordingIndicator::enterEvent(QEnterEvent *event)
{
    beginTemporaryReveal();
    QWidget::enterEvent(event);
}

void RecordingIndicator::leaveEvent(QEvent *event)
{
    if (requiresCaptureSafePresentation() && m_temporaryPause)
        QTimer::singleShot(250, this, &RecordingIndicator::hideAndResumeTemporaryReveal);
    QWidget::leaveEvent(event);
}

void RecordingIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!m_overlayVisible)
        return;
    QPainter painter(this);
    const QColor accent = m_mode == RecordingIndicatorMode::Video
        ? QColor(63, 156, 255, 235)
        : QColor(255, 76, 91, 235);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawRect(QRect(m_borderRect.left(), m_borderRect.top() - m_borderWidth,
                           m_borderRect.width(), m_borderWidth));
    painter.drawRect(QRect(m_borderRect.left(), m_borderRect.bottom() + 1,
                           m_borderRect.width(), m_borderWidth));
    painter.drawRect(QRect(m_borderRect.left() - m_borderWidth, m_borderRect.top(),
                           m_borderWidth, m_borderRect.height()));
    painter.drawRect(QRect(m_borderRect.right() + 1, m_borderRect.top(),
                           m_borderWidth, m_borderRect.height()));
}
