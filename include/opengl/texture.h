#ifndef _GLTEXTURE_H
#define _GLTEXTURE_H

#include <X11/Xlib-xcb.h>

#include <GL/gl.h>

#include <boost/shared_ptr.hpp>

#define POWER_OF_TWO(v) ((v & (v - 1)) == 0)

#define COMP_TEX_COORD_X(m, vx) ((m)->xx * (vx) + (m)->x0)
#define COMP_TEX_COORD_Y(m, vy) ((m)->yy * (vy) + (m)->y0)

#define COMP_TEX_COORD_XY(m, vx, vy)		\
    ((m)->xx * (vx) + (m)->xy * (vy) + (m)->x0)
#define COMP_TEX_COORD_YX(m, vx, vy)		\
    ((m)->yx * (vx) + (m)->yy * (vy) + (m)->y0)


class CompScreen;
class PrivateTexture;

class GLTexture {
    public:

	typedef enum {
	    Fast,
	    Good
	} Filter;

	typedef struct {
	    float xx; float yx;
	    float xy; float yy;
	    float x0; float y0;
	} Matrix;

    public:
	GLTexture (CompScreen *);
	~GLTexture ();

	void reset ();
	
	GLuint name ();
	GLenum target ();

	Matrix & matrix ();

	void damage ();

	bool bindPixmap (Pixmap	pixmap, int width, int height, int depth);
	void releasePixmap ();
	bool hasPixmap ();
	
	void enable (Filter filter);
	void disable ();

	bool & mipmap ();


	static bool imageBufferToTexture (GLTexture    *texture,
					  const char   *image,
					  unsigned int width,
					  unsigned int height);

	static bool imageDataToTexture (GLTexture    *texture,
					const char   *image,
					unsigned int width,
					unsigned int height,
					GLenum       format,
					GLenum       type);

	static bool readImageToTexture (CompScreen   *screen,
					GLTexture    *texture,
					const char   *imageFileName,
					unsigned int *returnWidth,
					unsigned int *returnHeight);


    private:
	boost::shared_ptr <PrivateTexture> priv;
};

#endif
