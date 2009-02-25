/*
 * Copyright © 2008 Dennis Kasprzyk
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
 */

#include <core/core.h>
#include <core/rect.h>

CompRect::CompRect ()
{
    mRegion.rects = &mRegion.extents;
    mRegion.numRects = 1;
    mRegion.extents.x1 = 0;
    mRegion.extents.x2 = 0;
    mRegion.extents.y1 = 0;
    mRegion.extents.y2 = 0;
}

CompRect::CompRect (int x1, int x2, int y1, int y2)
{
    mRegion.rects = &mRegion.extents;
    mRegion.numRects = 1;
    mRegion.extents.x1 = x1;
    mRegion.extents.x2 = x2;
    mRegion.extents.y1 = y1;
    mRegion.extents.y2 = y2;
}

CompRect::CompRect (const CompRect& r)
{
    mRegion = r.mRegion;
    mRegion.rects = &mRegion.extents;
}

CompRect::CompRect (const XRectangle xr)
{
    mRegion.rects = &mRegion.extents;
    mRegion.numRects = 1;
    mRegion.extents.x1 = xr.x;
    mRegion.extents.x2 = xr.x + xr.width;
    mRegion.extents.y1 = xr.y;
    mRegion.extents.y2 = xr.y + xr.height;
}

const Region
CompRect::region () const
{
    return const_cast<const Region> (&mRegion);
}

void
CompRect::setGeometry (int x1, int x2, int y1, int y2)
{
    mRegion.extents.x1 = x1;
    mRegion.extents.y1 = y1;

    if (x2 < x1)
	mRegion.extents.x2 = x1;
    else
	mRegion.extents.x2 = x2;

    if (y2 < y1)
	mRegion.extents.y2 = y1;
    else
	mRegion.extents.y2 = y2;
}

bool
CompRect::contains (const CompPoint& point) const
{
    if (point.x () < x1 ())
	return false;
    if (point.x () > x2 ())
	return false;
    if (point.y () < y1 ())
	return false;
    if (point.y () > y2 ())
	return false;

    return true;
}

bool
CompRect::operator== (const CompRect &rect) const
{
    if (mRegion.extents.x1 != rect.mRegion.extents.x1)
	return false;
    if (mRegion.extents.y1 != rect.mRegion.extents.y1)
	return false;
    if (mRegion.extents.x2 != rect.mRegion.extents.x2)
	return false;
    if (mRegion.extents.y2 != rect.mRegion.extents.y2)
	return false;

    return true;
}

bool
CompRect::operator!= (const CompRect &rect) const
{
    return !(*this == rect);
}
