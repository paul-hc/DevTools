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
	enum StdPopup { AppMainPopup, AppDebugPopup, TextEditorPopup, HistoryComboPopup, ListView, DateTimePopup };

	enum FontEffect { Regular = 0, Bold = 1 << 0, Italic = 1 << 1, Underline = 1 << 2 };
	typedef int TFontEffect;


	inline int ToCmdId( UINT uCmdId ) { return (short)(unsigned short)( uCmdId ); }


	// conversion to/from state (0-based index); raw state (UINT) uses is 1-based indexes - works for list control, tree control
	typedef UINT RawCheckState;

	inline RawCheckState CheckStateToRaw( int checkState ) { return INDEXTOSTATEIMAGEMASK( checkState + 1 ); }
	inline int CheckStateFromRaw( RawCheckState rawCheckState )	{ return ( rawCheckState >> 12 ) - 1; }


	struct CNmHdr : public tagNMHDR
	{
		CNmHdr( const CWnd* pCtrl, int notifyCode )
		{
			hwndFrom = pCtrl->GetSafeHwnd();
			idFrom = pCtrl->GetDlgCtrlID();
			code = notifyCode;
		}

		LRESULT NotifyParent( void );						// returns 0 if handler did not reject the action (0 means unhandled or success; 1 means reject)
	};


	COLORREF AlterColorSlightly( COLORREF bkColor );		// slightly modified background colour used as transparent colour for bitmap masks, so that themes that render with alpha blending don't show weird colours (such as radio button)
}


#endif // ui_fwd_h
