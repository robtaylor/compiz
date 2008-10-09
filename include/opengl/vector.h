/*
 * Copyright © 2008 Danny Baumann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Danny Baumann <maniac@compiz-fusion.org>
 */

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
