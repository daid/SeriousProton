#ifndef SP2_TIMER_H
#define SP2_TIMER_H

#include <chrono>

namespace sp {

template<typename ClockSource> class TimerBase
{
public:
    void start(float timeout)
    {
        running = true;
        auto_restart = false;
        start_time = ClockSource::now();
        this->timeout = timeout;
    }

    void repeat(float timeout)
    {
        running = true;
        auto_restart = true;
        start_time = ClockSource::now();
        this->timeout = timeout;
    }

    void stop()
    {
        running = false;
    }

    bool isRunning() const
    {
        return running;
    }

    //Call this to check if the timer has expired. This will return true only once if the timer expired.
    bool isExpired()
    {
        if (!running)
            return false;
        bool expired = getTimeElapsed() > timeout;
        if (expired)
        {
            running = auto_restart;
            start_time += std::chrono::duration_cast<typename ClockSource::duration>(std::chrono::duration<float>(timeout));
        }
        return expired;
    }

    void setProgress(float progress)
    {
        if (!running)
            return;
        start_time = ClockSource::now() - std::chrono::duration_cast<typename ClockSource::duration>(std::chrono::duration<float>(timeout * progress));
    }

    float getTimeLeft() const
    {
        return timeout - getTimeElapsed();
    }

    float getTimeElapsed() const
    {
        return std::chrono::duration<float>(ClockSource::now() - start_time).count();
    }

    // Return the timer progress in the range 0.0 to 1.0, where 0.0 is at the start.
    float getProgress() const
    {
        return getTimeElapsed() / timeout;
    }

private:
    bool running = false;
    bool auto_restart = false;
    typename ClockSource::time_point start_time;
    float timeout = 0.0f;
};

//Simple stopwatch, to measure elapsed time
template<typename ClockSource> class StopwatchBase
{
public:
    StopwatchBase()
    {
        start_time = ClockSource::now();
    }

    float get()
    {
        return std::chrono::duration<float>(ClockSource::now() - start_time).count();
    }

    float restart()
    {
        auto now = ClockSource::now();
        float result = std::chrono::duration<float>(now - start_time).count();
        start_time = now;
        return result;
    }

private:
    typename ClockSource::time_point start_time;
};

class EngineTime
{
public:
    using time_point = std::chrono::duration<double>;
    using duration = time_point;

    static time_point now();
};

// The normal timer runs at the game speed. If the game is paused or slowed down, this timer slows down as well.
using Timer = TimerBase<EngineTime>;
using Stopwatch = StopwatchBase<EngineTime>;

// The system timer always runs, even if the game is paused or at a different game speed.
using SystemTimer = TimerBase<std::chrono::steady_clock>;
using SystemStopwatch = StopwatchBase<std::chrono::steady_clock>;

} //!namespace

#endif//SP2_TIMER_H
