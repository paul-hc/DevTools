#ifndef GdiCoords_h
#define GdiCoords_h
#pragma once

#include "ui_fwd.h"


namespace ui
{
	inline CPoint MaxPoint( const POINT& left, const POINT& right ) { return CPoint( std::max( left.x, right.x ), std::max( left.y, right.y ) ); }
	inline CPoint MinPoint( const POINT& left, const POINT& right ) { return CPoint( std::min( left.x, right.x ), std::min( left.y, right.y ) ); }

	inline CSize MaxSize( const SIZE& left, const SIZE& right ) { return CSize( std::max( left.cx, right.cx ), std::max( left.cy, right.cy ) ); }
	inline CSize MinSize( const SIZE& left, const SIZE& right ) { return CSize( std::min( left.cx, right.cx ), std::min( left.cy, right.cy ) ); }

	inline bool IsEmptySize( const SIZE& size ) { return 0 == size.cx || 0 == size.cy; }

	inline CSize ScaleSize( const SIZE& size, int mulBy, int divBy = 100 )
	{
		return CSize( MulDiv( size.cx, mulBy, divBy ), MulDiv( size.cy, mulBy, divBy ) );
	}

	inline CSize ScaleSize( const SIZE& size, double factor )
	{
		return CSize( static_cast< long >( factor * size.cx ), static_cast< long >( factor * size.cy ) );
	}

	inline CRect& SetRectSize( CRect& rRect, const SIZE& size )
	{
		rRect.BottomRight() = rRect.TopLeft() + size;
		return rRect;
	}


	inline bool FitsInside( const SIZE& destBoundsSize, const SIZE& srcSize )
	{
		return srcSize.cx <= destBoundsSize.cx && srcSize.cy <= destBoundsSize.cy;
	}

	inline bool FitsExactly( const SIZE& destBoundsSize, const SIZE& srcSize )
	{
		return
			( srcSize.cx == destBoundsSize.cx && srcSize.cy <= destBoundsSize.cy ) ||
			( srcSize.cx <= destBoundsSize.cx && srcSize.cy == destBoundsSize.cy );
	}


	// aspect ratio
	inline double GetAspectRatio( const SIZE& size ) { return (double)size.cx / size.cy; }
	double GetDistFromSquareAspect( const SIZE& size );
	CSize StretchToFit( const SIZE& destBoundsSize, const SIZE& srcSize );
	CRect StretchToFit( const CRect& destBoundsRect, const SIZE& srcSize, CSize spacingSize = CSize( 0, 0 ), int alignment = H_AlignCenter | V_AlignCenter );

	inline CSize ShrinkToFit( const SIZE& destBoundsSize, const SIZE& srcSize )
	{
		return FitsInside( destBoundsSize, srcSize )
			? srcSize
			: StretchToFit( destBoundsSize, srcSize );
	}

	CSize StretchSize( const SIZE& destBoundsSize, const SIZE& srcSize, StretchMode stretchMode );


	inline CPoint TopLeft( const RECT& rect ) { return CPoint( rect.left, rect.top ); }
	inline CPoint TopRight( const RECT& rect ) { return CPoint( rect.right, rect.top ); }
	inline CPoint BottomLeft( const RECT& rect ) { return CPoint( rect.left, rect.bottom ); }
	inline CPoint BottomRight( const RECT& rect ) { return CPoint( rect.right, rect.bottom ); }

	inline CSize RectSize( const RECT& rect ) { return CSize( rect.right - rect.left, rect.bottom - rect.top ); }
	inline int RectWidth( const RECT& rect ) { return rect.right - rect.left; }
	inline int RectHeight( const RECT& rect ) { return rect.bottom - rect.top; }


	CRect& AlignRect( CRect& rDest, const RECT& anchor, int alignment, bool limitDest = false );

	inline CRect& AlignRectHV( CRect& rDest, const RECT& anchor, Alignment horz, Alignment vert, bool limitDest = false )
	{
		return AlignRect( rDest, anchor, horz | vert, limitDest );
	}

	CRect& CenterRect( CRect& rDest, const RECT& anchor, bool horiz = true, bool vert = true, bool limitDest = false, const CSize& offset = CSize( 0, 0 ) );


	enum Stretch { Width, Height, Size };

	void StretchRect( CRect& rDest, const RECT& anchor, Stretch stretch );


	bool EnsureVisibleRect( CRect& rDest, const RECT& anchor, bool horiz = true, bool vert = true );
	void EnsureMinEdge( CRect& rInner, const RECT& outer, int edge = 1 );


	enum LineOrientation { HorizontalLine, VerticalLine, DiagonalLine, NoLine };

	LineOrientation GetLineOrientation( const POINT& point1, const POINT& point2 );
}


namespace ui
{
	// GDI utils

	int CombineWithRegion( CRgn* pCombined, const RECT& rect, int combineMode );
	int CombineRects( CRgn* pCombined, const RECT& outerRect, const RECT& innerRect, int combineMode = RGN_DIFF );
}


#endif // GdiCoords_h