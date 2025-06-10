#ifndef GpUtilities_h
#define GpUtilities_h
#pragma once

#include "GdiPlus_fwd.h"
#include "Color.h"


// GDI+ support

namespace gp
{
	// for palette devices transparency doesn't work - use 0xFF for alpha

	inline Gdiplus::Color MakeColor( COLORREF rgb, BYTE alpha = 0xFF )
	{
		return Gdiplus::Color( alpha, GetRValue( rgb ), GetGValue( rgb ), GetBValue( rgb ) );
	}

	inline Gdiplus::Color MakeSysColor( int sysColorIndex, BYTE alpha = 0xFF )
	{
		return MakeColor( GetSysColor( sysColorIndex ), alpha );
	}

	inline Gdiplus::Color MakeColor( const ui::CColorAlpha& colorAlpha )
	{
		return MakeColor( colorAlpha.m_color, colorAlpha.m_alpha );
	}

	inline BYTE FromPercentage( UINT percentage )
	{
		ASSERT( percentage <= 100 );
		return (BYTE)( (double)percentage * 255 / 100 );
	}

	inline BYTE MakeAlpha( UINT transpPct )
	{
		return FromPercentage( 100 - transpPct );
	}

	inline Gdiplus::Color MakeTransparentColor( COLORREF rgb, UINT transpPct )
	{
		return MakeColor( rgb, MakeAlpha( transpPct ) );
	}

	inline Gdiplus::Color MakeOpaqueColor( COLORREF rgb, UINT opacityPct )
	{
		return MakeColor( rgb, MakeAlpha( 100 - opacityPct ) );
	}


	// geometric types

	inline Point ToPoint( const CPoint& point )
	{
		return Gdiplus::Point( point.x, point.y );
	}

	inline Size ToSize( const CSize& size )
	{
		return Gdiplus::Size( size.cx, size.cy );
	}

	inline Rect ToRect( const CRect& rect )
	{
		return Gdiplus::Rect( rect.left, rect.top, rect.Width(), rect.Height() );
	}

	inline CPoint FromPoint( const Point& point )
	{
		return CPoint( point.X, point.Y );
	}

	inline CSize FromSize( const Size& size )
	{
		return CSize( size.Width, size.Height );
	}

	inline CRect FromRect( const Rect& rect )
	{
		return CRect( rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom() );
	}


	// drawing

	inline void FrameRectangle( Graphics& graphics, const Rect& rect, const Pen* pPen )
	{
		graphics.DrawRectangle( pPen, rect.X, rect.Y, rect.Width - 1, rect.Height - 1 );		// the equivalent GDI coords of CDC::FrameRect()
	}

	inline void FillRectangle( Graphics& graphics, const Rect& rect, const Brush* pBrush )
	{
		graphics.FillRectangle( pBrush, rect.X, rect.Y, rect.Width - 1, rect.Height - 1 );		// the equivalent GDI coords
	}

	inline void FrameRect( CDC* pDC, const CRect& rect, COLORREF frameRgb, UINT opacityPct )
	{
		Graphics graphics( *pDC );
		Pen pen( gp::MakeOpaqueColor( frameRgb, opacityPct ) );

		gp::FrameRectangle( graphics, gp::ToRect( rect ), &pen );
	}
}


#endif // GpUtilities_h
