#ifndef _GLMATRIX_H
#define _GLMATRIX_H

#include <opengl/vector.h>

class CompOutput;

class GLMatrix {
    public:
	GLMatrix ();

	const float* getMatrix () const;

	GLMatrix& operator*= (const GLMatrix& rhs);

	void reset ();
	void toScreenSpace (CompOutput *output, float z);

	void rotate (const float angle, const float x,
		     const float y, const float z);
	void rotate (const float angle, const GLVector& vector);

	void scale (const float x, const float y, const float z);
	void scale (const GLVector& vector);

	void translate (const float x, const float y, const float z);
	void translate (const GLVector& vector);

    private:
	friend GLMatrix operator* (const GLMatrix& lhs,
				   const GLMatrix& rhs);
	friend GLVector operator* (const GLMatrix& lhs,
				   const GLVector& rhs);

	float m[16];
};

#endif
