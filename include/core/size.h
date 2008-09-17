#ifndef _COMPSIZE_H
#define _COMPSIZE_H

#include <vector>
#include <list>

class CompSize {

    public:
	CompSize ();
	CompSize (unsigned int, unsigned int);

	unsigned int width () const;
	unsigned int height () const;

	void setWidth (unsigned int);
	void setHeight (unsigned int);

	typedef std::vector<CompSize> vector;
	typedef std::vector<CompSize *> ptrVector;
	typedef std::list<CompSize> list;
	typedef std::list<CompSize *> ptrList;

    private:
	unsigned int mWidth, mHeight;
};

inline unsigned int
CompSize::width () const
{
    return mWidth;
}

inline unsigned int
CompSize::height () const
{
    return mHeight;
}

#endif
