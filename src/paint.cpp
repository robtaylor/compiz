/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <compiz-core.h>

#include "privatescreen.h"
#include "privatewindow.h"

ScreenPaintAttrib defaultScreenPaintAttrib = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -DEFAULT_Z_CAMERA
};

WindowPaintAttrib defaultWindowPaintAttrib = {
    OPAQUE, BRIGHT, COLOR, 1.0f, 1.0f, 0.0f, 0.0f
};

void
CompScreen::preparePaint (int msSinceLastPaint)
    WRAPABLE_HND_FUNC(preparePaint, msSinceLastPaint)

void
CompScreen::donePaint ()
    WRAPABLE_HND_FUNC(donePaint)

void
CompScreen::applyTransform (const ScreenPaintAttrib *sAttrib,
			    CompOutput              *output,
			    CompTransform           *transform)
{
    WRAPABLE_HND_FUNC(applyTransform, sAttrib, output, transform)

    matrixTranslate (transform,
		     sAttrib->xTranslate,
		     sAttrib->yTranslate,
		     sAttrib->zTranslate + sAttrib->zCamera);
    matrixRotate (transform,
		  sAttrib->xRotate, 0.0f, 1.0f, 0.0f);
    matrixRotate (transform,
		  sAttrib->vRotate,
		  cosf (sAttrib->xRotate * DEG2RAD),
		  0.0f,
		  sinf (sAttrib->xRotate * DEG2RAD));
    matrixRotate (transform,
		  sAttrib->yRotate, 0.0f, 1.0f, 0.0f);
}

void
transformToScreenSpace (CompScreen    *screen,
			CompOutput    *output,
			float         z,
			CompTransform *transform)
{
    matrixTranslate (transform, -0.5f, -0.5f, z);
    matrixScale (transform,
		 1.0f  / output->width (),
		 -1.0f / output->height (),
		 1.0f);
    matrixTranslate (transform,
		     -output->x1 (),
		     -output->y2 (),
		     0.0f);
}

void
prepareXCoords (CompScreen *screen,
		CompOutput *output,
		float      z)
{
    glTranslatef (-0.5f, -0.5f, z);
    glScalef (1.0f  / output->width (),
	      -1.0f / output->height (),
	      1.0f);
    glTranslatef (-output->x1 (),
		  -output->y2 (),
		  0.0f);
}

void
CompScreen::paintCursor (CompCursor          *c,
			 const CompTransform *transform,
			 Region              region,
			 unsigned int        mask)
{
    WRAPABLE_HND_FUNC(paintCursor, c, transform, region, mask)

    int x1, y1, x2, y2;

    if (!c->image)
	return;

    x1 = c->x;
    y1 = c->y;
    x2 = c->x + c->image->width;
    y2 = c->y + c->image->height;

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

    c->screen->enableTexture (&c->image->texture, COMP_TEXTURE_FILTER_FAST);

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X (&c->matrix, x1),
		  COMP_TEX_COORD_Y (&c->matrix, y2));
    glVertex2i (x1, y2);
    glTexCoord2f (COMP_TEX_COORD_X (&c->matrix, x2),
		  COMP_TEX_COORD_Y (&c->matrix, y2));
    glVertex2i (x2, y2);
    glTexCoord2f (COMP_TEX_COORD_X (&c->matrix, x2),
		  COMP_TEX_COORD_Y (&c->matrix, y1));
    glVertex2i (x2, y1);
    glTexCoord2f (COMP_TEX_COORD_X (&c->matrix, x1),
		  COMP_TEX_COORD_Y (&c->matrix, y1));
    glVertex2i (x1, y1);

    glEnd ();

    c->screen->disableTexture (&c->image->texture);

    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
}

void
PrivateScreen::paintBackground (Region	      region,
				bool	      transformed)
{
    CompTexture *bg = &backgroundTexture;
    BoxPtr      pBox = region->rects;
    int	        n, nBox = region->numRects;
    GLfloat     *d, *data;

    if (!nBox)
	return;

    if (desktopWindowCount)
    {
	if (bg->name)
	{
	    finiTexture (screen, bg);
	    initTexture (screen, bg);
	}

	backgroundLoaded = false;

	return;
    }
    else
    {
	if (!backgroundLoaded)
	    updateScreenBackground (bg);

	backgroundLoaded = true;
    }

    data = (GLfloat *) malloc (sizeof (GLfloat) * nBox * 16);
    if (!data)
	return;

    d = data;
    n = nBox;
    while (n--)
    {
	*d++ = COMP_TEX_COORD_X (&bg->matrix, pBox->x1);
	*d++ = COMP_TEX_COORD_Y (&bg->matrix, pBox->y2);

	*d++ = pBox->x1;
	*d++ = pBox->y2;

	*d++ = COMP_TEX_COORD_X (&bg->matrix, pBox->x2);
	*d++ = COMP_TEX_COORD_Y (&bg->matrix, pBox->y2);

	*d++ = pBox->x2;
	*d++ = pBox->y2;

	*d++ = COMP_TEX_COORD_X (&bg->matrix, pBox->x2);
	*d++ = COMP_TEX_COORD_Y (&bg->matrix, pBox->y1);

	*d++ = pBox->x2;
	*d++ = pBox->y1;

	*d++ = COMP_TEX_COORD_X (&bg->matrix, pBox->x1);
	*d++ = COMP_TEX_COORD_Y (&bg->matrix, pBox->y1);

	*d++ = pBox->x1;
	*d++ = pBox->y1;

	pBox++;
    }

    glTexCoordPointer (2, GL_FLOAT, sizeof (GLfloat) * 4, data);
    glVertexPointer (2, GL_FLOAT, sizeof (GLfloat) * 4, data + 2);

    if (bg->name)
    {
	if (transformed)
	    screen->enableTexture (bg, COMP_TEXTURE_FILTER_GOOD);
	else
	    screen->enableTexture (bg, COMP_TEXTURE_FILTER_FAST);

	glDrawArrays (GL_QUADS, 0, nBox * 4);

	screen->disableTexture (bg);
    }
    else
    {
	glColor4us (0, 0, 0, 0);
	glDrawArrays (GL_QUADS, 0, nBox * 4);
	glColor4usv (defaultColor);
    }

    free (data);
}


/* This function currently always performs occlusion detection to
   minimize paint regions. OpenGL precision requirements are no good
   enough to guarantee that the results from using occlusion detection
   is the same as without. It's likely not possible to see any
   difference with most hardware but occlusion detection in the
   transformed screen case should be made optional for those who do
   see a difference. */
void
PrivateScreen::paintOutputRegion (const CompTransform *transform,
				  Region	       region,
				  CompOutput	       *output,
				  unsigned int	       mask)
{
    static Region tmpRegion = NULL;
    CompWindow    *w;
    CompCursor	  *c;
    int		  count, windowMask, odMask;
    CompWindow	  *fullscreenWindow = NULL;
    CompWalker    walk;
    bool          status;
    bool          withOffset = false;
    CompTransform vTransform;
    int           offX, offY;
    Region        clip = region;

    if (!tmpRegion)
    {
	tmpRegion = XCreateRegion ();
	if (!tmpRegion)
	    return;
    }

    if (mask & PAINT_SCREEN_TRANSFORMED_MASK)
    {
	windowMask     = PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK;
	count	       = 1;
    }
    else
    {
	windowMask     = 0;
	count	       = 0;
    }

    XSubtractRegion (region, &emptyRegion, tmpRegion);

    screen->initWindowWalker (&walk);

    if (!(mask & PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK))
    {
	/* detect occlusions */
	for (w = (*walk.last) (screen); w; w = (*walk.prev) (w))
	{
	    if (w->destroyed ())
		continue;

	    if (!w->shaded ())
	    {
		if (w->attrib ().map_state != IsViewable || !w->damaged ())
		    continue;
	    }

	    /* copy region */
	    XSubtractRegion (tmpRegion, &emptyRegion, w->clip ());

	    odMask = PAINT_WINDOW_OCCLUSION_DETECTION_MASK;
		
	    if ((windowOffsetX != 0 || windowOffsetY != 0) &&
		!w->onAllViewports ())
	    {
		withOffset = true;

		w->getMovementForOffset (windowOffsetX,
					 windowOffsetY,
					 &offX, &offY);

		vTransform = *transform;
		matrixTranslate (&vTransform, offX, offY, 0);
	 
		XOffsetRegion (w->clip (), -offX, -offY);

		odMask |= PAINT_WINDOW_WITH_OFFSET_MASK;
		status = w->paint (&w->paintAttrib (), &vTransform,
				   tmpRegion, odMask);
	    }
	    else
	    {
		withOffset = false;
		status = w->paint (&w->paintAttrib (), transform, tmpRegion,
				   odMask);
	    }

	    if (status)
	    {
		if (withOffset)
		{
		    XOffsetRegion (w->region (), offX, offY);
		    XSubtractRegion (tmpRegion, w->region (), tmpRegion);
		    XOffsetRegion (w->region (), -offX, -offY);
		}
		else
		    XSubtractRegion (tmpRegion, w->region (), tmpRegion);

		/* unredirect top most fullscreen windows. */
		if (count == 0 &&
		    opt[COMP_SCREEN_OPTION_UNREDIRECT_FS].value.b)
		{
		    if (XEqualRegion (w->region (), &this->region) &&
			!REGION_NOT_EMPTY (tmpRegion))
		    {
			fullscreenWindow = w;
		    }
		    else
		    {
			for (unsigned int i = 0; i < outputDevs.size (); i++)
			    if (XEqualRegion (w->region (),
					      outputDevs[i].region ()))
				fullscreenWindow = w;
		    }
		}
	    }

	    count++;
	}
    }

    if (fullscreenWindow)
	fullscreenWindow->unredirect ();

    if (!(mask & PAINT_SCREEN_NO_BACKGROUND_MASK))
	paintBackground (tmpRegion, (mask & PAINT_SCREEN_TRANSFORMED_MASK));

    /* paint all windows from bottom to top */
    for (w = (*walk.first) (screen); w; w = (*walk.next) (w))
    {
	if (w->destroyed ())
	    continue;

	if (w == fullscreenWindow)
	    continue;

	if (!w->shaded ())
	{
	    if (w->attrib ().map_state != IsViewable || !w->damaged ())
		continue;
	}

	if (!(mask & PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK))
	    clip = w->clip ();

	if ((windowOffsetX != 0 || windowOffsetY != 0) &&
	    !w->onAllViewports ())
	{
	    w->getMovementForOffset (windowOffsetX,
				     windowOffsetY, &offX, &offY);

	    vTransform = *transform;
	    matrixTranslate (&vTransform, offX, offY, 0);
	    w->paint (&w->paintAttrib (), &vTransform, clip,
		      windowMask | PAINT_WINDOW_WITH_OFFSET_MASK);
	}
	else
	{
	    w->paint (&w->paintAttrib (), transform, clip, windowMask);
	}
    }

    if (walk.fini)
	(*walk.fini) (screen, &walk);

    /* paint cursors */
    for (c = cursors; c; c = c->next)
	screen->paintCursor (c, transform, tmpRegion, 0);
}

void
CompScreen::enableOutputClipping (const CompTransform *transform,
				  Region              region,
				  CompOutput          *output)
{
    WRAPABLE_HND_FUNC(enableOutputClipping, transform, region, output)

    GLdouble h = priv->height;

    GLdouble p1[2] = { region->extents.x1, h - region->extents.y2 };
    GLdouble p2[2] = { region->extents.x2, h - region->extents.y1 };

    GLdouble halfW = output->width () / 2.0;
    GLdouble halfH = output->height () / 2.0;

    GLdouble cx = output->x1 () + halfW;
    GLdouble cy = (h - output->y2 ()) + halfH;

    GLdouble top[4]    = { 0.0, halfH / (cy - p1[1]), 0.0, 0.5 };
    GLdouble bottom[4] = { 0.0, halfH / (cy - p2[1]), 0.0, 0.5 };
    GLdouble left[4]   = { halfW / (cx - p1[0]), 0.0, 0.0, 0.5 };
    GLdouble right[4]  = { halfW / (cx - p2[0]), 0.0, 0.0, 0.5 };

    glPushMatrix ();
    glLoadMatrixf (transform->m);

    glClipPlane (GL_CLIP_PLANE0, top);
    glClipPlane (GL_CLIP_PLANE1, bottom);
    glClipPlane (GL_CLIP_PLANE2, left);
    glClipPlane (GL_CLIP_PLANE3, right);

    glEnable (GL_CLIP_PLANE0);
    glEnable (GL_CLIP_PLANE1);
    glEnable (GL_CLIP_PLANE2);
    glEnable (GL_CLIP_PLANE3);

    glPopMatrix ();
}

void
CompScreen::disableOutputClipping ()
{
    WRAPABLE_HND_FUNC(disableOutputClipping)

    glDisable (GL_CLIP_PLANE0);
    glDisable (GL_CLIP_PLANE1);
    glDisable (GL_CLIP_PLANE2);
    glDisable (GL_CLIP_PLANE3);
}

#define CLIP_PLANE_MASK (PAINT_SCREEN_TRANSFORMED_MASK | \
			 PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK)

void
CompScreen::paintTransformedOutput (const ScreenPaintAttrib *sAttrib,
				    const CompTransform     *transform,
				    Region                  region,
				    CompOutput              *output,
				    unsigned int            mask)
{
    WRAPABLE_HND_FUNC(paintTransformedOutput, sAttrib, transform,
		      region, output, mask)

    CompTransform sTransform = *transform;

    if (mask & PAINT_SCREEN_CLEAR_MASK)
	priv->display->clearTargetOutput (GL_COLOR_BUFFER_BIT);

    setLighting (true);

    applyTransform (sAttrib, output, &sTransform);

    if ((mask & CLIP_PLANE_MASK) == CLIP_PLANE_MASK)
    {
	enableOutputClipping (&sTransform, region, output);

	transformToScreenSpace (this, output, -sAttrib->zTranslate,
				&sTransform);

	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	priv->paintOutputRegion (&sTransform, region, output, mask);

	glPopMatrix ();

	disableOutputClipping ();
    }
    else
    {
	transformToScreenSpace (this, output, -sAttrib->zTranslate,
				&sTransform);

	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	priv->paintOutputRegion (&sTransform, region, output, mask);

	glPopMatrix ();
    }
}

bool
CompScreen::paintOutput (const ScreenPaintAttrib *sAttrib,
			 const CompTransform     *transform,
			 Region                  region,
			 CompOutput              *output,
			 unsigned int            mask)
{
    WRAPABLE_HND_FUNC_RETURN(bool, paintOutput, sAttrib, transform,
			     region, output, mask)

    CompTransform sTransform = *transform;

    if (mask & PAINT_SCREEN_REGION_MASK)
    {
	if (mask & PAINT_SCREEN_TRANSFORMED_MASK)
	{
	    if (mask & PAINT_SCREEN_FULL_MASK)
	    {
		region = output->region ();
		paintTransformedOutput (sAttrib, &sTransform, region,
					output, mask);

		return true;
	    }

	    return false;
	}

	/* fall through and redraw region */
    }
    else if (mask & PAINT_SCREEN_FULL_MASK)
    {
	paintTransformedOutput (sAttrib, &sTransform, output->region (),
				output, mask);

	return true;
    }
    else
	return false;

    setLighting (false);

    transformToScreenSpace (this, output, -DEFAULT_Z_CAMERA, &sTransform);

    glPushMatrix ();
    glLoadMatrixf (sTransform.m);

    priv->paintOutputRegion (&sTransform, region, output, mask);

    glPopMatrix ();

    return true;
}

#define ADD_RECT(data, m, n, x1, y1, x2, y2)	   \
    for (it = 0; it < n; it++)			   \
    {						   \
	*(data)++ = COMP_TEX_COORD_X (&m[it], x1); \
	*(data)++ = COMP_TEX_COORD_Y (&m[it], y1); \
    }						   \
    *(data)++ = (x1);				   \
    *(data)++ = (y1);				   \
    *(data)++ = 0.0;				   \
    for (it = 0; it < n; it++)			   \
    {						   \
	*(data)++ = COMP_TEX_COORD_X (&m[it], x1); \
	*(data)++ = COMP_TEX_COORD_Y (&m[it], y2); \
    }						   \
    *(data)++ = (x1);				   \
    *(data)++ = (y2);				   \
    *(data)++ = 0.0;				   \
    for (it = 0; it < n; it++)			   \
    {						   \
	*(data)++ = COMP_TEX_COORD_X (&m[it], x2); \
	*(data)++ = COMP_TEX_COORD_Y (&m[it], y2); \
    }						   \
    *(data)++ = (x2);				   \
    *(data)++ = (y2);				   \
    *(data)++ = 0.0;				   \
    for (it = 0; it < n; it++)			   \
    {						   \
	*(data)++ = COMP_TEX_COORD_X (&m[it], x2); \
	*(data)++ = COMP_TEX_COORD_Y (&m[it], y1); \
    }						   \
    *(data)++ = (x2);				   \
    *(data)++ = (y1);				   \
    *(data)++ = 0.0

#define ADD_QUAD(data, m, n, x1, y1, x2, y2)		\
    for (it = 0; it < n; it++)				\
    {							\
	*(data)++ = COMP_TEX_COORD_XY (&m[it], x1, y1);	\
	*(data)++ = COMP_TEX_COORD_YX (&m[it], x1, y1);	\
    }							\
    *(data)++ = (x1);					\
    *(data)++ = (y1);					\
    *(data)++ = 0.0;					\
    for (it = 0; it < n; it++)				\
    {							\
	*(data)++ = COMP_TEX_COORD_XY (&m[it], x1, y2);	\
	*(data)++ = COMP_TEX_COORD_YX (&m[it], x1, y2);	\
    }							\
    *(data)++ = (x1);					\
    *(data)++ = (y2);					\
    *(data)++ = 0.0;					\
    for (it = 0; it < n; it++)				\
    {							\
	*(data)++ = COMP_TEX_COORD_XY (&m[it], x2, y2);	\
	*(data)++ = COMP_TEX_COORD_YX (&m[it], x2, y2);	\
    }							\
    *(data)++ = (x2);					\
    *(data)++ = (y2);					\
    *(data)++ = 0.0;					\
    for (it = 0; it < n; it++)				\
    {							\
	*(data)++ = COMP_TEX_COORD_XY (&m[it], x2, y1);	\
	*(data)++ = COMP_TEX_COORD_YX (&m[it], x2, y1);	\
    }							\
    *(data)++ = (x2);					\
    *(data)++ = (y1);					\
    *(data)++ = 0.0;


bool
CompWindow::moreVertices (int newSize)
{
    if (newSize > priv->vertexSize)
    {
	GLfloat *vertices;

	vertices = (GLfloat *)
	    realloc (priv->vertices, sizeof (GLfloat) * newSize);
	if (!vertices)
	    return false;

	priv->vertices = vertices;
	priv->vertexSize = newSize;
    }

    return true;
}

bool
CompWindow::moreIndices (int newSize)
{
    if (newSize > priv->indexSize)
    {
	GLushort *indices;

	indices = (GLushort *)
	    realloc (priv->indices, sizeof (GLushort) * newSize);
	if (!indices)
	    return false;

	priv->indices = indices;
	priv->indexSize = newSize;
    }

    return true;
}

void
CompWindow::drawGeometry ()
{
    WRAPABLE_HND_FUNC(drawGeometry)

    int     texUnit = priv->texUnits;
    int     currentTexUnit = 0;
    int     stride = priv->vertexStride;
    GLfloat *vertices = priv->vertices + (stride - 3);

    stride *= sizeof (GLfloat);

    glVertexPointer (3, GL_FLOAT, stride, vertices);

    while (texUnit--)
    {
	if (texUnit != currentTexUnit)
	{
	    (*priv->screen->clientActiveTexture) (GL_TEXTURE0_ARB + texUnit);
	    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	    currentTexUnit = texUnit;
	}
	vertices -= priv->texCoordSize;
	glTexCoordPointer (priv->texCoordSize, GL_FLOAT, stride, vertices);
    }

    glDrawArrays (GL_QUADS, 0, priv->vCount);

    /* disable all texture coordinate arrays except 0 */
    texUnit = priv->texUnits;
    if (texUnit > 1)
    {
	while (--texUnit)
	{
	    (*priv->screen->clientActiveTexture) (GL_TEXTURE0_ARB + texUnit);
	    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	(*priv->screen->clientActiveTexture) (GL_TEXTURE0_ARB);
    }
}

void
CompWindow::addGeometry (CompMatrix *matrix,
			 int        nMatrix,
			 Region     region,
			 Region     clip)
{
    WRAPABLE_HND_FUNC(addGeometry, matrix, nMatrix, region, clip)

    BoxRec full;

    priv->texUnits = nMatrix;

    full = clip->extents;
    if (region->extents.x1 > full.x1)
	full.x1 = region->extents.x1;
    if (region->extents.y1 > full.y1)
	full.y1 = region->extents.y1;
    if (region->extents.x2 < full.x2)
	full.x2 = region->extents.x2;
    if (region->extents.y2 < full.y2)
	full.y2 = region->extents.y2;

    if (full.x1 < full.x2 && full.y1 < full.y2)
    {
	BoxPtr  pBox;
	int     nBox;
	BoxPtr  pClip;
	int     nClip;
	BoxRec  cbox;
	int     vSize;
	int     n, it, x1, y1, x2, y2;
	GLfloat *d;
	bool    rect = true;

	for (it = 0; it < nMatrix; it++)
	{
	    if (matrix[it].xy != 0.0f || matrix[it].yx != 0.0f)
	    {
		rect = false;
		break;
	    }
	}

	pBox = region->rects;
	nBox = region->numRects;

	vSize = 3 + nMatrix * 2;

	n = priv->vCount / 4;

	if ((n + nBox) * vSize * 4 > priv->vertexSize)
	{
	    if (!moreVertices ((n + nBox) * vSize * 4))
		return;
	}

	d = priv->vertices + (priv->vCount * vSize);

	while (nBox--)
	{
	    x1 = pBox->x1;
	    y1 = pBox->y1;
	    x2 = pBox->x2;
	    y2 = pBox->y2;

	    pBox++;

	    if (x1 < full.x1)
		x1 = full.x1;
	    if (y1 < full.y1)
		y1 = full.y1;
	    if (x2 > full.x2)
		x2 = full.x2;
	    if (y2 > full.y2)
		y2 = full.y2;

	    if (x1 < x2 && y1 < y2)
	    {
		nClip = clip->numRects;

		if (nClip == 1)
		{
		    if (rect)
		    {
			ADD_RECT (d, matrix, nMatrix, x1, y1, x2, y2);
		    }
		    else
		    {
			ADD_QUAD (d, matrix, nMatrix, x1, y1, x2, y2);
		    }

		    n++;
		}
		else
		{
		    pClip = clip->rects;

		    if (((n + nClip) * vSize * 4) > priv->vertexSize)
		    {
			if (!moreVertices ((n + nClip) * vSize * 4))
			    return;

			d = priv->vertices + (n * vSize * 4);
		    }

		    while (nClip--)
		    {
			cbox = *pClip;

			pClip++;

			if (cbox.x1 < x1)
			    cbox.x1 = x1;
			if (cbox.y1 < y1)
			    cbox.y1 = y1;
			if (cbox.x2 > x2)
			    cbox.x2 = x2;
			if (cbox.y2 > y2)
			    cbox.y2 = y2;

			if (cbox.x1 < cbox.x2 && cbox.y1 < cbox.y2)
			{
			    if (rect)
			    {
				ADD_RECT (d, matrix, nMatrix,
					  cbox.x1, cbox.y1, cbox.x2, cbox.y2);
			    }
			    else
			    {
				ADD_QUAD (d, matrix, nMatrix,
					  cbox.x1, cbox.y1, cbox.x2, cbox.y2);
			    }

			    n++;
			}
		    }
		}
	    }
	}

	priv->vCount	   = n * 4;
	priv->vertexStride = vSize;
	priv->texCoordSize = 2;
    }
}

static bool
enableFragmentProgramAndDrawGeometry (CompWindow	   *w,
				      CompTexture	   *texture,
				      const FragmentAttrib *attrib,
				      int		   filter,
				      unsigned int	   mask)
{
    FragmentAttrib fa = *attrib;
    CompScreen     *s = w->screen ();
    Bool	   blending;

    if (s->canDoSaturated () && attrib->saturation != COLOR)
    {
	int param, function;

	param    = allocFragmentParameters (&fa, 1);
	function = getSaturateFragmentFunction (s, texture, param);

	addFragmentFunction (&fa, function);

	(*s->programEnvParameter4f) (GL_FRAGMENT_PROGRAM_ARB, param,
				     RED_SATURATION_WEIGHT,
				     GREEN_SATURATION_WEIGHT,
				     BLUE_SATURATION_WEIGHT,
				     attrib->saturation / 65535.0f);
    }

    if (!enableFragmentAttrib (s, &fa, &blending))
	return false;

    s->enableTexture (texture, filter);

    if (mask & PAINT_WINDOW_BLEND_MASK)
    {
	if (blending)
	    glEnable (GL_BLEND);

	if (attrib->opacity != OPAQUE || attrib->brightness != BRIGHT)
	{
	    GLushort color;

	    color = (attrib->opacity * attrib->brightness) >> 16;

	    s->setTexEnvMode (GL_MODULATE);
	    glColor4us (color, color, color, attrib->opacity);

	    w->drawGeometry ();

	    glColor4usv (defaultColor);
	    s->setTexEnvMode (GL_REPLACE);
	}
	else
	{
	    w->drawGeometry ();
	}

	if (blending)
	    glDisable (GL_BLEND);
    }
    else if (attrib->brightness != BRIGHT)
    {
	s->setTexEnvMode (GL_MODULATE);
	glColor4us (attrib->brightness, attrib->brightness,
		    attrib->brightness, BRIGHT);

	w->drawGeometry ();

	glColor4usv (defaultColor);
	s->setTexEnvMode (GL_REPLACE);
    }
    else
    {
	w->drawGeometry ();
    }

    s->disableTexture (texture);

    disableFragmentAttrib (s, &fa);

    return true;
}

static void
enableFragmentOperationsAndDrawGeometry (CompWindow	      *w,
					 CompTexture	      *texture,
					 const FragmentAttrib *attrib,
					 int		      filter,
					 unsigned int	      mask)
{
    CompScreen *s = w->screen ();

    if (s->canDoSaturated () && attrib->saturation != COLOR)
    {
	GLfloat constant[4];

	if (mask & PAINT_WINDOW_BLEND_MASK)
	    glEnable (GL_BLEND);

	s->enableTexture (texture, filter);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	glColor4f (1.0f, 1.0f, 1.0f, 0.5f);

	s->activeTexture (GL_TEXTURE1_ARB);

	s->enableTexture (texture, filter);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	if (s->canDoSlightlySaturated () && attrib->saturation > 0)
	{
	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	    constant[0] = 0.5f + 0.5f * RED_SATURATION_WEIGHT;
	    constant[1] = 0.5f + 0.5f * GREEN_SATURATION_WEIGHT;
	    constant[2] = 0.5f + 0.5f * BLUE_SATURATION_WEIGHT;
	    constant[3] = 1.0;

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    s->activeTexture (GL_TEXTURE2_ARB);

	    s->enableTexture (texture, filter);

	    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	    constant[3] = attrib->saturation / 65535.0f;

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    if (attrib->opacity < OPAQUE || attrib->brightness != BRIGHT)
	    {
		s->activeTexture (GL_TEXTURE3_ARB);

		s->enableTexture (texture, filter);

		constant[3] = attrib->opacity / 65535.0f;
		constant[0] = constant[1] = constant[2] = constant[3] *
		    attrib->brightness / 65535.0f;

		glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

		w->drawGeometry ();

		s->disableTexture (texture);

		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		s->activeTexture (GL_TEXTURE2_ARB);
	    }
	    else
	    {
		w->drawGeometry ();
	    }

	    s->disableTexture (texture);

	    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	    s->activeTexture (GL_TEXTURE1_ARB);
	}
	else
	{
	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

	    constant[3] = attrib->opacity / 65535.0f;
	    constant[0] = constant[1] = constant[2] = constant[3] *
		attrib->brightness / 65535.0f;

	    constant[0] = 0.5f + 0.5f * RED_SATURATION_WEIGHT   * constant[0];
	    constant[1] = 0.5f + 0.5f * GREEN_SATURATION_WEIGHT * constant[1];
	    constant[2] = 0.5f + 0.5f * BLUE_SATURATION_WEIGHT  * constant[2];

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    w->drawGeometry ();
	}

	s->disableTexture (texture);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	s->activeTexture (GL_TEXTURE0_ARB);

	s->disableTexture (texture);

	glColor4usv (defaultColor);
	s->setTexEnvMode (GL_REPLACE);

	if (mask & PAINT_WINDOW_BLEND_MASK)
	    glDisable (GL_BLEND);
    }
    else
    {
	s->enableTexture (texture, filter);

	if (mask & PAINT_WINDOW_BLEND_MASK)
	{
	    glEnable (GL_BLEND);
	    if (attrib->opacity != OPAQUE || attrib->brightness != BRIGHT)
	    {
		GLushort color;

		color = (attrib->opacity * attrib->brightness) >> 16;

		s->setTexEnvMode (GL_MODULATE);
		glColor4us (color, color, color, attrib->opacity);

		w->drawGeometry ();

		glColor4usv (defaultColor);
		s->setTexEnvMode (GL_REPLACE);
	    }
	    else
	    {
		w->drawGeometry ();
	    }

	    glDisable (GL_BLEND);
	}
	else if (attrib->brightness != BRIGHT)
	{
	    s->setTexEnvMode (GL_MODULATE);
	    glColor4us (attrib->brightness, attrib->brightness,
			attrib->brightness, BRIGHT);

	    w->drawGeometry ();

	    glColor4usv (defaultColor);
	    s->setTexEnvMode (GL_REPLACE);
	}
	else
	{
	    w->drawGeometry ();
	}

	s->disableTexture (texture);
    }
}

void
CompWindow::drawTexture (CompTexture		*texture,
			 const FragmentAttrib	*attrib,
			 unsigned int		mask)
{
    WRAPABLE_HND_FUNC(drawTexture, texture, attrib, mask)

    int filter;

    if (mask & (PAINT_WINDOW_TRANSFORMED_MASK |
		PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK))
	filter = priv->screen->filter (SCREEN_TRANS_FILTER);
    else
	filter = priv->screen->filter (NOTHING_TRANS_FILTER);

    if ((!attrib->nFunction && (!priv->screen->lighting () ||
	 attrib->saturation == COLOR || attrib->saturation == 0)) ||
	!enableFragmentProgramAndDrawGeometry (this,
					       texture,
					       attrib,
					       filter,
					       mask))
    {
	enableFragmentOperationsAndDrawGeometry (this,
						 texture,
						 attrib,
						 filter,
						 mask);
    }
}

bool
CompWindow::draw (const CompTransform  *transform,
		  const FragmentAttrib *fragment,
		  Region               region,
		  unsigned int         mask)
{
    WRAPABLE_HND_FUNC_RETURN(bool, draw, transform, fragment, region, mask)

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	region = &infiniteRegion;

    if (!region->numRects)
	return true;

    if (priv->attrib.map_state != IsViewable)
	return true;

    if (!priv->texture->pixmap && !bind ())
	return false;

    if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	mask |= PAINT_WINDOW_BLEND_MASK;

    priv->vCount = priv->indexCount = 0;
    addGeometry (&priv->matrix, 1, priv->region, region);
    if (priv->vCount)
	drawTexture (priv->texture, fragment, mask);

    return true;
}

bool
CompWindow::paint (const WindowPaintAttrib *attrib,
		   const CompTransform     *transform,
		   Region                  region,
		   unsigned int            mask)
{
    WRAPABLE_HND_FUNC_RETURN(bool, paint, attrib, transform, region, mask)

    FragmentAttrib fragment;
    Bool	   status;

    priv->lastPaint = *attrib;

    if (priv->alpha || attrib->opacity != OPAQUE)
	mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

    priv->lastMask = mask;

    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
    {
	if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	    return false;

	if (mask & PAINT_WINDOW_NO_CORE_INSTANCE_MASK)
	    return false;

	if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	    return false;

	if (priv->shaded)
	    return false;

	return true;
    }

    if (mask & PAINT_WINDOW_NO_CORE_INSTANCE_MASK)
	return true;

    initFragmentAttrib (&fragment, attrib);

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK ||
        mask & PAINT_WINDOW_WITH_OFFSET_MASK)
    {
	glPushMatrix ();
	glLoadMatrixf (transform->m);
    }

    status = draw (transform, &fragment, region, mask);

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK ||
        mask & PAINT_WINDOW_WITH_OFFSET_MASK)
	glPopMatrix ();

    return status;
}
