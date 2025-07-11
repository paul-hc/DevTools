#ifndef ui_fwd_h
#define ui_fwd_h
#pragma once

#include "ui_types.h"


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

typedef int TAlignment;


namespace ui
{
	enum StretchMode { OriginalSize, StretchFit, ShrinkFit };
}


#define ON_CN_INPUTERROR( id, memberFxn )		ON_CONTROL( ui::CN_INPUTERROR, id, memberFxn )


namespace ui
{
	enum ComboField { BySel, ByEdit };

	enum PopupAlign { DropRight, DropDown, DropLeft, DropUp };
	enum { HistoryMaxSize = 20 };

	enum StdContext			// IDR_STD_CONTEXT_MENU popups
	{
		AppMainPopup, AppSysTray, AppDebugPopup, TextEditorPopup, HistoryComboPopup,
		ListView, DateTimePopup, ColorPickerPopup, ColorPopupDialog,
		TestPopup
	};

	enum StdPopups			// IDR_STD_POPUPS_MENU popups
	{
		AppLookPopup
	};

	enum FontEffect { Regular = 0, Bold = 1 << 0, Italic = 1 << 1, Underline = 1 << 2 };
	typedef int TFontEffect;

	enum ColorDialogStyle { OfficeColorDialog, ClassicColorDialog };


	struct CCmdAlias { UINT m_cmdId, m_imageCmdId; };

	enum { MinCmdId = 0, MaxCmdId = 0xFFFF, MinAppCmdId = 1, MaxAppCmdId = 0x7FFF, AtEnd = -1 };

	inline int ToIntCmdId( UINT uCmdId ) { return (short)(unsigned short)( uCmdId ); }


	CPoint GetCursorPos( HWND hWnd = nullptr );			// return screen coordinates if NULL
	const CPoint& GetNullPos( void );

	// works for VK_CONTROL, VK_SHIFT, VK_MENU (alt), VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, etc
	inline bool IsKeyPressed( int virtKey ) { return ::GetKeyState( virtKey ) < 0; }


	// conversion to/from state (0-based index); raw state (UINT) uses is 1-based indexes - works for list control, tree control
	typedef UINT TRawCheckState;

	inline TRawCheckState CheckStateToRaw( int checkState ) { return INDEXTOSTATEIMAGEMASK( checkState + 1 ); }
	inline int CheckStateFromRaw( TRawCheckState rawCheckState )	{ return ( rawCheckState >> 12 ) - 1; }


	// standard notifications
	enum NotifCode { CN_INPUTERROR = CBN_SELENDCANCEL + 20 };		// note: notifications are suppressed during parent's UpdateData()


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


class CBalloonHostWnd;


namespace ui
{
	// baloon tips (modeless)
	void RequestCloseAllBalloons( void );		// close the balloon quickly

	CBalloonHostWnd* ShowBalloonTip( const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon = TTI_NONE, const CPoint& screenPos = GetNullPos() );
	CBalloonHostWnd* ShowBalloonTip( const CWnd* pCtrl, const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon = TTI_NONE );

	// show modeless balloon if available, otherwise modal ui::MessageBox
	CBalloonHostWnd* SafeShowBalloonTip( UINT mbStyle, const TCHAR* pTitle, const std::tstring& message, CWnd* pCtrl );
}


#endif // ui_fwd_h
