// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		25aug17	add check for updates
		02		05sep17	add view spline drag
		
*/

#ifdef GROUPDEF

GROUPDEF(	VIEW		)
GROUPDEF(	EXPORT		)
GROUPDEF(	OSCILLOSCOPE	)
GROUPDEF(	SPLINE		)
GROUPDEF(	GENERAL		)

#undef GROUPDEF
#endif

#ifdef PROPDEF

//			group		proptype	type		name					initval			minval		maxval		itemname	items
PROPDEF(	VIEW,		VAR,		double,		fZoomStep,				10.0,			1.0,		100.0,		NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fPanStep,				30.0,			1.0,		100.0,		NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fDragRotateStep,		0.57f,			0.01,		1.0,		NULL,		0)
PROPDEF(	EXPORT,		VAR,		bool,		bVertexColor,			0,				0,			0,			NULL,		0)
PROPDEF(	EXPORT,		VAR,		bool,		bCustomImageSize,		0,				0,			0,			NULL,		0)
PROPDEF(	EXPORT,		VAR,		int,		nCustomImageWidth,		1024,			1,			USHRT_MAX,	NULL,		0)
PROPDEF(	EXPORT,		VAR,		int,		nCustomImageHeight,		768,			1,			USHRT_MAX,	NULL,		0)
PROPDEF(	EXPORT,		VAR,		int,		nFloatPrecision,		6,				3,			17,			NULL,		0)
PROPDEF(	OSCILLOSCOPE,	VAR,	bool,		bPlotAllModulations,	0,				0,			0,			NULL,		0)
PROPDEF(	SPLINE,		VAR,		double,		fSplineZoomStep,		10.0,			1.0,		100.0,		NULL,		0)
PROPDEF(	SPLINE,		VAR,		bool,		bViewSplineDrag,		0,				0,			0,			NULL,		0)
PROPDEF(	GENERAL,	VAR,		bool,		bPropertyDescrips,		1,				0,			0,			NULL,		0)
PROPDEF(	GENERAL,	VAR,		bool,		bAutoCheckUpdates,		1,				0,			0,			NULL,		0)

#undef PROPDEF
#endif
