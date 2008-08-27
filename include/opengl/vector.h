#ifndef _GLVECTOR_H
#define _GLVECTOR_H

class GLVector {
    public:
	typedef enum {
	    x,
	    y,
	    z,
	    w
	} VectorCoordsEnum;

	GLVector ();
	GLVector (float x, float y, float z, float w);

	float& operator[] (int item);
	float& operator[] (VectorCoordsEnum coord);

	const float operator[] (int item) const;
	const float operator[] (VectorCoordsEnum coord) const;

	GLVector& operator+= (const GLVector& rhs);
	GLVector& operator-= (const GLVector& rhs);
	GLVector& operator*= (const float k);
	GLVector& operator/= (const float k);
	GLVector& operator^= (const GLVector& rhs);

	float norm ();
	GLVector& normalize ();

    private:
	friend GLVector operator+ (const GLVector& lhs,
				   const GLVector& rhs);
	friend GLVector operator- (const GLVector& lhs,
				   const GLVector& rhs);
	friend GLVector operator- (const GLVector& vector);
	friend float operator* (const GLVector& lhs,
				const GLVector& rhs);
	friend GLVector operator* (const float       k,
				   const GLVector& vector);
	friend GLVector operator* (const GLVector& vector,
				   const float       k);
	friend GLVector operator/ (const GLVector& lhs,
				   const GLVector& rhs);
	friend GLVector operator^ (const GLVector& lhs,
				   const GLVector& rhs);

	float v[4];
};

#endif
