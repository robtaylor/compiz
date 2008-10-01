#ifndef _PRIVATETEXTURE_H
#define _PRIVATETEXTURE_H

#include <map>

#include <GL/gl.h>
#include <GL/glx.h>
#include <opengl/texture.h>

class GLScreen;
class GLDisplay;

class PrivateTexture {
    public:
	PrivateTexture (GLTexture *);
	~PrivateTexture ();

	static GLTexture::List loadImageData (const char   *image,
					      unsigned int width,
					      unsigned int height,
					      GLenum       format,
					      GLenum       type);

    public:
	GLTexture         *texture;
	GLuint            name;
	GLenum            target;
	GLenum            filter;
	GLenum            wrap;
	GLTexture::Matrix matrix;
	bool              mipmap;
	bool              mipmapSupport;
	bool              initial;
	int               refCount;
	CompRect          size;
};

class TfpTexture : public GLTexture {
    public:
	TfpTexture ();
	~TfpTexture ();

	void enable (Filter filter);
	void disable ();

	static List bindPixmapToTexture (Pixmap pixmap,
					 int width,
					 int height,
					 int depth);

    public:
	GLXPixmap pixmap;
	bool      damaged;
	Damage    damage;
};

extern std::map<Damage, TfpTexture*> boundPixmapTex;

#endif
