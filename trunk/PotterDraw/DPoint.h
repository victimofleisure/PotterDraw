// Copyleft 2005 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		28jan03	initial version
		01		17apr06	add DPOINT ctor and assignment
		02		14feb08	add SIZE ctor
        03		10jun17	remove self-assignment checks
        04		16jun17	add CPoint operator
        05		22jun17	add length, distance and intersect methods
		06		07dec17	add dot product and normalize
		07		15dec17	make == and != exact, and add Compare with epsilon

		double-precision 2D coordinate

*/

#ifndef DPOINT_INCLUDED
#define DPOINT_INCLUDED

#include <math.h>

typedef struct tagDPOINT {
	double	x;
	double	y;
} DPOINT;

class DPoint : public DPOINT {
public:
	DPoint();
	DPoint(double x, double y);
	DPoint(const DPoint& p);
	DPoint(const DPOINT& p);
	DPoint(const POINT& p);
	DPoint(const SIZE& p);
	DPoint& operator=(const DPoint& p);
	DPoint& operator=(const DPOINT& p);
	const DPoint operator+(const DPoint& p) const;
	const DPoint operator-(const DPoint& p) const;
	const DPoint operator*(const DPoint& p) const;
	const DPoint operator/(const DPoint& p) const;
	const DPoint operator+(double d) const;
	const DPoint operator-(double d) const;
	const DPoint operator*(double d) const;
	const DPoint operator/(double d) const;
	DPoint& operator+=(const DPoint& p);
	DPoint& operator-=(const DPoint& p);
	DPoint& operator*=(const DPoint& p);
	DPoint& operator/=(const DPoint& p);
	DPoint& operator+=(double d);
	DPoint& operator-=(double d);
	DPoint& operator*=(double d);
	DPoint& operator/=(double d);
	bool operator==(const DPoint& p) const;
	bool operator!=(const DPoint& p) const;
	bool Compare(const DPoint& p, double fEpsilon = 1e-10) const;
	const double operator[](int i) const;
	double& operator[](int i);
  	operator POINT() const;
  	operator CPoint() const;
	static bool Equal(double a, double b);
	DPoint Square() const;
	DPoint SquareRoot() const;
	double Length() const;
	double Distance(const DPoint& p) const;
	void Scale(DPoint ptOrigin, double fScale);
	void Scale(DPoint ptOrigin, DPoint ptScale);
	void Rotate(DPoint ptOrigin, double fRotation);
	DPoint Intersect(DPoint p1, DPoint p2, DPoint& vIntersect) const;
	double Dot(const DPoint& p) const;
	void Normalize();
};

inline DPoint::DPoint()
{
}

inline DPoint::DPoint(double x, double y)
{
	this->x = x;
	this->y = y;
}

inline DPoint::DPoint(const DPoint& p)
{
	x = p.x;
	y = p.y;
}

inline DPoint::DPoint(const DPOINT& p)
{
	x = p.x;
	y = p.y;
}

inline DPoint::DPoint(const POINT& p)
{
	x = p.x;
	y = p.y;
}

inline DPoint::DPoint(const SIZE& p)
{
	x = p.cx;
	y = p.cy;
}

inline DPoint& DPoint::operator=(const DPoint& p)
{
	x = p.x;
	y = p.y;
	return(*this);
}

inline DPoint& DPoint::operator=(const DPOINT& p)
{
	x = p.x;
	y = p.y;
	return(*this);
}

const inline DPoint DPoint::operator+(const DPoint& p) const
{
	return(DPoint(x + p.x, y + p.y));
}

const inline DPoint DPoint::operator-(const DPoint& p) const
{
	return(DPoint(x - p.x, y - p.y));
}

const inline DPoint DPoint::operator*(const DPoint& p) const
{
	return(DPoint(x * p.x, y * p.y));
}

const inline DPoint DPoint::operator/(const DPoint& p) const
{
	return(DPoint(x / p.x, y / p.y));
}

const inline DPoint DPoint::operator+(double d) const
{
	return(*this + DPoint(d, d));
}

const inline DPoint DPoint::operator-(double d) const
{
	return(*this - DPoint(d, d));
}

const inline DPoint DPoint::operator*(double d) const
{
	return(*this * DPoint(d, d));
}

const inline DPoint DPoint::operator/(double d) const
{
	return(*this / DPoint(d, d));
}

inline DPoint& DPoint::operator+=(const DPoint& p)
{
	return(*this = *this + p);
}

inline DPoint& DPoint::operator-=(const DPoint& p)
{
	return(*this = *this - p);
}

inline DPoint& DPoint::operator*=(const DPoint& p)
{
	return(*this = *this * p);
}

inline DPoint& DPoint::operator/=(const DPoint& p)
{
	return(*this = *this / p);
}

inline DPoint& DPoint::operator+=(double d)
{
	return(*this = *this + d);
}

inline DPoint& DPoint::operator-=(double d)
{
	return(*this = *this - d);
}

inline DPoint& DPoint::operator*=(double d)
{
	return(*this = *this * d);
}

inline DPoint& DPoint::operator/=(double d)
{
	return(*this = *this / d);
}

inline bool DPoint::operator==(const DPoint& p) const
{
	return(p.x == x && p.y == y);
}

inline bool DPoint::operator!=(const DPoint& p) const
{
	return(p.x != x || p.y != y);
}

inline bool DPoint::Compare(const DPoint& p, double fEpsilon) const
{
	return(fabs(p.x - x) < fEpsilon && fabs(p.y - y) < fEpsilon);
}

inline const double DPoint::operator[](int i) const
{
	ASSERT(i >= 0 && i < 2);
	return(((double *)this)[i]);
}

inline double& DPoint::operator[](int i)
{
	ASSERT(i >= 0 && i < 2);
	return(((double *)this)[i]);
}

inline DPoint::operator POINT() const
{
	POINT	p;
	p.x = round(x);
	p.y = round(y);
	return(p);
}

inline DPoint::operator CPoint() const
{
	CPoint	p;
	p.x = round(x);
	p.y = round(y);
	return(p);
}

inline DPoint DPoint::Square() const
{
	return(DPoint(x * x, y * y));	
}

inline DPoint DPoint::SquareRoot() const
{
	return(DPoint(sqrt(x), sqrt(y)));
}

inline double DPoint::Length() const
{
	return(sqrt(x * x + y * y));
}

inline double DPoint::Distance(const DPoint& p) const
{
	return((p - *this).Length());
}

inline void DPoint::Scale(DPoint ptOrigin, double fScale)
{
	*this = (*this - ptOrigin) * fScale + ptOrigin;
}

inline void DPoint::Scale(DPoint ptOrigin, DPoint ptScale)
{
	*this = (*this - ptOrigin) * ptScale + ptOrigin;
}

inline double DPoint::Dot(const DPoint& p) const
{
	return(x * p.x + y * p.y);
}

inline void DPoint::Normalize()
{
	*this /= Length();
}

#endif
