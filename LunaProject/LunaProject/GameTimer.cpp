#include "stdafx.h"
#include "GameTimer.h"

GameTimer::GameTimer()
	: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false) 
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::DeltaTime() const {
	return (float)mDeltaTime;
}

float GameTimer::TotalTime() const {
	
	// if we're stopped, total time is time since stopped, minus all
	if (mStopped) {
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
	else {
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}

void GameTimer::Reset() {
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void GameTimer::Start() {
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (mStopped) {
		mPausedTime += (startTime - mStopTime);
		mPrevTime = startTime;

		// reset the time we are stopped for
		mStopTime = 0;
		mStopped = false;
	}
}

void GameTimer::Stop() {
	if (!mStopped) {
		QueryPerformanceCounter((LARGE_INTEGER*)&mCurrTime);
		mStopTime = mCurrTime;
		mStopped = true;
	}
}

void GameTimer::Tick() {
	if (mStopped) {
		mDeltaTime = 0.0;
		return;
	}

	// time this frame
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	mPrevTime = mCurrTime;

	// Force nonnegative.  CDXUTimer mentions that if the processor
	// goes into a power save mode or we get shuffled to another processor,
	// then mDeltaTime can be negative.
	if (mDeltaTime < 0.0) {
		mDeltaTime = 0.0;
	}
}