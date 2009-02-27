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

#ifndef _COMPREGION_H
#define _COMPREGION_H

#include <X11/Xutil.h>
#include <X11/Xregion.h>

#include <core/rect.h>
#include <core/point.h>

class PrivateRegion;

class CompRegion {
    public:
	typedef std::vector<CompRegion> List;
	typedef std::vector<CompRegion *> PtrList;
	typedef std::vector<CompRegion> Vector;
	typedef std::vector<CompRegion *> PtrVector;

    public:
	CompRegion ();
	CompRegion (const CompRegion &);
	CompRegion (int x, int y, int w, int h);
        CompRegion (const CompRect &);
	~CompRegion ();

	CompRect boundingRect () const;

	bool isEmpty () const;
	int numRects () const;
	CompRect::vector rects () const;
	const Region handle () const;

	bool contains (const CompPoint &) const;
	bool contains (const CompRect &) const;

	CompRegion intersected (const CompRegion &) const;
	CompRegion intersected (const CompRect &) const;
	bool intersects (const CompRegion &) const;
	bool intersects (const CompRect &) const;
	CompRegion subtracted (const CompRegion &) const;
	CompRegion subtracted (const CompRect &) const;
	void translate (int, int);
	void translate (const CompPoint &);
	CompRegion translated (int, int) const;
	CompRegion translated (const CompPoint &) const;
	void shrink (int, int);
	void shrink (const CompPoint &);
	CompRegion shrinked (int, int) const;
	CompRegion shrinked (const CompPoint &) const;
	CompRegion united (const CompRegion &) const;
	CompRegion united (const CompRect &) const;
	CompRegion xored (const CompRegion &) const;
	
	bool operator== (const CompRegion &) const;
	bool operator!= (const CompRegion &) const;
	const CompRegion operator& (const CompRegion &) const;
	const CompRegion operator& (const CompRect &) const;
	CompRegion & operator&= (const CompRegion &);
	CompRegion & operator&= (const CompRect &);
	const CompRegion operator+ (const CompRegion &) const;
	const CompRegion operator+ (const CompRect &) const;
	CompRegion & operator+= (const CompRegion &);
	CompRegion & operator+= (const CompRect &);
	const CompRegion operator- (const CompRegion &) const;
	const CompRegion operator- (const CompRect &) const;
	CompRegion & operator-= (const CompRegion &);
	CompRegion & operator-= (const CompRect &);
	CompRegion & operator= (const CompRegion &);
	
	const CompRegion operator^ (const CompRegion &) const;
	CompRegion & operator^= (const CompRegion &);
	const CompRegion operator| (const CompRegion &) const;
	CompRegion & operator|= (const CompRegion &);
	
    private:
	PrivateRegion *priv;
};

extern const CompRegion infiniteRegion;
extern const CompRegion emptyRegion;

#endif
