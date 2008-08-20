#ifndef _COMPMATRIX_H
#define _COMPMATRIX_H

#include <compvector.h>

class CompOutput;

class CompMatrix {
    public:
	CompMatrix ();

	const float* getMatrix () const;

	CompMatrix& operator*= (const CompMatrix& rhs);

	void reset ();
	void toScreenSpace (CompOutput *output, float z);

	void rotate (const float angle, const float x,
		     const float y, const float z);
	void rotate (const float angle, const CompVector& vector);

	void scale (const float x, const float y, const float z);
	void scale (const CompVector& vector);

	void translate (const float x, const float y, const float z);
	void translate (const CompVector& vector);

    private:
	friend CompMatrix operator* (const CompMatrix& lhs,
				     const CompMatrix& rhs);
	friend CompVector operator* (const CompMatrix& lhs,
				     const CompVector& rhs);

	float m[16];
};

typedef CompMatrix CompTransform;

#endif
