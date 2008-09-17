#include <core/point.h>

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
