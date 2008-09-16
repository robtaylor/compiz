#ifndef _PRIVATETEXTURE_H
#define _PRIVATETEXTURE_H

#include <GL/gl.h>
#include <GL/glx.h>
#include <opengl/texture.h>

class GLScreen;
class GLDisplay;

class PrivateTexture {
    public:
	PrivateTexture (GLTexture *);
	~PrivateTexture ();

	bool loadImageData (const char   *image,
			    unsigned int width,
			    unsigned int height,
			    GLenum       format,
			    GLenum       type);

    public:
	GLTexture         *texture;
	GLuint            name;
	GLenum            target;
	GLfloat           dx, dy;
	GLXPixmap         pixmap;
	GLenum            filter;
	GLenum            wrap;
	GLTexture::Matrix matrix;
	bool              damaged;
	bool              mipmap;
	unsigned int      width;
	unsigned int      height;
};

#endif
