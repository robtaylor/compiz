#include <comppoint.h>

CompPoint::CompPoint () :
   mX (0),
   mY (0)
{
}

CompPoint::CompPoint (int x, int y) :
   mX (x),
   mY (y)
{
}

int
CompPoint::x ()
{
    return mX;
}

int
CompPoint::y ()
{
    return mY;
}

void
CompPoint::setX (int x)
{
    mX = x;
}

void
CompPoint::setY (int y)
{
    mY = y;
}
