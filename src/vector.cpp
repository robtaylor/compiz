/*
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * From Mesa 3-D graphics library.
 */

#include <string.h>
#include <math.h>
#include <compvector.h>

CompVector::CompVector ()
{
    memset (v, 0, sizeof (v));
}

CompVector::CompVector (float x,
			float y,
			float z,
			float w)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

float&
CompVector::operator[] (int item)
{
    return v[item];
}

float&
CompVector::operator[] (VectorCoordsEnum coord)
{
    int item = (int) coord;
    return v[item];
}

const float
CompVector::operator[] (int item) const
{
    return v[item];
}

const float
CompVector::operator[] (VectorCoordsEnum coord) const
{
    int item = (int) coord;
    return v[item];
}

CompVector&
CompVector::operator+= (const CompVector& rhs)
{
    int i;

    for (i = 0; i < 4; i++)
	v[i] += rhs[i];

    return *this;
}

CompVector
operator+ (const CompVector& lhs,
	   const CompVector& rhs)
{
    CompVector result;
    int        i;

    for (i = 0; i < 4; i++)
	result[i] = lhs[i] + rhs[i];

    return result;
}

CompVector&
CompVector::operator-= (const CompVector& rhs)
{
    int i;

    for (i = 0; i < 4; i++)
	v[i] -= rhs[i];

    return *this;
}

CompVector
operator- (const CompVector& lhs,
	   const CompVector& rhs)
{
    CompVector result;
    int        i;

    for (i = 0; i < 4; i++)
	result[i] = lhs[i] - rhs[i];

    return result;
}

CompVector
operator- (const CompVector& vector)
{
    CompVector result;
    int        i;

    for (i = 0; i < 4; i++)
	result[i] = -vector[i];

    return result;
}

CompVector&
CompVector::operator*= (const float k)
{
    int i;

    for (i = 0; i < 4; i++)
	v[i] *= k;

    return *this;
}

float
operator* (const CompVector& lhs,
	   const CompVector& rhs)
{
    float result = 0;
    int   i;

    for (i = 0; i < 4; i++)
	result += lhs[i] * rhs[i];

    return result;
}

CompVector
operator* (const float       k,
	   const CompVector& vector)
{
    CompVector result;
    int        i;

    for (i = 0; i < 4; i++)
	result[i] = k * vector[i];

    return result;
}

CompVector
operator* (const CompVector& vector,
	   const float       k)
{
    return k * vector;
}

CompVector&
CompVector::operator/= (const float k)
{
    int i;

    for (i = 0; i < 4; i++)
	v[i] /= k;

    return *this;
}

CompVector
operator/ (const CompVector& vector,
	   const float       k)
{
    CompVector result;
    int        i;

    for (i = 0; i < 4; i++)
	result[i] = vector[i] / k;

    return result;
}

CompVector&
CompVector::operator^= (const CompVector& vector)
{
    *this = *this ^ vector;
    return *this;
}

CompVector
operator^ (const CompVector& lhs,
	   const CompVector& rhs)
{
    CompVector result;

    result[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    result[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    result[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
    result[3] = 0.0f;

    return result;
}
