#ifndef ui_fwd_h
#define ui_fwd_h
#pragma once


enum Alignment
{
	NoAlign = 0,

	H_AlignLeft		= 0x01,
	H_AlignCenter	= 0x02,
	H_AlignRight	= 0x03,

	V_AlignTop		= 0x10,
	V_AlignCenter	= 0x20,
	V_AlignBottom	= 0x30,

		HorizontalMask	= 0x0F,
		VerticalMask	= 0xF0
};


namespace ui
{
	enum StretchMode { OriginalSize, StretchFit, ShrinkFit };
	enum ComboField { BySel, ByEdit };

	enum PopupAlign { DropRight, DropDown, DropLeft, DropUp };
	enum { HistoryMaxSize = 20 };
	enum StdPopup { AppMainPopup, AppDebugPopup, TextEditorPopup, HistoryComboPopup, ListViewSelectionPopup, ListViewNowherePopup };

	enum FontEffect { Regular = 0, Bold = 1 << 0, Italic = 1 << 1, Underline = 1 << 2 };
	typedef int TFontEffect;


	struct CTextEffect
	{
		explicit CTextEffect( ui::TFontEffect fontEffect = ui::Regular, COLORREF textColor = CLR_NONE, COLORREF bkColor = CLR_NONE )
			: m_fontEffect( fontEffect ), m_textColor( textColor ), m_bkColor( bkColor ) {}

		static CTextEffect MakeColor( COLORREF textColor, COLORREF bkColor = CLR_NONE, ui::TFontEffect fontEffect = ui::Regular ) { return CTextEffect( fontEffect, textColor, bkColor ); }

		CTextEffect& operator|=( const CTextEffect& right ) { CombineWith( right ); return *this; }
		CTextEffect operator|( const CTextEffect& right ) const { CTextEffect effect = *this; effect.CombineWith( right ); return effect; }

		void CombineWith( const CTextEffect& right );
	public:
		ui::TFontEffect m_fontEffect;
		COLORREF m_textColor;
		COLORREF m_bkColor;
	};
}


#endif // ui_fwd_h
