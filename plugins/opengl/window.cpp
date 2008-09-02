#include "privates.h"

GLWindow::GLWindow (CompWindow *w) :
    OpenGLPrivateHandler<GLWindow, CompWindow, COMPIZ_OPENGL_ABI> (w),
    priv (new PrivateGLWindow (w, this))
{
    priv->clip = XCreateRegion ();
    assert (priv->clip);

    CompositeWindow *cw = CompositeWindow::get (w);
	
    priv->paint.opacity    = cw->opacity ();
    priv->paint.brightness = cw->brightness ();
    priv->paint.saturation = cw->saturation ();

    priv->lastPaint = priv->paint;

}

GLWindow::~GLWindow ()
{
    delete priv;
}

PrivateGLWindow::PrivateGLWindow (CompWindow *w,
				  GLWindow   *gw) :
    window (w),
    gWindow (gw),
    cWindow (CompositeWindow::get (w)),
    screen (w->screen ()),
    gScreen (GLScreen::get (screen)),
    texture (screen),
    clip (0),
    bindFailed (false),
    geometry ()
{
    paint.xScale	= 1.0f;
    paint.yScale	= 1.0f;
    paint.xTranslate	= 0.0f;
    paint.yTranslate	= 0.0f;

    WindowInterface::setHandler (w);
    CompositeWindowInterface::setHandler (cWindow);
}

PrivateGLWindow::~PrivateGLWindow ()
{
    if (clip)
	XDestroyRegion (clip);
}

void
PrivateGLWindow::setWindowMatrix ()
{
    matrix = texture.matrix ();
    matrix.x0 -= ((window->attrib ().x - window->input ().left) * matrix.xx);
    matrix.y0 -= ((window->attrib ().y - window->input ().top) * matrix.yy);
}

bool
GLWindow::bind ()
{
    CompWindowExtents i = priv->window->input ();

    if (!priv->cWindow->pixmap () && !priv->cWindow->bind ())
	return false;

    if (!priv->texture.bindPixmap (priv->cWindow->pixmap (),
				   priv->window->width () + i.left + i.right,
				   priv->window->height () + i.top + i.bottom,
				   priv->window->attrib ().depth))
    {
	compLogMessage (priv->screen->display (), "opengl", CompLogLevelInfo,
			"Couldn't bind redirected window 0x%x to "
			"texture\n", (int) priv->window->id ());
    }

    priv->setWindowMatrix ();

    return true;
}

void
GLWindow::release ()
{
    if (priv->cWindow->pixmap ())
    {
	priv->texture = GLTexture (priv->screen);
	priv->cWindow->release ();
    }
}

bool
GLWindowInterface::glPaint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix            &transform,
			    Region                    region,
			    unsigned int              mask)
    WRAPABLE_DEF (glPaint, attrib, transform, region, mask)

bool
GLWindowInterface::glDraw (const GLMatrix     &transform,
			   GLFragment::Attrib &fragment,
			   Region             region,
			   unsigned int       mask)
    WRAPABLE_DEF (glDraw, transform, fragment, region, mask)

void
GLWindowInterface::glAddGeometry (GLTexture::Matrix *matrix,
				  int	            nMatrix,
				  Region	    region,
				  Region	    clip)
    WRAPABLE_DEF (glAddGeometry, matrix, nMatrix, region, clip)

void
GLWindowInterface::glDrawTexture (GLTexture          *texture,
				  GLFragment::Attrib &fragment,
				  unsigned int       mask)
    WRAPABLE_DEF (glDrawTexture, texture, fragment, mask)

void
GLWindowInterface::glDrawGeometry ()
    WRAPABLE_DEF (glDrawGeometry)

Region
GLWindow::clip ()
{
    return priv->clip;
}

GLWindowPaintAttrib &
GLWindow::paintAttrib ()
{
    return priv->paint;
}

void
PrivateGLWindow::resizeNotify (int dx, int dy, int dwidth, int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);
    setWindowMatrix ();
    gWindow->release ();
}

void
PrivateGLWindow::moveNotify (int dx, int dy, bool now)
{
    window->moveNotify (dx, dy, now);
    setWindowMatrix ();
}

void
PrivateGLWindow::windowNotify (CompWindowNotify n)
{
    switch (n)
    {
	case CompWindowNotifyUnmap:
	case CompWindowNotifyReparent:
	case CompWindowNotifyUnreparent:
	case CompWindowNotifyFrameUpdate:
	    gWindow->release ();
	    break;
	case CompWindowNotifyAliveChanged:
	    gWindow->updatePaintAttribs ();
	    break;
	default:
	    break;
	
    }

    window->windowNotify (n);
}

void
GLWindow::updatePaintAttribs ()
{
    CompositeWindow *cw = CompositeWindow::get (priv->window);

    if (priv->window->alive ())
    {
	priv->paint.opacity    = cw->opacity ();
	priv->paint.brightness = cw->brightness ();
	priv->paint.saturation = cw->saturation ();
    }
    else
    {
	priv->paint.opacity    = cw->opacity ();
	priv->paint.brightness = 0xa8a8;
	priv->paint.saturation = 0;
    }
}

bool
PrivateGLWindow::damageRect (bool initial, BoxPtr box)
{
    texture.damage ();
    return cWindow->damageRect (initial, box);
}

GLWindow::Geometry &
GLWindow::geometry ()
{
    return priv->geometry;
}

GLWindow::Geometry::Geometry () :
    vertices (NULL),
    vertexSize (0),
    vertexStride (0),
    indices (NULL),
    indexSize (0),
    vCount (0),
    texUnits (0),
    texCoordSize (0),
    indexCount (0)
{
}

GLWindow::Geometry::~Geometry ()
{
    if (vertices)
	free (vertices);

    if (indices)
	free (indices);
}

void
GLWindow::Geometry::reset ()
{
    vCount = indexCount = 0;
}

bool
GLWindow::Geometry::moreVertices (int newSize)
{
    if (newSize > vertexSize)
    {
	GLfloat *nVertices;

	nVertices = (GLfloat *)
	    realloc (vertices, sizeof (GLfloat) * newSize);
	if (!nVertices)
	    return false;

	vertices = nVertices;
	vertexSize = newSize;
    }

    return true;
}

bool
GLWindow::Geometry::moreIndices (int newSize)
{
    if (newSize > indexSize)
    {
	GLushort *nIndices;

	nIndices = (GLushort *)
	    realloc (indices, sizeof (GLushort) * newSize);
	if (!nIndices)
	    return false;

	indices = nIndices;
	indexSize = newSize;
    }

    return true;
}
