#include <time.h>

class itemTimer {
public:
	float getTimeRemaining();
	float getTimeRemainingRatio();
	float getMaxTime();
	void setMaxTime(float);
	void startTimer();
private:
	float timeStarted;
	float maxTime;
};