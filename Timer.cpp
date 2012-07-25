#include "Timer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

CPerfCounter::CPerfCounter() : _clocks(0), _start(0)
{

#ifdef _WIN32
    QueryPerformanceFrequency((LARGE_INTEGER *)&_freq);
#else
    _freq = 1000;
#endif

}

CPerfCounter::~CPerfCounter()
{
    // EMPTY!
}

void
CPerfCounter::Start(void)
{

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER *)&_start);
#else
    struct timespec s;
    clock_gettime( CLOCK_REALTIME, &s );
    _start = (i64)s.tv_sec * 1e9 + (i64)s.tv_nsec;
#endif

}

void
CPerfCounter::Stop(void)
{
    i64 n;

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER *)&n);
#else
    struct timespec s;
    clock_gettime( CLOCK_REALTIME, &s );
    n = (i64)s.tv_sec * 1e9 + (i64)s.tv_nsec;
#endif

    n -= _start;
    _start = 0;
    _clocks += n;
}

void
CPerfCounter::Reset(void)
{
    _clocks = 0;
}

double
CPerfCounter::GetTotalTime(void)
{
	//!! ns * 1000 = ms, we use ms as the unit

#if _WIN32
    return (double)(_clocks * 1000) / (double) _freq;
#else
    return (double)(_clocks * 1000) / (double) 1e9;
#endif

}

double
CPerfCounter::GetElapsedTime(void)
{
    i64 n;

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER *)&n);
#else
    struct timespec s;
    clock_gettime( CLOCK_REALTIME, &s );
    n = (i64)s.tv_sec * 1e9 + (i64)s.tv_nsec;
#endif

#if _WIN32
    return (double)((n - _start) * 1000) / (double) _freq;
#else
    return (double)((n - _start) * 1000) / (double) 1e9;
#endif

}

