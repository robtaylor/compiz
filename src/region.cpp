#include <stdio.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xregion.h>

#include <core/core.h>

#include "privateregion.h"

CompRegion::CompRegion ()
{
    priv = new PrivateRegion ();
}

CompRegion::CompRegion (const CompRegion &c)
{
    priv = new PrivateRegion ();
    priv->box = c.priv->box;
    if (priv->box.rects)
	priv->box.rects = &priv->box.extents;
    if (c.priv->region)
    {
	priv->region = XCreateRegion ();
	XUnionRegion (&emptyRegion, c.priv->region, priv->region);
    }
}

CompRegion::CompRegion ( int x, int y, int w, int h)
{
    priv = new PrivateRegion ();
    priv->box.extents = CompRect (x, x + w, y, y + h).region ()->extents;
    priv->box.numRects = 1;
    priv->box.rects = &priv->box.extents;
}

CompRegion::CompRegion (const CompRect &r)
{
    priv = new PrivateRegion ();
    priv->box.extents = r.region ()->extents;
    priv->box.numRects = 1;
    priv->box.rects = &priv->box.extents;
}

CompRegion::~CompRegion ()
{
    delete priv;
}

const Region
CompRegion::handle () const
{
    return (priv->region)? priv->region : &priv->box;
}

CompRegion &
CompRegion::operator= (const CompRegion &c)
{
    priv->box = c.priv->box;
    if (priv->box.rects)
	priv->box.rects = &priv->box.extents;
    if (c.priv->region)
    {
	if (!priv->region)
	    priv->region = XCreateRegion ();
	XUnionRegion (&emptyRegion, c.priv->region, priv->region);
    }
    return *this;
}

bool
CompRegion::operator== (const CompRegion &c) const
{
    return XEqualRegion(handle (), c.handle ());
}

bool
CompRegion::operator!= (const CompRegion &c) const
{
    return *this == c;
}

CompRect
CompRegion::boundingRect () const
{
    BOX b = handle ()->extents;
    return CompRect (b.x1, b.x2, b.y1, b.y2);
}

bool
CompRegion::contains (const CompPoint &p) const
{
    return XPointInRegion (handle (), p.x (), p.y ());
}

bool
CompRegion::contains (const CompRect &r) const
{
    return XRectInRegion (handle (), r.x (), r.y (), r.width (), r.height ())
	   != RectangleOut;
}

CompRegion
CompRegion::intersected (const CompRegion &r) const
{
    CompRegion reg (r);
    reg.priv->makeReal ();
    XIntersectRegion (reg.handle (), handle (), reg.handle ());
    return reg;
}

CompRegion
CompRegion::intersected (const CompRect &r) const
{
    CompRegion reg (r);
    reg.priv->makeReal ();
    XIntersectRegion (reg.handle (), handle (), reg.handle ());
    return reg;
}

bool
CompRegion::intersects (const CompRegion &r) const
{
    return !intersected (r).isEmpty ();
}

bool
CompRegion::intersects (const CompRect &r) const
{
    return !intersected (r).isEmpty ();
}

bool
CompRegion::isEmpty () const
{
    return XEmptyRegion (handle ());
}

int
CompRegion::numRects () const
{
    return handle ()->numRects;
}

CompRect::vector
CompRegion::rects () const
{
    CompRect::vector rv;
    if (!numRects ())
	return rv;
    if (!priv->region)
    {
	rv.push_back (CompRect (priv->box.extents.x1, priv->box.extents.x2,
		                priv->box.extents.y1, priv->box.extents.y2));
	return rv;
    }

    BOX b;
    for (int i = 0; i < priv->region->numRects; i++)
    {
	b = priv->region->rects[i];
	rv.push_back (CompRect (b.x1, b.x2, b.y1, b.y2));
    }
    return rv;
}
	
CompRegion
CompRegion::subtracted (const CompRegion &r) const
{
    CompRegion rv;
    rv.priv->makeReal ();
    XSubtractRegion (handle (), r.handle (), rv.handle ());
    return rv;
}

void
CompRegion::translate (int dx, int dy)
{
    priv->makeReal ();
    XOffsetRegion (handle (), dx, dy);
}

void
CompRegion::translate (const CompPoint &p)
{
    translate (p.x (), p.y ());
}

CompRegion
CompRegion::translated (int dx, int dy) const
{
    CompRegion rv (*this);
    rv.translate (dx, dy);
    return rv;
}

CompRegion
CompRegion::translated (const CompPoint &p) const
{
    CompRegion rv (*this);
    rv.translate (p);
    return rv;
}

CompRegion
CompRegion::united (const CompRegion &r) const
{
    CompRegion rv;
    rv.priv->makeReal ();
    XUnionRegion (handle (), r.handle (), rv.handle ());
    return rv;
}

CompRegion
CompRegion::united (const CompRect &r) const
{
    CompRegion rv;
    rv.priv->makeReal ();
    XUnionRegion (handle (), r.region (), rv.handle ());
    return rv;
}

CompRegion
CompRegion::xored (const CompRegion &r) const
{
    CompRegion rv;
    rv.priv->makeReal ();
    XXorRegion (handle (), r.handle (), rv.handle ());
    return rv;
}
	

const CompRegion
CompRegion::operator& (const CompRegion &r) const
{
    return intersected (r);
}

const CompRegion
CompRegion::operator& (const CompRect &r) const
{
    return intersected (r);
}

CompRegion &
CompRegion::operator&= (const CompRegion &r)
{
    return *this = *this & r;
}

CompRegion &
CompRegion::operator&= (const CompRect &r)
{
    return *this = *this & r;
}

const CompRegion
CompRegion::operator+ (const CompRegion &r) const
{
    return united (r);
}

const CompRegion
CompRegion::operator+ (const CompRect &r) const
{
    return united (r);
}

CompRegion &
CompRegion::operator+= (const CompRegion &r)
{
    return *this = *this + r;
}

CompRegion &
CompRegion::operator+= (const CompRect &r)
{
    return *this = *this + r;
}

const CompRegion
CompRegion::operator- (const CompRegion &r) const
{
    return subtracted (r);
}

CompRegion &
CompRegion::operator-= (const CompRegion &r)
{
    return *this = *this - r;
}

const CompRegion
CompRegion::operator^ (const CompRegion &r) const
{
    return xored (r);
}

CompRegion &
CompRegion::operator^= (const CompRegion &r)
{
    return *this = *this ^ r;
}

const CompRegion
CompRegion::operator| (const CompRegion &r) const
{
    united (r);
}

CompRegion &
CompRegion::operator|= (const CompRegion &r)
{
    return *this = *this | r;
}


PrivateRegion::PrivateRegion ()
{
    region = NULL;
    box = emptyRegion;
}

PrivateRegion::~PrivateRegion ()
{
    if (region)
	XDestroyRegion (region);
}

void
PrivateRegion::makeReal ()
{
    if (region)
	return;
    region = XCreateRegion ();
    if (box.numRects)
	XUnionRegion (&emptyRegion, &box, region);
}

