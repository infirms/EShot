#include "RecordingTimeline.h"

#include <QtGlobal>

void RecordingTimeline::start(qint64 nowMs)
{
    m_startedMs = nowMs;
    m_pauseStartedMs = 0;
    m_pausedMs = 0;
    m_started = true;
    m_paused = false;
}

bool RecordingTimeline::pause(qint64 nowMs)
{
    if (!m_started || m_paused)
        return false;
    m_pauseStartedMs = qMax(nowMs, m_startedMs);
    m_paused = true;
    return true;
}

bool RecordingTimeline::resume(qint64 nowMs)
{
    if (!m_started || !m_paused)
        return false;
    m_pausedMs += qMax<qint64>(0, nowMs - m_pauseStartedMs);
    m_pauseStartedMs = 0;
    m_paused = false;
    return true;
}

qint64 RecordingTimeline::activeElapsedMs(qint64 nowMs) const
{
    if (!m_started)
        return 0;
    qint64 pausedMs = m_pausedMs;
    if (m_paused)
        pausedMs += qMax<qint64>(0, nowMs - m_pauseStartedMs);
    return qMax<qint64>(0, nowMs - m_startedMs - pausedMs);
}
