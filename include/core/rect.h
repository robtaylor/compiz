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
