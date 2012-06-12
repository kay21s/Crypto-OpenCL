#ifndef _STREAMGENERATOR_H_
#define _STREAMGENERATOR_H_

#include <math.h>
#include <memory.h>
#include "Timer.h"

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


class OneStream {

public:
	OneStream() : _rate(0), _deadline(0), _period(0), _frame_size(0){}
	~OneStream() {}

	void setRate(unsigned int a) {_rate = a;}
	void setDeadline(unsigned int a) {_deadline = a;}
	void setPeriod(unsigned int a) {_period = a;}
	void setFrameSize()
	{
		unsigned int b_rate = _rate/8; // convert to byte
		_frame_size = ceil((float)((b_rate * _period * 1e-3) / 16)) * 16; // a frame block should be 128 bits aligned
	}

	void setIV(unsigned char a)		{memset(_iv, a, 16);}
	void setKey(unsigned char a)	{memset(_key, a, 16);}

	unsigned int getRate()		{return _rate;}
	unsigned int getDeadline()	{return _deadline;} // unit : ms
	unsigned int getPeriod()	{return _period;} // unit : ms
	unsigned int getFrameSize()	{return _frame_size;}

	unsigned char *getIVPtr()	{return _iv;}
	unsigned char *getKeyPtr()	{return _key;}

private:
	unsigned int _rate;		// bps
	unsigned int _deadline;	// ms
	unsigned int _period;	// ms
	unsigned int _frame_size;	// Bytes

	unsigned char _iv[16]; //128 bits initial vector
	unsigned char _key[16]; //128 bits key
};


class StreamGenerator {

public:

	StreamGenerator() : _times(0), _stream_num(0), _start(0) {}
	~StreamGenerator() {}

	void SetStreamNumber(unsigned int);
	void SetInterval(unsigned int);
	unsigned int GetStreamNumber()	{ return _stream_num; }
	unsigned int GetInterval()	{ return _interval; }

	void InitStreams(unsigned int, unsigned int,
		unsigned int, unsigned int, unsigned int, unsigned int);
	void StartStreams();
	i64 GetStartTimestamp();
	void GetStreams(unsigned char *, unsigned int,
		unsigned char *, unsigned char *,
		unsigned int *, unsigned int *);

	unsigned int GetMaxBufferSize();

private:

	unsigned int _stream_num;

	CPerfCounter t;

	unsigned int _rate_avg;
	unsigned int _rate_var;
	unsigned int _deadline_avg;
	unsigned int _deadline_var;
	unsigned int _period_avg;
	unsigned int _period_var;

	i64 _start;				// When the game starts
	unsigned int _times;	// The nth time that fetch a batch of streams
	unsigned int _interval;	// The time interval between two batches (ms)
	OneStream *_streams;	// stream set
};


#endif