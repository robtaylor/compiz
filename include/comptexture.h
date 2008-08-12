#ifndef _COMPTEXTURE_H
#define _COMPTEXTURE_H

#include <X11/Xlib-xcb.h>

#include <GL/gl.h>

#include <boost/shared_ptr.hpp>

#define POWER_OF_TWO(v) ((v & (v - 1)) == 0)

class CompScreen;
class PrivateTexture;

class CompTexture {
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
	CompTexture (CompScreen *);
	~CompTexture ();

	GLuint name ();
	GLenum target ();

	void damage ();

	Matrix & matrix ();
	
	void reset ();

	bool bindPixmap (Pixmap	pixmap, int width, int height, int depth);

	void releasePixmap ();

	void enable (Filter filter);

	void disable ();

	bool hasPixmap ();

	static bool imageBufferToTexture (CompTexture  *texture,
					  const char   *image,
					  unsigned int width,
					  unsigned int height);

	static bool imageDataToTexture (CompTexture  *texture,
					const char   *image,
					unsigned int width,
					unsigned int height,
					GLenum       format,
					GLenum       type);

	static bool
	readImageToTexture (CompScreen   *screen,
			    CompTexture  *texture,
			    const char	 *imageFileName,
			    unsigned int *returnWidth,
			    unsigned int *returnHeight);


    private:
	boost::shared_ptr <PrivateTexture> priv;
};

#endif
