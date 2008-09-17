#ifndef _COMPICON_H
#define _COMPICON_H

class CompScreen;

class CompIcon {
    public:
	CompIcon (CompScreen *screen, unsigned width, unsigned int height);
	~CompIcon ();

	unsigned int width ();
	unsigned int height ();
	unsigned char* data ();

    private:
	int           mWidth;
	unsigned int  mHeight;
	unsigned char *mData;
	bool          mUpdateTex;
};

#endif
