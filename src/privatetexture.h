#ifndef _PRIVATETEXTURE_H
#define _PRIVATETEXTURE_H

#include <comptexture.h>

class CompScreen;

class PrivateTexture {
    public:
	PrivateTexture (CompTexture *, CompScreen *);
	~PrivateTexture ();

	void init ();
	void fini ();

	bool loadImageData (const char   *image,
			    unsigned int width,
			    unsigned int height,
			    GLenum       format,
			    GLenum       type);

    public:
	CompScreen          *screen;
	CompTexture         *texture;
	GLuint              name;
	GLenum              target;
	GLfloat             dx, dy;
	GLXPixmap           pixmap;
	GLenum              filter;
	GLenum              wrap;
	CompTexture::Matrix matrix;
	Bool                damaged;
	Bool                mipmap;
	unsigned int        width;
	unsigned int        height;
};


#endif