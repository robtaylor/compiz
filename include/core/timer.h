#ifndef _TIMER_H
#define _TIMER_H

#include <boost/function.hpp>

class CompTimer {

    public:

	typedef boost::function<bool ()> CallBack;

	CompTimer ();
	~CompTimer ();

	bool active ();
	unsigned int minTime ();
	unsigned int maxTime ();
	unsigned int minLeft ();
	unsigned int maxLeft ();

	void setTimes (unsigned int min, unsigned int max = 0);
	void setCallback (CallBack callback);

	void start ();
	void start (unsigned int min, unsigned int max = 0);
	void start (CallBack callback,
		    unsigned int min, unsigned int max = 0);
	void stop ();

	friend class CompScreen;
	friend class PrivateScreen;

    private:
	bool         mActive;
	unsigned int mMinTime;
	unsigned int mMaxTime;
	int          mMinLeft;
	int          mMaxLeft;
	CallBack     mCallBack;
};

#endif
