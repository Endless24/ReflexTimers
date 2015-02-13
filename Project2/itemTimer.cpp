#include "itemTimer.h"
//a countdown timer
float itemTimer::getTimeRemaining() {
	float time = (timeStarted-((float)clock()/(float)CLOCKS_PER_SEC)) + maxTime;
	return time ? time : 0.00001f; //ensure we don't ever return a 0
}

float itemTimer::getTimeRemainingRatio() {
	return getTimeRemaining()/getMaxTime();
}

float itemTimer::getMaxTime() {
	return maxTime ? maxTime : 1; //don't ever return 0
}

void itemTimer::startTimer() {
	timeStarted = (float)((float)clock()/(float)CLOCKS_PER_SEC);
}

void itemTimer::setMaxTime(float time) {
	maxTime = time;
}