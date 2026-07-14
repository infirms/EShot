#ifndef RECORDINGTIMELINE_H
#define RECORDINGTIMELINE_H

#include <QtGlobal>

class RecordingTimeline
{
public:
    void start(qint64 nowMs);
    bool pause(qint64 nowMs);
    bool resume(qint64 nowMs);

    qint64 activeElapsedMs(qint64 nowMs) const;
    bool isPaused() const { return m_paused; }

private:
    qint64 m_startedMs = 0;
    qint64 m_pauseStartedMs = 0;
    qint64 m_pausedMs = 0;
    bool m_started = false;
    bool m_paused = false;
};

#endif
