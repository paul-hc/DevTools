#ifndef TextEffect_h
#define TextEffect_h
#pragma once

#include "ui_fwd.h"


namespace ui
{
	struct CFrameFillTraits
	{
		CFrameFillTraits( void ) : m_color( CLR_NONE ), m_frameAlpha( 0xFF ), m_fillAlpha( 0xFF ) {}
		CFrameFillTraits( COLORREF color, BYTE frameAlpha, BYTE fillAlpha ) : m_color( color ), m_frameAlpha( frameAlpha ), m_fillAlpha( fillAlpha ) {}

		void Init( COLORREF color, UINT frameOpacityPct, UINT fillOpacityPct );

		bool IsNull( void ) const { return CLR_NONE == m_color || IsDefault(); }
		bool IsDefault( void ) const { return CLR_DEFAULT == m_color; }
	private:
		static BYTE FromPercentage( UINT percentage ) { ASSERT( percentage <= 100 ); return static_cast< BYTE >( (double)percentage * 255 / 100 ); }
		static BYTE MakeOpaqueAlpha( UINT opacityPct ) { return FromPercentage( opacityPct ); }
	public:
		COLORREF m_color;
		BYTE m_frameAlpha;
		BYTE m_fillAlpha;
	};


	struct CTextEffect
	{
		explicit CTextEffect( ui::TFontEffect fontEffect = ui::Regular, COLORREF textColor = CLR_NONE, COLORREF bkColor = CLR_NONE, const CFrameFillTraits& frameFillTraits = CFrameFillTraits() )
			: m_fontEffect( fontEffect ), m_textColor( textColor ), m_bkColor( bkColor ), m_frameFillTraits( frameFillTraits ) {}

		static CTextEffect MakeColor( COLORREF textColor, COLORREF bkColor = CLR_NONE, ui::TFontEffect fontEffect = ui::Regular ) { return CTextEffect( fontEffect, textColor, bkColor ); }

		bool IsNull( void ) const { return ui::Regular == m_fontEffect && CLR_NONE == m_textColor && CLR_NONE == m_bkColor && m_frameFillTraits.IsNull(); }

		bool AssignPtr( const CTextEffect* pRight );
		void SetFrameFromTextColor( UINT frameOpacityPct, UINT fillOpacityPct ) { m_frameFillTraits.Init( m_textColor, frameOpacityPct, fillOpacityPct ); }

		CTextEffect& operator|=( const CTextEffect& right ) { Combine( right ); return *this; }
		CTextEffect operator|( const CTextEffect& right ) const { CTextEffect effect = *this; effect.Combine( right ); return effect; }

		void Combine( const CTextEffect& right );
		bool CombinePtr( const CTextEffect* pRight );
	public:
		ui::TFontEffect m_fontEffect;
		COLORREF m_textColor;
		COLORREF m_bkColor;
		CFrameFillTraits m_frameFillTraits;			// surrounds text with a semi-transparent frame and fill

		static const ui::CTextEffect s_null;
	};
}


#endif // TextEffect_h
