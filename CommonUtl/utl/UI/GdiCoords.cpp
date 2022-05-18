
#include "stdafx.h"
#include "GdiCoords.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ui_fwd.h imlementation

namespace ui
{
	// CValuePct implementation

	int CValuePct::EvalValue( int extent ) const
	{
		if ( HasValue() )
			return GetValue();
		else if ( HasPercentage() )
			return MulDiv( extent, GetPercentage(), 100 );

		ASSERT( false );		// invalid
		return 0;
	}

	double CValuePct::EvalValue( double extent ) const
	{
		if ( HasValue() )
			return GetValue();
		else if ( HasPercentage() )
			return extent * GetPercentage() / 100.0;

		ASSERT( false );		// invalid
		return 0;
	}
}


namespace ui
{
	double GetDistFromSquareAspect( const SIZE& size )
	{
		double aspect = size.cx > size.cy ? ( (double)size.cx / size.cy ) : ( (double)size.cy / size.cx );
		return aspect - 1.0;						// 0 for square apsect: H==V
	}

	CSize StretchToFit( const SIZE& destBoundsSize, const SIZE& srcSize )
	{
		ASSERT( srcSize.cx > 0 && srcSize.cy > 0 );

		static const CSize minSize( 1, 1 );
		CSize destSize = MaxSize( destBoundsSize, minSize );		// ensure positive size when stretching with extreme spacing

		const double srcAspect = GetAspectRatio( srcSize ), destAspect = GetAspectRatio( destSize );

		if ( destAspect > srcAspect )
			destSize.cx = static_cast<long>( (double)destSize.cy * srcAspect );
		else
			destSize.cy = static_cast<long>( (double)destSize.cx / srcAspect );
		return MaxSize( destSize, minSize );
	}

	CRect StretchToFit( const CRect& destBoundsRect, const SIZE& srcSize, CSize spacingSize /*= CSize( 0, 0 )*/, TAlignment alignment /*= H_AlignCenter | V_AlignCenter*/ )
	{
		// reduce spacing if greater that source or destination
		spacingSize = MinSize( spacingSize, destBoundsRect.Size() );
		spacingSize = MinSize( spacingSize, srcSize );

		// NOTE: spacing doesn't need to be streched
		// stretch by factoring out spacing from both destination and source
		CSize destSize = StretchToFit( destBoundsRect.Size() - spacingSize, CSize( srcSize ) - spacingSize );
		CRect destRect( destBoundsRect.TopLeft(), destSize );

		destRect.InflateRect( spacingSize.cx / 2, spacingSize.cy / 2 );		// factoring-in spacing by keeping content centered
		ui::AlignRect( destRect, destBoundsRect, alignment, true );
		return destRect;
	}

	CSize StretchSize( const SIZE& destBoundsSize, const SIZE& srcSize, StretchMode stretchMode )
	{
		switch ( stretchMode )
		{
			case StretchFit:	return StretchToFit( destBoundsSize, srcSize );
			case ShrinkFit:		return ShrinkToFit( destBoundsSize, srcSize );
		}
		return srcSize;
	}


	bool IsValidAlignment( TAlignment alignment )
	{
		switch ( alignment & HorizontalMask )
		{
			case H_AlignLeft:
			case H_AlignCenter:
			case H_AlignRight:
			case NoAlign:
				break;
			default:
				return false;		// conflicting horizontal alignment values
		}

		switch ( alignment & VerticalMask )
		{
			case V_AlignTop:
			case V_AlignCenter:
			case V_AlignBottom:
			case NoAlign:
				break;
			default:
				return false;		// conflicting vertical alignment values
		}
		return true;		// no alignment conflicts
	}

	CRect& AlignRect( CRect& rDest, const RECT& anchor, TAlignment alignment, bool limitDest /*= false*/ )
	{
		CSize offset( 0, 0 );

		switch ( alignment & HorizontalMask )
		{
			case H_AlignLeft:
				offset.cx = anchor.left - rDest.left;
				break;
			case H_AlignCenter:
				offset.cx = anchor.left - rDest.left + ( RectWidth( anchor ) - rDest.Width() ) / 2;
				break;
			case H_AlignRight:
				offset.cx = anchor.right - rDest.Width() - rDest.left;
				break;
			case NoAlign:
				break;
			default:
				ASSERT( false );
		}

		switch ( alignment & VerticalMask )
		{
			case V_AlignTop:
				offset.cy = anchor.top - rDest.top;
				break;
			case V_AlignCenter:
				offset.cy = anchor.top - rDest.top + ( RectHeight( anchor ) - rDest.Height() ) / 2;
				break;
			case V_AlignBottom:
				offset.cy = anchor.bottom - rDest.Height() - rDest.top;
				break;
			case NoAlign:
				break;
			default:
				ASSERT( false );
		}

		rDest.OffsetRect( offset );

		if ( limitDest && !rDest.IsRectEmpty() )
			rDest &= anchor;

		return rDest;
	}

	CRect& AlignRectOutside( CRect& rDest, const RECT& anchor, TAlignment alignment, const CSize& spacing /*= CSize( 0, 0 )*/ )
	{	// buddy or tile align: layout the rect outside the anchor (by the anchor)
		CSize offset( 0, 0 );

		switch ( alignment & HorizontalMask )
		{
			case H_AlignLeft:
				rDest.MoveToX( anchor.left - rDest.Width() - spacing.cx );		// tile to the left
				break;
			case H_AlignCenter:
				offset.cx = anchor.left - rDest.left + ( RectWidth( anchor ) - rDest.Width() ) / 2;
				break;
			case H_AlignRight:
				rDest.MoveToX( anchor.right + spacing.cx );						// tile to the right
				break;
			case NoAlign:
				break;
			default:
				ASSERT( false );
		}

		switch ( alignment & VerticalMask )
		{
			case V_AlignTop:
				rDest.MoveToY( anchor.top - rDest.Height() - spacing.cy );		// tile to the top
				break;
			case V_AlignCenter:
				offset.cy = anchor.top - rDest.top + ( RectHeight( anchor ) - rDest.Height() ) / 2;
				break;
			case V_AlignBottom:
				rDest.MoveToY( anchor.bottom + spacing.cy );					// tile to the bottom
				break;
			case NoAlign:
				break;
			default:
				ASSERT( false );
		}

		rDest.OffsetRect( offset );
		return rDest;
	}

	CRect& CenterRect( CRect& rDest, const RECT& anchor, bool horiz /*= true*/, bool vert /*= true*/, bool limitDest /*= false*/,
					   const CSize& offset /*= CSize( 0, 0 )*/ )
	{
		CPoint delta = TopLeft( anchor ) - rDest.TopLeft() + ScaleSize( RectSize( anchor ) - rDest.Size() + offset, 1, 2 );

		if ( !horiz )
			delta.x = 0;
		if ( !vert )
			delta.y = 0;
		rDest.OffsetRect( delta );

		if ( limitDest && !rDest.IsRectEmpty() )
			rDest &= anchor;
		return rDest;
	}

	void StretchRect( CRect& rDest, const RECT& anchor, Stretch stretch )
	{
		if ( Width == stretch || Size == stretch )
		{
			rDest.left = anchor.left;
			rDest.right = anchor.right;
		}
		if ( Height == stretch || Size == stretch )
		{
			rDest.top = anchor.top;
			rDest.bottom = anchor.bottom;
		}
	}

	bool EnsureVisibleRect( CRect& rDest, const RECT& anchor, bool horiz /*= true*/, bool vert /*= true*/ )
	{
		if ( rDest.left >= anchor.left && rDest.top >= anchor.top &&
			 rDest.right <= anchor.right && rDest.bottom <= anchor.bottom )
			return false;		// no overflow, no change

		CPoint offset( 0, 0 );

		if ( horiz )
			if ( rDest.Width() > RectWidth( anchor ) )
				offset.x = anchor.left - rDest.left;
			else
				if ( rDest.left < anchor.left )
					offset.x = anchor.left - rDest.left;
				else if ( rDest.right > anchor.right )
					offset.x = anchor.right - rDest.right;

		if ( vert )
			if ( rDest.Height() > RectHeight( anchor ) )
				offset.y = anchor.top - rDest.top;
			else
				if ( rDest.top < anchor.top )
					offset.y = anchor.top - rDest.top;
				else if ( rDest.bottom > anchor.bottom )
					offset.y = anchor.bottom - rDest.bottom;

		if ( 0 == offset.x && 0 == offset.y )
			return false;		// no change

		rDest += offset;
		return true;
	}

	void EnsureMinEdge( CRect& rInner, const RECT& outer, int edge /*= 1*/ )
	{
		if ( rInner.Width() * rInner.Height() != 0 )
		{
			if ( rInner.left == outer.left )
				rInner.left += edge;
			if ( rInner.top == outer.top )
				rInner.top += edge;
			if ( rInner.right == outer.right )
				rInner.right -= edge;
			if ( rInner.bottom == outer.bottom )
				rInner.bottom -= edge;
		}
	}


	LineOrientation GetLineOrientation( const POINT& point1, const POINT& point2 )
	{
		if ( point1.x == point2.x && point1.y == point2.y )
			return NoLine;

		if ( point1.x == point2.x )
			return VerticalLine;

		if ( point1.y == point2.y )
			return HorizontalLine;

		return DiagonalLine;
	}
}


namespace ui
{
	int CombineWithRegion( CRgn* pCombined, const RECT& rect, int combineMode )
	{
		CRgn rectRgn;
		rectRgn.CreateRectRgnIndirect( &rect );
		return pCombined->CombineRgn( pCombined, &rectRgn, combineMode );
	}

	int CombineRects( CRgn* pCombined, const RECT& outerRect, const RECT& innerRect, int combineMode /*= RGN_DIFF*/ )
	{
		CRgn outerRgn, innerRgn;
		outerRgn.CreateRectRgnIndirect( &outerRect );
		innerRgn.CreateRectRgnIndirect( &innerRect );

		pCombined->CreateRectRgn( 0, 0, 0, 0 );
		return pCombined->CombineRgn( &outerRgn, &innerRgn, combineMode );
	}
}
