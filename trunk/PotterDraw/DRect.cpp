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

#include "stdafx.h"
#include "DRect.h"

void DRect::Union(const DRect& Rect1, const DRect& Rect2)
{
	x1 = min(Rect1.x1, Rect2.x1);
	y1 = min(Rect1.y1, Rect2.y1);
	x2 = max(Rect1.x2, Rect2.x2);
	y2 = max(Rect1.y2, Rect2.y2);
}

DRect& DRect::Normalize()
{
	if (x1 > x2) {
		double	t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		double	t = y1;
		y1 = y2;
		y2 = t;
	}
	return(*this);
}

void DRect::Scale(const DPoint& Factor)
{
	TopLeft() = TopLeft() * Factor.x;
	BottomRight() = BottomRight() * Factor.y;
}

void DRect::Scale(double Factor)
{
	TopLeft() = TopLeft() * Factor;
	BottomRight() = BottomRight() * Factor;
}

void DRect::Scale(const DPoint& Origin, const DPoint& Factor)
{
	TopLeft() = (TopLeft() - Origin) * Factor.x + Origin;
	BottomRight() = (BottomRight() - Origin) * Factor.y + Origin;
}

void DRect::Scale(const DPoint& Origin, double Factor)
{
	TopLeft() = (TopLeft() - Origin) * Factor + Origin;
	BottomRight() = (BottomRight() - Origin) * Factor + Origin;
}


