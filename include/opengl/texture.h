#ifndef _GLTEXTURE_H
#define _GLTEXTURE_H

#include <X11/Xlib-xcb.h>

#include <GL/gl.h>

#include <vector>

#include <core/region.h>

#define POWER_OF_TWO(v) ((v & (v - 1)) == 0)

#define COMP_TEX_COORD_X(m, vx) ((m).xx * (vx) + (m).x0)
#define COMP_TEX_COORD_Y(m, vy) ((m).yy * (vy) + (m).y0)

#define COMP_TEX_COORD_XY(m, vx, vy)		\
    ((m).xx * (vx) + (m).xy * (vy) + (m).x0)
#define COMP_TEX_COORD_YX(m, vx, vy)		\
    ((m).yx * (vx) + (m).yy * (vy) + (m).y0)

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

	typedef std::vector<Matrix> MatrixList;

	class List : public std::vector <GLTexture *> {

	    public:
		List ();
		List (unsigned int);
		List (const List &);
		~List ();

		List & operator= (const List &);

		void clear ();
	};

    public:
	
	GLuint name () const;
	GLenum target () const;
	GLenum filter () const;

	const Matrix & matrix () const;

	virtual void enable (Filter filter);
	virtual void disable ();

	bool mipmap () const;
	void setMipmap (bool);
	void setFilter (GLenum);
	void setWrap (GLenum);

	static void incRef (GLTexture *);
	static void decRef (GLTexture *);

	static List bindPixmapToTexture (Pixmap pixmap,
					 int width,
					 int height,
					 int depth);

	static List imageBufferToTexture (const char   *image,
					  unsigned int width,
					  unsigned int height);

	static List imageDataToTexture (const char   *image,
					unsigned int width,
					unsigned int height,
					GLenum       format,
					GLenum       type);

	static List readImageToTexture (const char   *imageFileName,
					unsigned int *returnWidth,
					unsigned int *returnHeight);

	friend class PrivateTexture;

    protected:
	GLTexture ();
	virtual ~GLTexture ();

	void setData (GLenum target, Matrix &m, bool mipmap);

    private:
	PrivateTexture *priv;
};

#endif
