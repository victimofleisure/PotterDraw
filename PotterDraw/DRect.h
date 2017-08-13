// Copyleft 2005 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date		comments
		00		28jan03		initial version

		rectangle container class

*/

#ifndef DRECT_INCLUDED
#define DRECT_INCLUDED

#include "DPoint.h"

typedef struct tagDRECT {
	double	x1;
	double	y1;
	double	x2;
	double	y2;
} DRECT;

class DRect : public DRECT {
public:
	DRect();
	DRect(double x1, double y1, double x2, double y2);
	DRect(DPoint p);
	DRect(DPoint p1, DPoint p2);
	DRect(const RECT& r);
	DRect(const DRect& r);
	DRect& operator=(const DRect& r);
	double Height() const;
	double Width() const;
	const DPoint Size() const;
	DPoint& TopLeft();
	const DPoint& TopLeft() const;
	DPoint& BottomRight();
	const DPoint& BottomRight() const;
	const DPoint CenterPoint() const;
	bool IsRectEmpty() const;
	bool IsRectNull() const;
	void SetRectEmpty();
	const DRect operator+(const DPoint& p) const;
	const DRect operator-(const DPoint& p) const;
	DRect& operator+=(const DPoint& p);
	DRect& operator-=(const DPoint& p);
	bool operator==(const DRect& r) const;
	bool operator!=(const DRect& r) const;
	operator RECT() const;
	DRect& Normalize();
	void Union(const DRect& Rect1, const DRect& Rect2);
	void Inflate(const DPoint& p);
	void Deflate(const DPoint& p);
	void Scale(const DPoint& Factor);
	void Scale(double Factor);
	void Scale(const DPoint& Origin, const DPoint& Factor);
	void Scale(const DPoint& Origin, double Factor);
};

inline DRect::DRect()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
}

inline DRect::DRect(double x1, double y1, double x2, double y2)
{
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
}

inline DRect::DRect(DPoint p)
{
	x1 = 0;
	y1 = 0;
	x2 = p.x;
	y2 = p.y;
}

inline DRect::DRect(DPoint p1, DPoint p2)
{
	x1 = p1.x;
	y1 = p1.y;
	x2 = p2.x;
	y2 = p2.y;
}

inline DRect::DRect(const RECT& r)
{
	x1 = r.left;
	y1 = r.top;
	x2 = r.right;
	y2 = r.bottom;
}

inline DRect::DRect(const DRect& r)
{
	x1 = r.x1;
	y1 = r.y1;
	x2 = r.x2;
	y2 = r.y2;
}

inline DRect& DRect::operator=(const DRect& r)
{
	if (this == &r)
		return(*this);	// self-assignment
	x1 = r.x1;
	y1 = r.y1;
	x2 = r.x2;
	y2 = r.y2;
	return(*this);
}

inline double DRect::Height() const
{
	return(y2 - y1);
}

inline double DRect::Width() const
{
	return(x2 - x1);
}

inline const DPoint DRect::Size() const
{
	return(DPoint(x2 - x1, y2 - y1));
}

inline DPoint& DRect::TopLeft()
{
	return(*((DPoint *)this));
}

inline const DPoint& DRect::TopLeft() const
{
	return(*((DPoint *)this));
}

inline DPoint& DRect::BottomRight()
{
	return(*((DPoint *)this + 1));
}

inline const DPoint& DRect::BottomRight() const
{
	return(*((DPoint *)this + 1));
}

inline const DPoint DRect::CenterPoint() const
{
	return(DPoint(x1 + Width() / 2, y1 + Height() / 2));
}

inline bool DRect::IsRectEmpty() const
{
	return(Width() == 0 && Height() == 0);
}

inline bool DRect::IsRectNull() const
{
	return(x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0);
}

inline void DRect::SetRectEmpty()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
}

inline const DRect DRect::operator+(const DPoint& p) const
{
	return(DRect(TopLeft() + p, BottomRight() + p));
}

inline const DRect DRect::operator-(const DPoint& p) const
{
	return(DRect(TopLeft() - p, BottomRight() - p));
}

inline DRect& DRect::operator+=(const DPoint& p)
{
	return(*this = *this + p);
}

inline DRect& DRect::operator-=(const DPoint& p)
{
	return(*this = *this - p);
}

inline bool DRect::operator==(const DRect& p) const
{
	return(TopLeft() == p.TopLeft() && BottomRight() == p.BottomRight());
}

inline bool DRect::operator!=(const DRect& p) const
{
	return(TopLeft() != p.TopLeft() || BottomRight() != p.BottomRight());
}

inline DRect::operator RECT() const
{
	RECT	r;
	r.left		= round(x1);
	r.top		= round(y1);
	r.right		= round(x2);
	r.bottom	= round(y2);
	return(r);
}

inline void DRect::Inflate(const DPoint& p)
{
	TopLeft() -= p;
	BottomRight() += p;
}

inline void DRect::Deflate(const DPoint& p)
{
	TopLeft() += p;
	BottomRight() -= p;
}

#endif
