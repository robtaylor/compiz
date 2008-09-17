#include <core/size.h>

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
