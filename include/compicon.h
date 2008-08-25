#ifndef _COMPICON_H
#define _COMPICON_H

#include <comptexture.h>
class CompScreen;

class CompIcon {
    public:
	CompIcon (CompScreen *screen, unsigned width, unsigned int height);
	~CompIcon ();

	CompTexture & texture ();
	unsigned int width ();
	unsigned int height ();
	unsigned char* data ();

    private:
	CompTexture   mTexture;
	int           mWidth;
	unsigned int  mHeight;
	unsigned char *mData;
	bool          mUpdateTex;
};

#endif
