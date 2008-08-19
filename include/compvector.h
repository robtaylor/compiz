#ifndef _COMPVECTOR_H
#define _COMPVECTOR_H

class CompVector {
    public:
	typedef enum {
	    x,
	    y,
	    z,
	    w
	} VectorCoordsEnum;

	CompVector ();
	CompVector (float x, float y, float z, float w);

	float& operator[] (int item);
	float& operator[] (VectorCoordsEnum coord);

	const float operator[] (int item) const;
	const float operator[] (VectorCoordsEnum coord) const;

	CompVector& operator+= (const CompVector& rhs);
	CompVector& operator-= (const CompVector& rhs);
	CompVector& operator*= (const float k);
	CompVector& operator/= (const float k);
	CompVector& operator^= (const CompVector& rhs);

	float norm ();
	CompVector& normalize ();

    private:
	friend CompVector operator+ (const CompVector& lhs,
				     const CompVector& rhs);
	friend CompVector operator- (const CompVector& lhs,
				     const CompVector& rhs);
	friend CompVector operator- (const CompVector& vector);
	friend float operator* (const CompVector& lhs,
				const CompVector& rhs);
	friend CompVector operator* (const float       k,
				     const CompVector& vector);
	friend CompVector operator* (const CompVector& vector,
				     const float       k);
	friend CompVector operator/ (const CompVector& lhs,
				     const CompVector& rhs);
	friend CompVector operator^ (const CompVector& lhs,
				     const CompVector& rhs);

	float v[4];
};

#endif
