#include <compiz-core.h>
#include <core/window.h>
#include <privatewindow.h>


CompWindow::Geometry::Geometry () :
    mBorder (0)
{
}

CompWindow::Geometry::Geometry (int x, int y,
				unsigned int width,
				unsigned int height,
				unsigned int border) :
    CompPoint (x, y),
    CompSize (width, height),
    mBorder (border)
{
}

unsigned int
CompWindow::Geometry::border ()
{
    return mBorder;
}

void
CompWindow::Geometry::setBorder (unsigned int border)
{
    mBorder = border;
}

void
CompWindow::Geometry::set (int x, int y,
			   unsigned int width,
			   unsigned int height,
			   unsigned int border)
{
    setX (x);
    setY (y);
    setWidth (width);
    setHeight (height);
    mBorder = border;
}

CompWindow::Geometry &
CompWindow::serverGeometry ()
{
    return priv->serverGeometry;
}

CompWindow::Geometry &
CompWindow::geometry ()
{
    return priv->geometry;
}
