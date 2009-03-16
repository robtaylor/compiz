/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

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

class GLTexture : public CompRect {
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

	typedef boost::function<List (Pixmap, int, int, int)> BindPixmapProc;
	typedef unsigned int BindPixmapHandle;

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

	static List imageBufferToTexture (const char *image,
					  CompSize   size);

	static List imageDataToTexture (const char *image,
					CompSize   size,
					GLenum     format,
					GLenum     type);

	static List readImageToTexture (CompString &imageFileName,
					CompSize   &size);

	friend class PrivateTexture;

    protected:
	GLTexture ();
	virtual ~GLTexture ();

	void setData (GLenum target, Matrix &m, bool mipmap);

    private:
	PrivateTexture *priv;
};

#endif
