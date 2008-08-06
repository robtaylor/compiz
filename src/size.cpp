#include <compsize.h>

CompSize::CompSize () :
   mWidth (0),
   mHeight (0)
{
}

CompSize::CompSize (unsigned int width, unsigned int height) :
   mWidth (width),
   mHeight (height)
{
}

unsigned int
CompSize::width ()
{
    return mWidth;
}

unsigned int
CompSize::height ()
{
    return mHeight;
}

void
CompSize::setWidth (unsigned int width)
{
    mWidth = width;
}

void
CompSize::setHeight (unsigned int height)
{
    mHeight = height;
}
