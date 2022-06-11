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
		return CSize( static_cast<long>( factor * size.cx ), static_cast<long>( factor * size.cy ) );
	}


	inline CRect& SetRectSize( CRect& rRect, const SIZE& size )
	{
		rRect.BottomRight() = rRect.TopLeft() + size;
		return rRect;
	}

	inline bool IsNormalized( const RECT& rect )
	{
		return rect.left <= rect.right && rect.top <= rect.bottom;
	}

	template< typename RectT >
	inline RectT& NormalizeRect( RectT& rRect )
	{
		if ( rRect.left > rRect.right )
			std::swap( rRect.left, rRect.right );

		if ( rRect.top > rRect.bottom )
			std::swap( rRect.top, rRect.bottom );

		return rRect;
	}

	inline CRect GetNormalizedRect( const RECT& rect )
	{
		CRect normalRect = rect;
		NormalizeRect( normalRect );
		return normalRect;
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

	inline bool InBounds( const RECT& boundsRect, const RECT& rect )
	{
		return
			rect.left >= boundsRect.left && rect.top >= boundsRect.top &&
			rect.right <= boundsRect.right && rect.bottom <= boundsRect.bottom;
	}


	inline __int64 GetSizeArea( const SIZE& size ) { return static_cast<__int64>( size.cx ) * static_cast<__int64>( size.cy ); }


	// aspect ratio
	inline double GetAspectRatio( const SIZE& size ) { return (double)size.cx / size.cy; }
	double GetDistFromSquareAspect( const SIZE& size );
	CSize StretchToFit( const SIZE& destBoundsSize, const SIZE& srcSize );
	CRect StretchToFit( const CRect& destBoundsRect, const SIZE& srcSize, CSize spacingSize = CSize( 0, 0 ), TAlignment alignment = H_AlignCenter | V_AlignCenter );

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


	bool IsValidAlignment( TAlignment alignment );

	CRect& AlignRect( CRect& rDest, const RECT& anchor, TAlignment alignment, bool limitDest = false );

	inline CRect& AlignRectHV( CRect& rDest, const RECT& anchor, Alignment horz, Alignment vert, bool limitDest = false )
	{
		return AlignRect( rDest, anchor, horz | vert, limitDest );
	}

	CRect& AlignRectOutside( CRect& rDest, const RECT& anchor, TAlignment alignment, const CSize& spacing = CSize( 0, 0 ) );	// buddy or tile align: layout the rect outside the anchor (by the anchor)

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
