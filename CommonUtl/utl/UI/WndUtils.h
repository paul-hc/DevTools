#ifndef WndUtils_h_UI
#define WndUtils_h_UI
#pragma once

#include "Dialog_fwd.h"
#include "Image_fwd.h"
#include "Range.h"
#include "ui_fwd.h"
#include "GdiCoords.h"
#include <unordered_map>


namespace num
{
	// forward declarations - required for C++ 14+ compilation
	const std::locale& GetEmptyLocale( void );
	std::tstring FormatNumber( double value, const std::locale& loc /*= GetEmptyLocale()*/ );

	template< typename ValueT >
	bool ParseNumber( ValueT& rNumber, const std::tstring& text, size_t* pSkipLength /*= nullptr*/, const std::locale& loc /*= GetEmptyLocale()*/ );
}


namespace ui
{
	int GetDrawTextAlignment( UINT dtFlags );
	void SetDrawTextAlignment( UINT* pDtFlags, Alignment alignment );


	// multiple monitors
	enum MonitorArea { Monitor, Workspace };	// Workspace means Desktop or work area

	CRect FindMonitorRect( HWND hWnd, MonitorArea area );
	CRect FindMonitorRectAt( const POINT& screenPoint, MonitorArea area );
	CRect FindMonitorRectAt( const RECT& screenRect, MonitorArea area );

	inline bool EnsureVisibleDesktopRect( CRect& rScreenRect, MonitorArea area = Workspace )
	{	// pull rScreenRect to the monitor desktop with most area
		return ui::EnsureVisibleRect( rScreenRect, FindMonitorRectAt( rScreenRect, area ) );
	}

	bool EnsureVisibleWindowRect( CRect& rWindowRect, HWND hWnd, bool clampToParent = true );

	CSize GetScreenSize( HWND hWnd = AfxGetMainWnd()->GetSafeHwnd(), MonitorArea area = ui::Monitor );


	CRect& AlignPopupWindowRect( CRect& rWindowRect, const RECT& excludeRect, ui::PopupAlign popupAlign, const CSize& spacing = CSize( 0, 0 ) );


	inline CSize GetTextSize( CDC* pDC, const TCHAR* pText, DWORD extraFormat = 0 )		// pass DT_EDITCONTROL for multi-line
	{	// more accurate, takes care of line-ends
		CRect textRect( 0, 0, 0, 0 );
		pDC->DrawText( pText, -1, &textRect, DT_CALCRECT | DT_LEFT | DT_VCENTER | extraFormat ); // more accurate, takes care of line-ends; DT_VCENTER ignored if multi-line
		return textRect.Size();
	}


	inline void FrameRect( HDC hDC, const RECT& rect, COLORREF color )
	{
		CBrush brush( color );
		::FrameRect( hDC, &rect, brush );
	}

	void FillRect( HDC hDC, const RECT& rect, COLORREF color );
	void FillRect( HDC hDC, const RECT& rect, HBRUSH hBrush );
}


namespace ui
{
	// forward declared in ui_fwd.h
	//CPoint GetCursorPos( HWND hWnd = nullptr );			// return screen coordinates if NULL


	// window coords

	inline CPoint ScreenToClient( HWND hWnd, const CPoint& screenPos )
	{
		CPoint clientPos = screenPos;
		::MapWindowPoints( HWND_DESKTOP, hWnd, &clientPos, 1 );			// more accurate than ::ScreenToClient - when using "Window Layout and Mirroring"
		return clientPos;
	}

	inline CPoint ClientToScreen( HWND hWnd, const CPoint& clientPos )
	{
		CPoint screenPos = clientPos;
		::MapWindowPoints( hWnd, HWND_DESKTOP, &screenPos, 1 );
		return screenPos;
	}

	inline bool ScreenToClient( HWND hWnd, RECT& rRect )
	{
		return
			::ScreenToClient( safe_ptr( hWnd ), reinterpret_cast<POINT*>( &rRect ) ) &&
			::ScreenToClient( hWnd, reinterpret_cast<POINT*>( &rRect ) + 1 );
	}

	inline bool ClientToScreen( HWND hWnd, RECT& rRect )
	{
		return
			::ClientToScreen( safe_ptr( hWnd ), reinterpret_cast<POINT*>( &rRect ) ) &&
			::ClientToScreen( hWnd, reinterpret_cast<POINT*>( &rRect ) + 1 );
	}

	void ScreenToNonClient( HWND hWnd, CRect& rRect );		// IN: screen coordiantes, OUT: non-client coordiantes, relative to CWindowDC( pWnd )
	void ClientToNonClient( HWND hWnd, CRect& rRect );		// IN: client coordiantes, OUT: non-client coordiantes, relative to CWindowDC( pWnd )
	CSize GetNonClientOffset( HWND hWnd );					// window-rect to client-rect offset
	CSize GetNonClientSize( HWND hWnd );					// window-rect size - client-rect size
	CRect GetWindowEdges( HWND hWnd );						// windowRect - clientRect

	CRect GetControlRect( HWND hCtrl );						// in parent's client coords
	inline CRect GetDlgItemRect( HWND hDlg, UINT ctrlId ) { return ui::GetControlRect( ::GetDlgItem( hDlg, ctrlId ) ); }

	void AlignWindow( HWND hWnd, HWND hAnchor, Alignment horz = H_AlignCenter, Alignment vert = V_AlignCenter, bool limitDest = false );
	void StretchWindow( HWND hWnd, HWND hAnchor, Stretch stretch, const CSize& inflateBy = CSize( 0, 0 ) );
	void OffsetWindow( HWND hWnd, int cx, int cy );
	void MoveControlOver( HWND hDlg, UINT sourceCtrlId, UINT destCtrlId, bool hideDestCtrl = true );

	CWnd* AlignToPlaceholder( CWnd* pCtrl, int placeholderId,
							  const CSize* pCustomSize = nullptr, TAlignment alignment = NoAlign, CSize addBottomRight = CSize( 0, 0 ) );

	inline bool SetTabOrder( CWnd* pCtrl, const CWnd* pPreviousCtrl )
	{	// move in tab order after previous control
		return pCtrl->SetWindowPos( pPreviousCtrl, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;
	}

	void RecalculateScrollBars( HWND hWnd );				// recalculates scrollbars and internal layout by forcing a resize


	struct CCtrlPlace
	{
		CCtrlPlace( HWND hCtrl = nullptr );
		CCtrlPlace( HWND hCtrl, const CRect& rect ) : m_hCtrl( hCtrl ), m_rect( rect ) {}

		UINT GetCtrlId( void ) const { ASSERT_PTR( m_hCtrl ); return ::GetDlgCtrlID( m_hCtrl ); }
	public:
		HWND m_hCtrl;
		CRect m_rect;			// for child windows: in parent's client coordinates; for top windows: screen rect
	};

	bool RepositionControls( const std::vector<CCtrlPlace>& ctrlPlaces, bool invalidate = true, UINT swpFlags = 0 );
}


namespace ui
{
	inline CToolTipCtrl* GetThreadTooltipCtrl( void ) { return AfxGetModuleThreadState()->m_pToolTip; }			// use when CWnd::EnableToolTips() was called (for a given window/dialog)
	void SetThreadTooltipCtrl( CToolTipCtrl* pTooltipCtrl );
}


namespace ui
{
	inline bool IsValidWindow( HWND hWnd ) { return hWnd != nullptr && ::IsWindow( hWnd ) != FALSE; }
	inline bool IsValidWindow( const CWnd* pWnd ) { return ui::IsValidWindow( pWnd->GetSafeHwnd() ); }

	DWORD GetWindowProcessId( HWND hWnd );

	inline DWORD GetStyle( HWND hWnd ) { ASSERT_PTR( hWnd ); return ::GetWindowLong( hWnd, GWL_STYLE ); }
	inline DWORD GetStyleEx( HWND hWnd ) { ASSERT_PTR( hWnd ); return ::GetWindowLong( hWnd, GWL_EXSTYLE ); }
	inline bool IsDisabled( HWND hWnd ) { ASSERT_PTR( hWnd ); return HasFlag( GetStyle( hWnd ), WS_DISABLED ); }
	inline bool IsVisible( HWND hWnd ) { ASSERT_PTR( hWnd ); return HasFlag( GetStyle( hWnd ), WS_VISIBLE ); }
	inline bool IsWindowVisible( HWND hWnd ) { ASSERT_PTR( hWnd ); return ::IsWindowVisible( hWnd ) != FALSE; }

	inline bool IsTransparent( HWND hWnd ) { ASSERT_PTR( hWnd ); return HasFlag( GetStyleEx( hWnd ), WS_EX_TRANSPARENT ); }
	bool SetTransparent( HWND hWnd, bool transparent = true );

	inline bool IsChild( HWND hWnd ) { return HasFlag( GetStyle( hWnd ), WS_CHILD ); }
	inline bool IsTopLevel( HWND hWnd ) { return !IsChild( hWnd ); }
	inline bool HasStyleTopLevelCaption( DWORD style ) { return !HasFlag( style, WS_CHILD ) && EqFlag( style, WS_CAPTION ); }	// WS_CAPTION=WS_BORDER|WS_DLGFRAME
	inline bool IsTopLevelCaption( HWND hWnd ) { return HasStyleTopLevelCaption( GetStyle( hWnd ) ); }
	inline bool IsPermanentWnd( HWND hWnd ) { return hWnd != nullptr && CWnd::FromHandlePermanent( hWnd ) != nullptr; }		// permanent: a window subclassed in this module


	HWND GetTopLevelParent( HWND hWnd );				// find first top-level parent (non-child)
	inline CWnd* GetTopLevelParent( const CWnd* pWnd ) { return CWnd::FromHandle( GetTopLevelParent( pWnd->GetSafeHwnd() ) ); }

	void GetWindowText( std::tstring& rText, HWND hWnd );
	inline std::tstring GetWindowText( HWND hWnd ) { std::tstring text; GetWindowText( text, hWnd ); return text; }		// no extra string copy
	inline std::tstring GetWindowText( const CWnd* pWnd ) { std::tstring text; GetWindowText( text, pWnd->GetSafeHwnd() ); return text; }

	inline std::tstring GetDlgItemText( HWND hDlg, UINT ctrlId ) { return ui::GetWindowText( ::GetDlgItem( hDlg, ctrlId ) ); }
	inline std::tstring GetDlgItemText( const CWnd* pDlg, UINT ctrlId ) { return ui::GetWindowText( ::GetDlgItem( pDlg->GetSafeHwnd(), ctrlId ) ); }

	bool SetWindowText( HWND hWnd, const std::tstring& text );

	inline bool SetDlgItemText( HWND hDlg, UINT ctrlId, const std::tstring& text ) { return ui::SetWindowText( ::GetDlgItem( hDlg, ctrlId ), text ); }
	inline bool SetDlgItemText( CWnd* pDlg, UINT ctrlId, const std::tstring& text ) { return SetDlgItemText( pDlg->GetSafeHwnd(), ctrlId, text ); }

	int GetDlgItemInt( HWND hDlg, UINT ctrlId, bool* pValid = nullptr );
	bool SetDlgItemInt( HWND hDlg, UINT ctrlId, int value );

	template< typename CtrlType >
	inline CtrlType* GetDlgItemAs( const CWnd* pDlg, UINT ctrlId )
	{
		CWnd* pCtrl = pDlg->GetDlgItem( ctrlId );
		ASSERT_PTR( pCtrl );
		return static_cast<CtrlType*>( pCtrl );
	}

	inline void GotoDlgItem( CDialog* pDlg, UINT ctrlId ) { pDlg->GotoDlgCtrl( pDlg->GetDlgItem( ctrlId ) ); }
	void GotoDlgCtrl( HWND hCtrl );


	bool EnableWindow( HWND hWnd, bool enable = true );
	inline bool EnableControl( HWND hDlg, UINT ctrlId, bool enable = true ) { return EnableWindow( ::GetDlgItem( hDlg, ctrlId ), enable ); }
	void EnableControls( HWND hDlg, const UINT ctrlIds[], size_t count, bool enable = true );

	bool ShowWindow( HWND hWnd, bool show = true, int showCmd = SW_SHOWNA );
	inline bool ShowControl( HWND hDlg, UINT ctrlId, bool show = true ) { return ShowWindow( ::GetDlgItem( hDlg, ctrlId ), show ); }
	void ShowControls( HWND hDlg, const UINT ctrlIds[], size_t count, bool show = true );

	bool RedrawWnd( HWND hWnd );				// works for both top level and child windows
	bool RedrawControl( HWND hCtrl );

	inline bool RedrawDialog( HWND hDlg, UINT moreFlags = RDW_UPDATENOW )
	{
		return ::RedrawWindow( hDlg, nullptr, nullptr, RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE | moreFlags ) != FALSE;
	}

	inline void RedrawDesktop( void ) { ::InvalidateRect( nullptr, nullptr, TRUE ); }		// invalidate desktop & all windows; redraws top level windows - slow, but works


	// Z-order

	inline bool BringWndToTop( HWND hWnd, bool top = true )		// move window to the top/bottom of Z-order (above other popup windows)
	{
		return ::SetWindowPos( hWnd, top ? HWND_TOP : HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER ) != FALSE;
	}

	inline bool BringWndToBottom( HWND hWnd ) { return BringWndToTop( hWnd, false ); }

	bool BringWndUp( HWND hWnd );
	bool BringWndDown( HWND hWnd );


	inline bool IsTopMost( HWND hWnd ) { return HasFlag( GetStyleEx( hWnd ), WS_EX_TOPMOST ); }
	bool SetTopMost( HWND hWnd, bool topMost = true );
	bool QueryTopLevelWindows( std::vector<HWND>& rTopWindows, DWORD styleFilter = WS_VISIBLE, DWORD dwThreadId = GetCurrentThreadId() );

	bool OwnsFocus( HWND hWnd );
	bool TakeFocus( HWND hWnd );
	bool TriggerInput( HWND hParent );					// trigger input if a modified edit is focused, i.e. uncommited

	bool IsEditLikeCtrl( HWND hCtrl );					// edit-box and combo-box?
	bool SelectAllText( CWnd* pCtrl );					// works for edit-box and combo-box

	bool RecreateControl( CWnd* pCtrl, DWORD newStyle, DWORD newStyleEx = -1 );


	bool IsCommandEnabled( CCmdTarget* pCmdTarget, UINT cmdId );	// check via CN_UPDATE_COMMAND_UI handler
	bool HandleCommand( CCmdTarget* pCmdTarget, UINT cmdId );		// menu or accelerator

	inline LRESULT SendCommand( HWND hTargetWnd, UINT cmdId, int notifCode = BN_CLICKED, HWND hCtrl = nullptr )
	{
		ASSERT_PTR( hTargetWnd );
		return ::SendMessage( hTargetWnd, WM_COMMAND, MAKEWPARAM( cmdId, notifCode ), (LPARAM)hCtrl );
	}

	inline void SendCommandToParent( HWND hCtrl, int notifCode = BN_CLICKED )
	{
		ASSERT( hCtrl != nullptr && ::GetParent( hCtrl ) != nullptr );
		::SendMessage( ::GetParent( hCtrl ),
					   WM_COMMAND, MAKEWPARAM( ::GetDlgCtrlID( hCtrl ), notifCode ),
					   (LPARAM)hCtrl );
	}

	inline void PostCommandToParent( HWND hCtrl, int notifCode = BN_CLICKED )
	{
		ASSERT( hCtrl != nullptr && ::GetParent( hCtrl ) != nullptr );
		::PostMessage( ::GetParent( hCtrl ),
					   WM_COMMAND, MAKEWPARAM( ::GetDlgCtrlID( hCtrl ), notifCode ),
					   (LPARAM)hCtrl );
	}

	inline LRESULT SendNotifyToParent( HWND hCtrl, int notifCode, NMHDR* pNmHdr )
	{
		ASSERT( hCtrl != nullptr && ::GetParent( hCtrl ) != nullptr );
		ASSERT_PTR( pNmHdr );
		pNmHdr->code = notifCode;
		pNmHdr->idFrom = ::GetDlgCtrlID( hCtrl );
		pNmHdr->hwndFrom = hCtrl;
		return ::SendMessage( ::GetParent( hCtrl ), WM_NOTIFY, pNmHdr->idFrom, (LPARAM)pNmHdr );
	}


	HBRUSH SendCtlColor( HWND hWnd, HDC hDC, UINT message = WM_CTLCOLORSTATIC );

	const AFX_MSGMAP_ENTRY* FindMessageHandler( const CCmdTarget* pCmdTarget, UINT message, UINT notifyCode, UINT id );

	inline bool ContainsMessageHandler( const CCmdTarget* pCmdTarget, UINT message, UINT notifyCode, UINT id )
	{
		return FindMessageHandler( pCmdTarget, message, notifyCode, id ) != nullptr;
	}

	inline bool ParentContainsMessageHandler( const CWnd* pChild, UINT message, UINT notifyCode )
	{
		return ContainsMessageHandler( pChild->GetParent(), message, notifyCode, pChild->GetDlgCtrlID() );
	}


	std::tstring GetClassName( HWND hWnd );
	bool IsEditBox( HWND hCtrl );
	bool IsWriteableEditBox( HWND hCtrl );
	bool IsComboWithEdit( HWND hCtrl );
	bool IsGroupBox( HWND hWnd );
	bool IsDialogBox( HWND hWnd );
	bool IsMenuWnd( HWND hWnd );

	bool ModifyBorder( CWnd* pWnd, bool useBorder = true );

	template< typename WndType >
	WndType* FindAncestorAs( const CWnd* pWnd )
	{
		ASSERT_PTR( pWnd );

		for ( CWnd* pParent = const_cast<CWnd*>( pWnd ); ( pParent = pParent->GetParent() ) != nullptr; )
			if ( WndType* pParentAsType = dynamic_cast<WndType*>( pParent ) )
				return pParentAsType;

		return nullptr;
	}

	template< typename WndType >
	inline WndType* LookupAncestorAs( const CWnd* pWnd ) { return safe_ptr( FindAncestorAs<WndType>( pWnd ) ); }

} //namespace ui


namespace ui
{
	inline int MessageBox( const std::tstring& message, UINT mbFlags = MB_OK ) { return ::AfxMessageBox( message.c_str(), mbFlags ); }

	bool BeepSignal( UINT beep = MB_OK );														// returns false for convenience

	bool ReportError( const std::tstring& message, UINT mbFlags = MB_OK | MB_ICONERROR );		// returns false for convenience
	int ReportException( const std::exception& exc, UINT mbFlags = MB_OK | MB_ICONERROR );
	int ReportException( const CException* pExc, UINT mbFlags = MB_OK | MB_ICONERROR );

	bool ShowInputError( CWnd* pCtrl, const std::tstring& message, UINT iconFlag = MB_ICONERROR );		// returns false for convenience

	bool& RefAsyncApiEnabled( void );


	bool EditColor( COLORREF* pColor IN OUT, CWnd* pParentWnd, bool popupDlg, ui::PopupAlign popupAlign = ui::DropDown );

	// color dialog style registry persist
	ui::ColorDialogStyle LoadColorDialogStyle( void );
	void SaveColorDialogStyle( ui::ColorDialogStyle dlgStyle );
}


#include <vector>
#include "utl/SubjectPredicates.h"


class CPathItemBase;
namespace pred { interface IComparator; }
namespace num { const std::locale& GetEmptyLocale( void ); }
namespace fs { class CPath; }


namespace ui
{
	void SetSpinRange( CWnd* pDlg, int ctrlId, int minValue, int maxValue );


	HTREEITEM FindTreeItem( const CTreeCtrl& treeCtrl, const std::tstring& itemText, HTREEITEM hParent = TVI_ROOT );
	bool SortTreeChildren( const pred::IComparator* pComparator, CTreeCtrl& rTreeCtrl, HTREEITEM hParent = TVI_ROOT, RecursionDepth depth = Shallow );

	template< typename Compare >
	bool SortCompareTreeChildren( Compare compare, CTreeCtrl& rTreeCtrl, HTREEITEM hParent = TVI_ROOT, RecursionDepth depth = Shallow )
	{
		pred::Comparator<Compare> comparator( compare );
		return SortTreeChildren( &comparator, rTreeCtrl, hParent, depth );
	}


	int FindListItem( const CComboBox& combo, const TCHAR* pItemText, str::CaseType caseType = str::Case );

	template< typename ComboType >
	inline std::tstring GetComboItemText( const ComboType& combo, int pos )
	{
		CString itemText;
		combo.GetLBText( pos, itemText );
		return itemText.GetString();
	}

	inline std::tstring GetListItemText( const CListBox& listBox, int pos )
	{
		CString itemText;
		listBox.GetText( pos, itemText );
		return itemText.GetString();
	}

	void ReadListBoxItems( std::vector<std::tstring>& rOutItems, const CListBox& listBox );
	void WriteListBoxItems( CListBox& rListBox, const std::vector<std::tstring>& items );

	void ReadComboItems( std::vector<std::tstring>& rOutItems, const CComboBox& combo );
	void WriteComboItems( CComboBox& rCombo, const std::vector<std::tstring>& items );

	template< typename ValueIterator_T, typename FormatFunc >
	void WriteComboItemValues( CComboBox& rCombo, ValueIterator_T itStartValue, ValueIterator_T itEndValue, FormatFunc formatFunc )
	{
		rCombo.ResetContent();
		for ( ; itStartValue != itEndValue; ++itStartValue )
			rCombo.AddString( formatFunc( *itStartValue ).c_str() );
	}

	template< typename Container_T, typename FormatFunc >
	inline void WriteComboItemValues( CComboBox& rCombo, const Container_T& itemValues, FormatFunc formatFunc )
	{
		WriteComboItemValues( rCombo, itemValues.begin(), itemValues.end(), formatFunc );
	}

	CEdit* GetComboEdit( const CComboBox& rCombo );
	CWnd* GetComboDropList( const CComboBox& rCombo );

	std::tstring GetComboSelText( const CComboBox& rCombo, ui::ComboField byField = ui::BySel );
	std::pair<bool, ui::ComboField> SetComboEditText( CComboBox& rCombo, const std::tstring& currText, str::CaseType caseType = str::Case );		// <changed, by_field>
	std::pair<bool, ui::ComboField> ReplaceComboEditText( CComboBox& rCombo, const std::tstring& currText, str::CaseType caseType = str::Case );

	void UpdateHistoryCombo( CComboBox& rCombo, size_t maxCount, str::CaseType caseType = str::Case );

	void LoadHistoryCombo( CComboBox& rHistoryCombo, const TCHAR* pSection, const TCHAR* pEntry,
						   const TCHAR* pDefaultText, const TCHAR* pSep = _T(";") );
	void SaveHistoryCombo( CComboBox& rHistoryCombo, const TCHAR* pSection, const TCHAR* pEntry,
						   const TCHAR* pSep = _T(";"), size_t maxCount = HistoryMaxSize, str::CaseType caseType = str::Case );

	std::tstring LoadHistorySelItem( const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText, const TCHAR* pSep = _T(";") );


	struct CFontInfo
	{
		CFontInfo( TFontEffect effect = Regular, int heightPct = 100 )
			: m_pFaceName( nullptr ), m_effect( effect ), m_heightPct( heightPct ) {}

		CFontInfo( const TCHAR* pFaceName, TFontEffect effect = Regular, int heightPct = 100 )
			: m_pFaceName( pFaceName ), m_effect( effect ), m_heightPct( heightPct ) {}
	public:
		const TCHAR* m_pFaceName;
		TFontEffect m_effect;
		int m_heightPct;
	};

	void MakeStandardControlFont( CFont& rOutFont, const ui::CFontInfo& fontInfo = ui::CFontInfo(), int stockFontType = DEFAULT_GUI_FONT );
	void MakeEffectControlFont( CFont& rOutFont, CFont* pSourceFont, TFontEffect fontEffect = ui::Regular, int heightPct = 100 );


	class CFontEffectCache
	{
	public:
		CFontEffectCache( CFont* pSourceFont );
		~CFontEffectCache();

		CFont* Lookup( TFontEffect fontEffect );
	private:
		CFont m_sourceFont;
		std::unordered_map<TFontEffect, CFont*> m_effectFonts;
	};

	inline COLORREF GetInverseColor( COLORREF color ) { return RGB( 255 - GetRValue( color ), 255 - GetGValue( color ), 255 - GetBValue( color ) ); }
	void AddSysColors( std::vector<COLORREF>& rColors, const int sysIndexes[], size_t count );


	bool PumpPendingMessages( HWND hWnd = nullptr );
	bool EatPendingMessages( HWND hWnd = nullptr, UINT minMessage = WM_TIMER, UINT maxMessage = WM_TIMER );
}


#include "utl/EnumTags.h"
#include "UserReport.h"


namespace ui
{
	namespace ddx
	{
		void FailInput( CDataExchange* pDX, UINT ctrlId, const std::tstring& validationError );

		// combo-friendly control IO
		std::tstring GetItemText( CDataExchange* pDX, UINT ctrlId );
		bool SetItemText( CDataExchange* pDX, UINT ctrlId, const std::tstring& text );
	}


	// DDX: dialog data exchange

	void DDX_Text( CDataExchange* pDX, int ctrlId, std::tstring& rValue, bool trim = false );		// Edit, Static, Combo, etc
	void DDX_Path( CDataExchange* pDX, int ctrlId, fs::CPath& rValue );
	void DDX_PathItem( CDataExchange* pDX, int ctrlId, CPathItemBase* pPathItem );

	void DDX_Int( CDataExchange* pDX, int ctrlId, int& rValue, const int nullValue = INT_MAX );		// empty text for nullValue
	void DDX_Bool( CDataExchange* pDX, int ctrlId, bool& rValue );
	void DDX_BoolRadio( CDataExchange* pDX, int radioFirstId, bool& rValue, bool firstRadioIsTrue );
	void DDX_Flag( CDataExchange* pDX, int ctrlId, int& rValue, int flag );
	inline void DDX_Flag( CDataExchange* pDX, int ctrlId, UINT& rValue, UINT flag ) { DDX_Flag( pDX, ctrlId, (int&)rValue, flag ); }

	void DDX_ButtonIcon( CDataExchange* pDX, int ctrlId, const CIconId& iconId = CIconId( 0 ), bool useText = true, bool useTextSpacing = true );
	void DDX_StaticIcon( CDataExchange* pDX, int ctrlId, const CIconId& iconId = CIconId( 0 ) );

	template< typename EnumType >
	inline void DDX_RadioEnum( CDataExchange* pDX, int radioFirstId, EnumType& rValue, int baseValue = 0 )		// works for bool when radioFirstId = false
	{
		// offset by enum base value
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			int selIndex = rValue - baseValue;
			::DDX_Radio( pDX, radioFirstId, selIndex );
		}
		else
		{
			int selIndex;
			::DDX_Radio( pDX, radioFirstId, selIndex );
			rValue = static_cast<EnumType>( selIndex + baseValue );
		}
	}

	template< typename NumericType >
	void DDX_Number( CDataExchange* pDX, int ctrlId, NumericType& rValue, const std::locale& loc = num::GetEmptyLocale() )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ddx::SetItemText( pDX, ctrlId, num::FormatNumber( rValue, loc ) );
		else
			num::ParseNumber( rValue, ddx::GetItemText( pDX, ctrlId ), nullptr, loc );
	}

	inline void DDX_ComboSelPos( CDataExchange* pDX, int comboId, size_t& rSelPos )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			int selIndex = static_cast<int>( rSelPos );
			DDX_CBIndex( pDX, comboId, selIndex );
		}
		else
		{
			int selIndex;
			DDX_CBIndex( pDX, comboId, selIndex );
			rSelPos = static_cast<size_t>( selIndex );
		}
	}

	template< typename EnumType >
	inline void DDX_EnumSelValue( CDataExchange* pDX, int comboId, EnumType& rValue )
	{
		DDX_CBIndex( pDX, comboId, (int&)rValue );
	}

	template< typename EnumType >
	void DDX_EnumCombo( CDataExchange* pDX, int comboId, CComboBox& rCombo, EnumType& rValue, const CEnumTags& enumTags )
	{
		bool firstInit = nullptr == rCombo.m_hWnd;
		::DDX_Control( pDX, comboId, rCombo );
		if ( firstInit )
		{
			ASSERT( DialogOutput == pDX->m_bSaveAndValidate );
			ui::WriteComboItems( rCombo, enumTags.GetUiTags() );
		}

		// offset by enum base value
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			int selIndex = enumTags.GetTagIndex( rValue );
			DDX_CBIndex( pDX, comboId, selIndex );
		}
		else
		{
			int selIndex;
			DDX_CBIndex( pDX, comboId, selIndex );
			rValue = enumTags.GetSelValue<EnumType>( selIndex );
		}
	}

	template< typename IntType >
	void DDX_EditSel( CDataExchange* pDX, int ctrlId, Range<IntType>& rValue )
	{
		CEdit* pEdit = GetDlgItemAs<CEdit>( pDX->m_pDlgWnd, ctrlId );
		ASSERT_PTR( pEdit );
		if ( DialogOutput == pDX->m_bSaveAndValidate )
			pEdit->SetSel( rValue.m_start, rValue.m_end );
		else
		{
			int start, end;
			pEdit->GetSel( start, end );
			rValue.SetRange( start, end );
		}
	}


	// DDV: dialog data validation

	template< typename NumericType >
	void DDV_NumberRange( CDataExchange* pDX, int ctrlId, NumericType& rValue, const Range<NumericType>& validRange )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
			return;
		if ( !validRange.Constrain( rValue ) )
			return;									// valid number, OK

		if ( CWnd* pCtrl = pDX->m_pDlgWnd->GetDlgItem( ctrlId ) )
		{
			checked_static_cast<CDialog*>( pDX->m_pDlgWnd )->GotoDlgCtrl( pCtrl );
			pDX->m_idLastControl = 0;
		}
		else
			pDX->m_idLastControl = ctrlId;

		ui::CInteractiveMode::Instance().MessageBox(
			str::Format( _T("Enter a valid number between %s and %s"), num::FormatNumber( validRange.m_start ).c_str(), num::FormatNumber( validRange.m_end ).c_str() ),
			MB_OK | MB_ICONWARNING );

		pDX->Fail();
	}

	template< typename NumericType, typename MinMaxType >
	inline void DDV_NumberMinMax( CDataExchange* pDX, int ctrlId, NumericType& rValue, MinMaxType minValue, MinMaxType maxValue )		// MinMaxType is enum friendly
	{
		DDV_NumberRange( pDX, ctrlId, rValue, Range<NumericType>( minValue, maxValue ) );
	}
}


#endif // WndUtils_h_UI
