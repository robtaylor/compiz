#ifndef _COMPOUTPUT_H
#define _COMPOUTPUT_H

class CompOutput {

    public:
	CompOutput ();

	CompString name ();
	
	unsigned int id ();

	unsigned int x1 ();
	unsigned int y1 ();
	unsigned int x2 ();
	unsigned int y2 ();
	unsigned int width ();
	unsigned int height ();

	Region region ();

	XRectangle workArea ();

	void setWorkArea (XRectangle);
	void setGeometry (unsigned int, unsigned int,
			  unsigned int, unsigned int);
	void setId (CompString, unsigned int);

	typedef std::vector<CompOutput> vector;
	typedef std::vector<CompOutput *> ptrVector;
	typedef std::list<CompOutput *> ptrList;

    private:

	CompString   mName;
	unsigned int mId;
	REGION       mRegion;

	XRectangle   mWorkArea;
};

#endif
