#include "StreamGenerator.hpp"
#include <stdio.h>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/*
OneStream::OneStream(unsigned int rate, unsigned int deadline, unsigned int period)
{
	setRate(rate);
	setDeadline(deadline);
	setPeriod(period);
}*/

double gaussrand()
{
	static double V1, V2, S;
	static int phase = 0;
	double X;

	if(phase == 0) {
		do {
			double U1 = (double)rand() / RAND_MAX;
			double U2 = (double)rand() / RAND_MAX;

			V1 = 2 * U1 - 1;
			V2 = 2 * U2 - 1;
			S = V1 * V1 + V2 * V2;
		} while(S >= 1 || S == 0);

		X = V1 * sqrt(-2 * log(S) / S);
	} else
		X = V2 * sqrt(-2 * log(S) / S);

	phase = 1 - phase;

	return X;
}

unsigned int getrand(unsigned int avg, unsigned int var)
{
	return (unsigned int)(gaussrand() * var + avg);
}

void set_random(unsigned char *input, int len)
{
	int i;
	for (i = 0; i < len; i ++)
		input[i] = rand() % 26 + 'a';
}


void StreamGenerator::SetStreamNumber(unsigned int stream_num)
{
	_stream_num = stream_num;
}

void StreamGenerator::SetInterval(unsigned int time_interval)
{
	_interval = time_interval;
}

void StreamGenerator::InitStreams(unsigned int rate_avg, unsigned int rate_var, 
	unsigned int dl_avg, unsigned int dl_var, unsigned int pd_avg, unsigned int pd_var)
{
	unsigned int res;

	_rate_avg = rate_avg;
	_rate_var = rate_var;

	_deadline_avg = dl_avg;
	_deadline_var = dl_var;

	_period_avg = pd_avg;
	_period_var = pd_var;

	if (_stream_num == 0) {
		std::cout << "Init Stream number first~~~~~~" << std::endl;
		exit(0);
	}
	_streams = new OneStream[_stream_num];

	for (unsigned int i = 0; i < _stream_num; i ++) {
		_streams[i].setRate(getrand(_rate_avg, _rate_var));
	}
	for (unsigned int i = 0; i < _stream_num; i ++) {
		// Currently, we make deadline = period
		res = getrand(_deadline_avg, _deadline_var);
		_streams[i].setDeadline(res);
		_streams[i].setPeriod(res);
	}
	for (unsigned int i = 0; i < _stream_num; i ++) {
		//_streams[i].setPeriod(getrand(_period_avg, _period_var));
		_streams[i].setFrameSize();

		_streams[i].setKey('a');
		_streams[i].setIV('a');
	}

	return;
}

void StreamGenerator::StartStreams()
{
	t.Reset();
	t.Start();

	_times = 0;
}

i64 StreamGenerator::GetStartTimestamp()
{
	return _start;
}

unsigned int StreamGenerator::GetStreams(unsigned char *buffer, unsigned int buffer_size, 
			unsigned char *keyBuffer, unsigned char *ivBuffer,
			unsigned int *stream_offset, unsigned int *stream_num)
{
	double elapsed_time;
	unsigned int time_point;
	unsigned int interval_frames;

	_times ++;
	unsigned int stream = 0; // which frame are we generating
	stream_offset[0] = 0;
	time_point = _interval * _times;

	for (unsigned int i = 0; i < _stream_num; i ++) {

		// For Each Stream

		// kI/p - (k-1)I/p = frame number in the interval, maybe not fixed
		interval_frames = time_point / _streams[i].getPeriod()
			- (time_point - _interval) / _streams[i].getPeriod();
		if (interval_frames == 0) {
			//std::cout << "interval_frames == 0~~~~~" << std::endl;
			//exit(0);
			continue;
		}
		
		// These frames belonging to the same stream are 
		stream_offset[stream + 1] = stream_offset[stream] + interval_frames * _streams[i].getFrameSize();
		if (stream_offset[stream + 1] > buffer_size) {
			std::cout << "The buffer is too small~~~~~"<< stream_offset[stream + 1] << "  " 
				<< buffer_size << " stream num: " << stream << std::endl;
			exit(0);
		}
		// set the frame content with random characters
		//set_random( &(buffer[stream_offset[stream]]), interval_frames * _streams[i].getFrameSize());
		//memset(&(buffer[stream_offset[stream]]), 'a', interval_frames * _streams[i].getFrameSize());

		// Copy key and iv to the buffer
		memcpy(&(keyBuffer[stream * 16]), _streams[i].getKeyPtr(), 16);
		memcpy(&(ivBuffer[stream * 16]), _streams[i].getIVPtr(), 16);

		stream ++;
	}

	*stream_num = stream; // tell the caller how many frames we generated

	do {
		elapsed_time = t.GetElapsedTime();
		
		if (elapsed_time - time_point > 1) { // surpassed the time point more than 1 ms
			std::cout << "Time point lost!!!!" << elapsed_time << " "<<  time_point << std::endl;
			exit(0);
		}


	} while(abs(elapsed_time - time_point) > 1);

	return stream_offset[stream];
}

unsigned int StreamGenerator::GetMaxBufferSize()
{
	unsigned int bytes = 0;

	for (unsigned int i = 0; i < _stream_num; i ++) {
		bytes += ceil((float)(_streams[i].getDeadline()/ _streams[i].getPeriod())) * _streams[i].getFrameSize();
	}

	return bytes;
}

unsigned int StreamGenerator::GetMinDeadline()
{
	unsigned int minDeadline = 10000;

	for (unsigned int i = 0; i < _stream_num; i ++) {
		if (minDeadline < _streams[i].getDeadline()) {
			minDeadline = _streams[i].getDeadline();
		}
	}

	return minDeadline;
}

bool StreamGenerator::CheckSchedulability()
{
	unsigned int minDeadline;
	minDeadline = GetMinDeadline();

	if (_interval <= minDeadline/2) {
		return 1;
	} else {
		std::cout << "minDeadline : " << minDeadline
			<< " interval : " << _interval << std::endl;
		return 0;
	}
}
