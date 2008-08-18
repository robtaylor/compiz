#include <compicon.h>

CompIcon::CompIcon (CompScreen *screen, unsigned int width,
		    unsigned int height) :
    mTexture (screen),
    mWidth (width),
    mHeight (height),
    mData (new unsigned char[width * height * 4]),
    mUpdateTex (true)
{
}

CompIcon::~CompIcon ()
{
    free (mData);
}

CompTexture &
CompIcon::texture ()
{
    if (mUpdateTex)
    {
	mUpdateTex = false;
	mTexture.reset ();
	if (!mTexture.imageBufferToTexture (&mTexture,
	    	reinterpret_cast<const char *> (mData), mWidth, mHeight))
	    mTexture.reset ();
    }
    return mTexture;
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