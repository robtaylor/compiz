#include <core/icon.h>

CompIcon::CompIcon (CompScreen *screen, unsigned int width,
		    unsigned int height) :
    mWidth (width),
    mHeight (height),
    mData (new unsigned char[width * height * 4]),
    mUpdateTex (true)
{
}

CompIcon::~CompIcon ()
{
    delete mData;
}

unsigned int
CompIcon::width ()
{
    return mWidth;
}

unsigned int
CompIcon::height ()
{
    return mHeight;
}

unsigned char*
CompIcon::data ()
{
    mUpdateTex = true;
    return mData;
}
