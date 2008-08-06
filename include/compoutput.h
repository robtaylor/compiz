#ifndef _COMPOUTPUT_H
#define _COMPOUTPUT_H

#include <comprect.h>

class CompOutput : public CompRect {

    public:
	CompOutput ();

	CompString name ();
	
	unsigned int id ();

	XRectangle workArea ();

	void setWorkArea (XRectangle);
	void setGeometry (int, int, int, int);
	void setId (CompString, unsigned int);

	typedef std::vector<CompOutput> vector;
	typedef std::vector<CompOutput *> ptrVector;
	typedef std::list<CompOutput *> ptrList;

    private:

	CompString   mName;
	unsigned int mId;

	XRectangle   mWorkArea;
};

#endif
