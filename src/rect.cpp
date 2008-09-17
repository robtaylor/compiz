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

Region
CompRect::region ()
{
    return &mRegion;
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

