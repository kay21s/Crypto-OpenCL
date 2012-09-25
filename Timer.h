#ifndef _TIMER_H_
#define _TIMER_H_
/**
 * \file Timer.h
 * \brief A timer class that provides a cross platform timer for use
 * in timing code progress with a high degree of accuracy.
 * FIXME:
 * 1s = 1000ms (millisecond)
 * 1ms = 1000us (microsecond)
 * 1us = 1000ns (nanosecond)
 * this counter returns in terms of us
 */
#ifdef _WIN32
/**
 * \typedef __int64 i64
 * \brief Maps the windows 64 bit integer to a uniform name
 */
#if defined(__MINGW64__) || defined(__MINGW32__)
typedef long long i64;
#else
typedef __int64 i64;
#endif
#else
/**
 * \typedef long long i64
 * \brief Maps the linux 64 bit integer to a uniform name
 */
typedef long long i64;
#endif

/**
 * \class CPerfCounter
 * \brief Counter that provides a fairly accurate timing mechanism for both
 * windows and linux. This timer is used extensively in all the samples.
 */
class CPerfCounter {

public:
    /**
     * \fn CPerfCounter()
     * \brief Constructor for CPerfCounter that initializes the class
     */
    CPerfCounter();
    /**
     * \fn ~CPerfCounter()
     * \brief Destructor for CPerfCounter that cleans up the class
     */
    ~CPerfCounter();
    /**
     * \fn void Start(void)
     * \brief Start the timer
     * \sa Stop(), Reset()
     */
    void Start(void);
    /**
     * \fn void Stop(void)
     * \brief Stop the timer
     * \sa Start(), Reset()
     */
    void Stop(void);
    /**
     * \fn void Reset(void)
     * \brief Reset the timer to 0
     * \sa Start(), Stop()
     */
    void Reset(void);
    /**
     * \fn double GetElapsedTime(void)
     * \return Amount of time that has accumulated between the \a Start()
     * and \a Stop() function calls
	 *
	 * Return unit is us~~~~~
     */
	double CPerfCounter::GetTotalTime(void);

	// time from _start to now
    double GetElapsedTime(void);

private:

    i64 _freq;
    i64 _clocks;
    i64 _start;
};

#endif // _TIMER_H_

