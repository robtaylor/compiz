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

#ifndef _COMPRECT_H
#define _COMPRECT_H

class CompRect {

    public:
	CompRect ();
	CompRect (int, int, int, int);
	CompRect (const CompRect&);

	int x () const;
	int y () const;

	int x1 () const;
	int y1 () const;
	int x2 () const;
	int y2 () const;
	unsigned int width () const;
	unsigned int height () const;

	const Region region () const;

	void setGeometry (int, int, int, int);

	bool operator== (const CompRect &) const;
	bool operator!= (const CompRect &) const;

	typedef std::vector<CompRect> vector;
	typedef std::vector<CompRect *> ptrVector;
	typedef std::list<CompRect *> ptrList;

    private:
	REGION       mRegion;
};

inline int
CompRect::x () const
{
    return mRegion.extents.x1;
}

inline int
CompRect::y () const
{
    return mRegion.extents.y1;
}

inline int
CompRect::x1 () const
{
    return mRegion.extents.x1;
}

inline int
CompRect::y1 () const
{
    return mRegion.extents.y1;
}

inline int
CompRect::x2 () const
{
    return mRegion.extents.x2;
}

inline int
CompRect::y2 () const
{
    return mRegion.extents.y2;
}

inline unsigned int
CompRect::width () const
{
    return mRegion.extents.x2 - mRegion.extents.x1;
}

inline unsigned int
CompRect::height () const
{
    return mRegion.extents.y2 - mRegion.extents.y1;
}

#endif
