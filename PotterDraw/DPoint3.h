// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		14dec17	initial version

		double-precision 3D coordinate

*/

#ifndef DPOINT3_INCLUDED
#define DPOINT3_INCLUDED

#include <math.h>

typedef struct tagDPOINT3 {
	double	x;
	double	y;
	double	z;
} DPOINT3;

class DPoint3 : public DPOINT3 {
public:
	DPoint3();
	DPoint3(double x, double y, double z);
	DPoint3(const DPoint3& p);
	DPoint3(const DPOINT3& p);
#ifdef D3DVECTOR_DEFINED
	DPoint3(const D3DVECTOR& p);
#endif
	DPoint3& operator=(const DPoint3& p);
	DPoint3& operator=(const DPOINT3& p);
	const DPoint3 operator+(const DPoint3& p) const;
	const DPoint3 operator-(const DPoint3& p) const;
	const DPoint3 operator*(const DPoint3& p) const;
	const DPoint3 operator/(const DPoint3& p) const;
	const DPoint3 operator+(double d) const;
	const DPoint3 operator-(double d) const;
	const DPoint3 operator*(double d) const;
	const DPoint3 operator/(double d) const;
	DPoint3& operator+=(const DPoint3& p);
	DPoint3& operator-=(const DPoint3& p);
	DPoint3& operator*=(const DPoint3& p);
	DPoint3& operator/=(const DPoint3& p);
	DPoint3& operator+=(double d);
	DPoint3& operator-=(double d);
	DPoint3& operator*=(double d);
	DPoint3& operator/=(double d);
	bool operator==(const DPoint3& p) const;
	bool operator!=(const DPoint3& p) const;
	bool Compare(const DPoint3& p, double fEpsilon = 1e-10) const;
	const double operator[](int i) const;
	double& operator[](int i);
	DPoint3 Square() const;
	DPoint3 SquareRoot() const;
	double Length() const;
	double Distance(const DPoint3& p) const;
	void Scale(DPoint3 ptOrigin, double fScale);
	void Scale(DPoint3 ptOrigin, DPoint3 ptScale);
	double Dot(const DPoint3& p) const;
	void Normalize();
};

inline DPoint3::DPoint3()
{
}

inline DPoint3::DPoint3(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

inline DPoint3::DPoint3(const DPoint3& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
}

inline DPoint3::DPoint3(const DPOINT3& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
}

#ifdef D3DVECTOR_DEFINED
inline DPoint3::DPoint3(const D3DVECTOR& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
}
#endif

inline DPoint3& DPoint3::operator=(const DPoint3& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
	return(*this);
}

inline DPoint3& DPoint3::operator=(const DPOINT3& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
	return(*this);
}

const inline DPoint3 DPoint3::operator+(const DPoint3& p) const
{
	return(DPoint3(x + p.x, y + p.y, z + p.z));
}

const inline DPoint3 DPoint3::operator-(const DPoint3& p) const
{
	return(DPoint3(x - p.x, y - p.y, z - p.z));
}

const inline DPoint3 DPoint3::operator*(const DPoint3& p) const
{
	return(DPoint3(x * p.x, y * p.y, z * p.z));
}

const inline DPoint3 DPoint3::operator/(const DPoint3& p) const
{
	return(DPoint3(x / p.x, y / p.y, z / p.z));
}

const inline DPoint3 DPoint3::operator+(double d) const
{
	return(*this + DPoint3(d, d, d));
}

const inline DPoint3 DPoint3::operator-(double d) const
{
	return(*this - DPoint3(d, d, d));
}

const inline DPoint3 DPoint3::operator*(double d) const
{
	return(*this * DPoint3(d, d, d));
}

const inline DPoint3 DPoint3::operator/(double d) const
{
	return(*this / DPoint3(d, d, d));
}

inline DPoint3& DPoint3::operator+=(const DPoint3& p)
{
	return(*this = *this + p);
}

inline DPoint3& DPoint3::operator-=(const DPoint3& p)
{
	return(*this = *this - p);
}

inline DPoint3& DPoint3::operator*=(const DPoint3& p)
{
	return(*this = *this * p);
}

inline DPoint3& DPoint3::operator/=(const DPoint3& p)
{
	return(*this = *this / p);
}

inline DPoint3& DPoint3::operator+=(double d)
{
	return(*this = *this + d);
}

inline DPoint3& DPoint3::operator-=(double d)
{
	return(*this = *this - d);
}

inline DPoint3& DPoint3::operator*=(double d)
{
	return(*this = *this * d);
}

inline DPoint3& DPoint3::operator/=(double d)
{
	return(*this = *this / d);
}

inline bool DPoint3::operator==(const DPoint3& p) const
{
	return(p.x == x && p.y == y && p.z == z);
}

inline bool DPoint3::operator!=(const DPoint3& p) const
{
	return(p.x != x || p.y != y || p.z != z);
}

inline bool DPoint3::Compare(const DPoint3& p, double fEpsilon) const
{
	return(fabs(p.x - x) < fEpsilon && fabs(p.y - y) < fEpsilon && fabs(p.z - z) < fEpsilon);
}

inline const double DPoint3::operator[](int i) const
{
	ASSERT(i >= 0 && i < 3);
	return(((double *)this)[i]);
}

inline double& DPoint3::operator[](int i)
{
	ASSERT(i >= 0 && i < 3);
	return(((double *)this)[i]);
}

inline DPoint3 DPoint3::Square() const
{
	return(DPoint3(x * x, y * y, z * z));	
}

inline DPoint3 DPoint3::SquareRoot() const
{
	return(DPoint3(sqrt(x), sqrt(y), sqrt(z)));
}

inline double DPoint3::Length() const
{
	return(sqrt(x * x + y * y + z * z));
}

inline double DPoint3::Distance(const DPoint3& p) const
{
	return((p - *this).Length());
}

inline void DPoint3::Scale(DPoint3 ptOrigin, double fScale)
{
	*this = (*this - ptOrigin) * fScale + ptOrigin;
}

inline void DPoint3::Scale(DPoint3 ptOrigin, DPoint3 ptScale)
{
	*this = (*this - ptOrigin) * ptScale + ptOrigin;
}

inline double DPoint3::Dot(const DPoint3& p) const
{
	return(x * p.x + y * p.y + z * p.z);
}

inline void DPoint3::Normalize()
{
	*this /= Length();
}

#endif
