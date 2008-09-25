#ifndef _COMPREGION_H
#define _COMPREGION_H

#include <X11/Xutil.h>
#include <X11/Xregion.h>

#include <core/rect.h>
#include <core/point.h>

class PrivateRegion;

class CompRegion {
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
	void translate (int, int);
	void translate (const CompPoint &);
	CompRegion translated (int, int) const;
	CompRegion translated (const CompPoint &) const;
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
	CompRegion & operator-= (const CompRegion &);
	CompRegion & operator= (const CompRegion &);
	
	const CompRegion operator^ (const CompRegion &) const;
	CompRegion & operator^= (const CompRegion &);
	const CompRegion operator| (const CompRegion &) const;
	CompRegion & operator|= (const CompRegion &);
	
    private:
	PrivateRegion *priv;
};

#endif