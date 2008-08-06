#ifndef _COMPRECT_H
#define _COMPRECT_H

class CompRect {

    public:
	CompRect ();
	CompRect (int, int, int, int);

	int x ();
	int y ();

	int x1 ();
	int y1 ();
	int x2 ();
	int y2 ();
	unsigned int width ();
	unsigned int height ();

	Region region ();

	void setGeometry (int, int, int, int);

	typedef std::vector<CompRect> vector;
	typedef std::vector<CompRect *> ptrVector;
	typedef std::list<CompRect *> ptrList;

    private:
	REGION       mRegion;
};

#endif
