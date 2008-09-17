#ifndef _COMPPOINT_H
#define _COMPPOINT_H

#include <vector>
#include <list>

class CompPoint {

    public:
	CompPoint ();
	CompPoint (int, int);

	int x () const;
	int y () const;

	void setX (int);
	void setY (int);

	typedef std::vector<CompPoint> vector;
	typedef std::vector<CompPoint *> ptrVector;
	typedef std::list<CompPoint> list;
	typedef std::list<CompPoint *> ptrList;

    private:
	int mX, mY;
};

inline int
CompPoint::x () const
{
    return mX;
}

inline int
CompPoint::y () const
{
    return mY;
}


#endif
