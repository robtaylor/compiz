#include <core/timer.h>
#include <core/screen.h>
#include "privatescreen.h"

CompTimer::CompTimer () :
    mActive (false),
    mMinTime (0),
    mMaxTime (0),
    mMinLeft (0),
    mMaxLeft (0),
    mCallBack (NULL)
{
}

CompTimer::~CompTimer ()
{
    if (mActive)
	screen->priv->removeTimer (this);
}

void
CompTimer::setTimes (unsigned int min, unsigned int max)
{
    bool wasActive = mActive;
    if (mActive)
	stop ();
    mMinTime = min;
    mMaxTime = (min <= max)? max : min;

    if (wasActive)
	start ();
}
	
void
CompTimer::setCallback (CompTimer::CallBack callback)
{
    bool wasActive = mActive;
    if (mActive)
	stop ();
    mCallBack = callback;

    if (wasActive)
	start ();
}

void
CompTimer::start ()
{
    stop ();
    mActive = true;
    screen->priv->addTimer (this);
}

void
CompTimer::start (unsigned int min, unsigned int max)
{
    stop ();
    setTimes (min, max);
    start ();
}

void
CompTimer::start (CompTimer::CallBack callback,
		  unsigned int min, unsigned int max)
{
    stop ();
    setTimes (min, max);
    setCallback (callback);
    start ();
}

void
CompTimer::stop ()
{
    mActive = false;
    screen->priv->removeTimer (this);
}

unsigned int
CompTimer::minTime ()
{
    return mMinTime;
}

unsigned int
CompTimer::maxTime ()
{
    return mMaxTime;
}

unsigned int
CompTimer::minLeft ()
{
    return (mMinLeft < 0)? 0 : mMinLeft;
}

unsigned int
CompTimer::maxLeft ()
{
    return (mMaxLeft < 0)? 0 : mMaxLeft;
}

bool
CompTimer::active ()
{
    return mActive;
}
