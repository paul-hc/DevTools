
#include "stdafx.h"
#include "Utilities.h"
#include "LayoutEngine.h"
#include "HistoryComboBox.h"
#include "IconButton.h"
#include "ImageStore.h"
#include "utl/ContainerUtilities.h"
#include "utl/Path.h"
#include "utl/PathItemBase.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/SubjectPredicates.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	int GetDrawTextAlignment( UINT dtFlags )
	{
		int align = NoAlign;

		if ( HasFlag( dtFlags, DT_CENTER ) )
			SetFlag( align, H_AlignCenter );
		else if ( HasFlag( dtFlags, DT_RIGHT ) )
			SetFlag( align, H_AlignRight );
		else
			SetFlag( align, H_AlignLeft );

		if ( HasFlag( dtFlags, DT_VCENTER ) )
			SetFlag( align, V_AlignCenter );
		else if ( HasFlag( dtFlags, DT_BOTTOM ) )
			SetFlag( align, V_AlignBottom );
		else
			SetFlag( align, V_AlignTop );

		return align;
	}

	void SetDrawTextAlignment( UINT* pDtFlags, Alignment alignment )
	{
		ASSERT_PTR( pDtFlags );

		UINT dtAlign = 0;

		if ( HasFlag( alignment, H_AlignCenter ) )
			SetFlag( dtAlign, DT_CENTER );
		else if ( HasFlag( alignment, H_AlignRight ) )
			SetFlag( dtAlign, DT_RIGHT );

		if ( HasFlag( alignment, V_AlignCenter ) )
			SetFlag( dtAlign, DT_VCENTER );
		else if ( HasFlag( alignment, V_AlignBottom ) )
			SetFlag( dtAlign, DT_BOTTOM );

		ModifyFlags( *pDtFlags, DT_CENTER | DT_RIGHT | DT_VCENTER | DT_BOTTOM, dtAlign );
	}


	CRect FindMonitorRect( HWND hWnd, MonitorArea area )
	{
		if ( HMONITOR hMonitor = ::MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi; mi.cbSize = sizeof( MONITORINFO );
			if ( ::GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );			// fallback to the main screen desktop rect
		return desktopRect;
	}

	CRect FindMonitorRectAt( const POINT& screenPoint, MonitorArea area )
	{
		if ( HMONITOR hMonitor = ::MonitorFromPoint( screenPoint, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi; mi.cbSize = sizeof( MONITORINFO );
			if ( ::GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );				// fallback to the main screen desktop rect
		return desktopRect;
	}

	CRect FindMonitorRectAt( const RECT& screenRect, MonitorArea area )
	{
		if ( HMONITOR hMonitor = MonitorFromRect( &screenRect, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi = { sizeof( MONITORINFO ) };
			if ( GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );				// fallback to the main screen desktop rect
		return desktopRect;
	}

	bool EnsureVisibleWindowRect( CRect& rWindowRect, HWND hWnd, bool clampToParent /*= true*/ )
	{
		ASSERT_PTR( hWnd );
		CRect parentRect;

		if ( IsChild( hWnd ) )
			::GetClientRect( ::GetParent( hWnd ), &parentRect );			// assuming rWindowRect is in parent's coords, too
		else
			parentRect = FindMonitorRect( hWnd, Workspace );

		const CRect oldRect = rWindowRect;
		ui::EnsureVisibleRect( rWindowRect, parentRect );
		if ( clampToParent )
			rWindowRect &= parentRect;

		return !( rWindowRect == oldRect );									// changed?
	}

	CSize GetScreenSize( HWND hWnd /*= AfxGetMainWnd()->GetSafeHwnd()*/, MonitorArea area /*= ui::Monitor*/ )
	{
		return ui::FindMonitorRect( hWnd, area ).Size();
	}
}


namespace ui
{
	CPoint GetCursorPos( HWND hWnd /*= NULL*/ )
	{
		CPoint mousePos;
		::GetCursorPos( &mousePos );
		if ( hWnd != NULL )
			::ScreenToClient( hWnd, &mousePos );			// convert to client coords
		return mousePos;
	}


	CSize GetNonClientOffset( HWND hWnd )
	{
		CRect windowRect, clientRect;
		::GetWindowRect( hWnd, &windowRect );
		::GetClientRect( hWnd, &clientRect );
		ClientToScreen( hWnd, clientRect );
		return clientRect.TopLeft() - windowRect.TopLeft();
	}

	CSize GetNonClientSize( HWND hWnd )
	{
		CRect windowRect, clientRect;
		::GetWindowRect( hWnd, &windowRect );
		::GetClientRect( hWnd, &clientRect );
		ClientToScreen( hWnd, clientRect );
		return windowRect.Size() - clientRect.Size();
	}

	void ScreenToNonClient( HWND hWnd, CRect& rRect )	// IN: screen coordiantes, OUT: non-client coordiantes, relative to CWindowDC( pWnd )
	{
		ScreenToClient( hWnd, rRect );

		CSize clientOffset = GetNonClientOffset( hWnd );
		rRect.OffsetRect( clientOffset );
	}

	void ClientToNonClient( HWND hWnd, CRect& rRect )
	{
		ClientToScreen( hWnd, rRect );
		ScreenToNonClient( hWnd, rRect );
	}

	CRect GetWindowEdges( HWND hWnd )
	{
		CRect windowRect, clientRect;
		::GetWindowRect( hWnd, &windowRect );
		::GetClientRect( hWnd, &clientRect );
		ui::ClientToScreen( hWnd, clientRect );
		windowRect.DeflateRect( &clientRect );
		return windowRect;									// edges: window_rect - client_rect
	}

	CRect GetControlRect( HWND hCtrl )
	{
		ASSERT_PTR( hCtrl );
		CRect controlRect;
		::GetWindowRect( hCtrl, &controlRect );

		if ( HasFlag( GetStyle( hCtrl ), WS_CHILD ) )
			if ( HWND hParent = ::GetParent( hCtrl ) )
				ui::ScreenToClient( hParent, controlRect );		// convert to parent's coords

		return controlRect;
	}

	void AlignWindow( HWND hWnd, HWND hAnchor, Alignment horz /*= H_AlignCenter*/, Alignment vert /*= V_AlignCenter*/, bool limitDest /*= false*/ )
	{
		ASSERT_PTR( hWnd );
		CRect targetRect, anchorRect;
		::GetWindowRect( hWnd, &targetRect );
		::GetWindowRect( hAnchor, &anchorRect );

		AlignRectHV( targetRect, anchorRect, horz, vert, limitDest );

		if ( GetStyle( hWnd ) & WS_CHILD )
			ScreenToClient( GetParent( hWnd ), targetRect );

		::MoveWindow( hWnd, targetRect.left, targetRect.top, targetRect.Width(), targetRect.Height(), TRUE );
	}

	void StretchWindow( HWND hWnd, HWND hAnchor, Stretch stretch, const CSize& inflateBy /*= CSize( 0, 0 )*/ )
	{
		ASSERT_PTR( hWnd );
		CRect targetRect, anchorRect;
		::GetWindowRect( hWnd, &targetRect );
		::GetWindowRect( hAnchor, &anchorRect );

		StretchRect( targetRect, anchorRect, stretch );
		targetRect.InflateRect( inflateBy );

		if ( GetStyle( hWnd ) & WS_CHILD )
			ScreenToClient( GetParent( hWnd ), targetRect );

		::MoveWindow( hWnd, targetRect.left, targetRect.top, targetRect.Width(), targetRect.Height(), TRUE );
	}

	void OffsetWindow( HWND hWnd, int cx, int cy )
	{
		ASSERT_PTR( hWnd );
		CRect targetRect;
		::GetWindowRect( hWnd, &targetRect );

		targetRect.OffsetRect( cx, cy );

		if ( GetStyle( hWnd ) & WS_CHILD )
			ScreenToClient( GetParent( hWnd ), targetRect );

		::MoveWindow( hWnd, targetRect.left, targetRect.top, targetRect.Width(), targetRect.Height(), TRUE );
	}

	void MoveControlOver( HWND hDlg, UINT sourceCtrlId, UINT destCtrlId, bool hideDestCtrl /*= true*/ )
	{
		HWND hDestCtrl = ::GetDlgItem( hDlg, destCtrlId ), hSourceCtrl = ::GetDlgItem( hDlg, sourceCtrlId );
		ASSERT( hDestCtrl != NULL && hSourceCtrl != NULL );

		CRect destRect, sourceRect;
		::GetWindowRect( hDlg, &destRect );
		GetWindowRect( hSourceCtrl, &sourceRect );
		ScreenToClient( hDlg, destRect );
		ScreenToClient( hDlg, sourceRect );

		sourceRect.OffsetRect( destRect.TopLeft() - sourceRect.TopLeft() );
		::MoveWindow( hSourceCtrl, sourceRect.left, sourceRect.top, sourceRect.Width(), sourceRect.Height(), TRUE );

		if ( hideDestCtrl )
			ShowWindow( hDestCtrl, false );
	}

	CWnd* AlignToPlaceholder( CWnd* pCtrl, int placeholderId,
							  const CSize* pCustomSize /*= NULL*/, TAlignment alignment /*= NoAlign*/, CSize addBottomRight /*= CSize( 0, 0 )*/ )
	{
		ASSERT_PTR( pCtrl->GetSafeHwnd() );

		CWnd* pParentWnd = pCtrl->GetParent();
		CWnd* pPlaceHolderWnd = pParentWnd->GetDlgItem( placeholderId );

		CRect placeholderRect;
		pPlaceHolderWnd->GetWindowRect( &placeholderRect );
		pParentWnd->ScreenToClient( &placeholderRect );

		CRect ctrlRect = placeholderRect;
		if ( pCustomSize != NULL )
			ctrlRect.BottomRight() = ctrlRect.TopLeft() + *pCustomSize;

		ctrlRect.BottomRight() += addBottomRight;

		if ( alignment != NoAlign )
			ui::AlignRect( ctrlRect, placeholderRect, alignment );

		pCtrl->SetWindowPos( pPlaceHolderWnd, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), SWP_NOACTIVATE );
		return pPlaceHolderWnd;
	}


	void RecalculateScrollBars( HWND hWnd )
	{
		ASSERT_PTR( hWnd );

		CRect windowRect;
		::GetWindowRect( hWnd, &windowRect );

		enum { Flags = SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER };

		::SetWindowPos( hWnd, NULL, 0, 0, windowRect.Width() - 1, windowRect.Height() - 1, Flags | SWP_NOSENDCHANGING );		// shrink it by 1 pixel
		::SetWindowPos( hWnd, NULL, 0, 0, windowRect.Width(), windowRect.Height(), Flags );				// restore original size

		::InvalidateRect( hWnd, NULL, TRUE );
	}


	CCtrlPlace::CCtrlPlace( HWND hCtrl /*= NULL*/ )
		: m_hCtrl( hCtrl )
	{
		if ( m_hCtrl != NULL )
			m_rect = ui::GetControlRect( m_hCtrl );
	}

	bool RepositionControls( const std::vector< CCtrlPlace >& ctrlPlaces, bool invalidate /*= true*/, UINT swpFlags /*= 0*/ )
	{
		HDWP hdwp = ::BeginDeferWindowPos( static_cast<int>( ctrlPlaces.size() ) );
		std::vector< CCtrlPlace >::const_iterator itCtrl, itEnd = ctrlPlaces.end();

		for ( itCtrl = ctrlPlaces.begin(); itCtrl != itEnd && hdwp != NULL; ++itCtrl )
			hdwp = ::DeferWindowPos( hdwp, itCtrl->m_hCtrl, NULL,
				itCtrl->m_rect.left, itCtrl->m_rect.top, itCtrl->m_rect.Width(), itCtrl->m_rect.Height(),
				SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | swpFlags );

		if ( NULL == hdwp || !::EndDeferWindowPos( hdwp ) )
			return false;

		if ( invalidate )
			for ( itCtrl = ctrlPlaces.begin(); itCtrl != itEnd; ++itCtrl )
				::InvalidateRect( itCtrl->m_hCtrl, NULL, TRUE );

		return true;
	}
}


namespace ui
{
	void SetThreadTooltipCtrl( CToolTipCtrl* pTooltipCtrl )
	{
		CToolTipCtrl* pOldTooltipCtrl = ui::GetThreadTooltipCtrl();

		if ( pTooltipCtrl != pOldTooltipCtrl && pOldTooltipCtrl != NULL )
		{
			pOldTooltipCtrl->DestroyWindow();
			delete pOldTooltipCtrl;
		}

		if ( pTooltipCtrl->GetSafeHwnd() != NULL )
			pTooltipCtrl->SendMessage( TTM_ACTIVATE, false );

		AfxGetModuleThreadState()->m_pToolTip = pTooltipCtrl;
	}
}


namespace ui
{
	HWND GetTopLevelParent( HWND hWnd )
	{
		while ( hWnd != NULL )
			if ( ui::IsChild( hWnd ) )
				hWnd = ::GetParent( hWnd );
			else
				break;

		return hWnd;
	}

	CWnd* FindTopParentPermanent( HWND hWnd )		// find first parent (non-child) that is a permanent window (subclassed in this module)
	{	// useful in shell extension DLLd to distinguish from windows created by the owner application
		ASSERT_PTR( hWnd );

		for ( CWnd* pParentPerm = CWnd::FromHandlePermanent( hWnd ); pParentPerm != NULL; )
			if ( ui::IsTopLevel( pParentPerm->GetSafeHwnd() ) )
				return pParentPerm;					// found top-level parent (non-child)
			else
				if ( CWnd* pParent = CWnd::FromHandlePermanent( ::GetParent( pParentPerm->GetSafeHwnd() ) ) )
					pParentPerm = pParent;
				else
					return pParentPerm;				// found top child window that's permanent

		return NULL;
	}

	DWORD GetWindowProcessId( HWND hWnd )
	{
		ASSERT_PTR( hWnd );
		DWORD processId = 0;
		::GetWindowThreadProcessId( hWnd, &processId );
		return processId;
	}

	void _GetWindowText( std::tstring& rText, HWND hWnd )
	{
		int length = ::GetWindowTextLength( hWnd );
		std::vector< TCHAR > buffer( length + 1 );
		TCHAR* pBuffer = &buffer.front();

		::GetWindowText( hWnd, pBuffer, length + 1 );
		buffer[ length ] = 0;
		rText = pBuffer;
	}

	void _GetChildWindowText( std::tstring& rText, HWND hWnd )
	{
		// note: GetWindowTextLength() and GetWindowText() fail for edits running in a different process
		// we use WM_GETTEXTLENGTH and WM_GETTEXT instead

		int length = static_cast<int>( ::SendMessage( hWnd, WM_GETTEXTLENGTH, 0, 0 ) );	// ::GetWindowTextLength( hWnd );
		std::vector< TCHAR > buffer( length + 1 );
		TCHAR* pBuffer = &buffer.front();

		::SendMessage( hWnd, WM_GETTEXT, length + 1, (LPARAM)pBuffer );						// ::GetWindowText( hWnd, pBuffer, length + 1 );
		buffer[ length ] = 0;
		rText = pBuffer;
	}

	void GetWindowText( std::tstring& rText, HWND hWnd )
	{
		if ( IsChild( hWnd ) )
			_GetChildWindowText( rText, hWnd );
		else
			_GetWindowText( rText, hWnd );
	}

	bool SetWindowText( HWND hWnd, const std::tstring& text )
	{
		ASSERT_PTR( hWnd );

		size_t textLength = text.length();
		TCHAR oldTextBuffer[ 512 ];

		// fast check to see if text really changes (reduces flash in controls)
		if ( textLength > _countof( oldTextBuffer ) ||
			 ::GetWindowText( hWnd, oldTextBuffer, (int)_countof( oldTextBuffer ) ) != (int)textLength ||
			 _tcscmp( oldTextBuffer, text.c_str() ) != pred::Equal )
		{
			::SetWindowText( hWnd, text.c_str() );
			return true;			// text has changed
		}

		return false;
	}

	int GetDlgItemInt( HWND hDlg, UINT ctrlId, bool* pValid /*= NULL*/ )
	{
		BOOL valid;
		int result = static_cast<int>( ::GetDlgItemInt( hDlg, ctrlId, &valid, TRUE ) );
		if ( pValid != NULL )
			*pValid = valid != FALSE;
		return result;
	}

	bool SetDlgItemInt( HWND hDlg, UINT ctrlId, int value )
	{
		return SetDlgItemText( hDlg, ctrlId, num::FormatNumber( value ) );
	}


	bool EnableWindow( HWND hWnd, bool enable /*= true*/ )
	{
		ASSERT_PTR( hWnd );
		if ( enable == !HasFlag( GetStyle( hWnd ), WS_DISABLED ) )
			return false;

		::EnableWindow( hWnd, enable );
		return true;		// state changed
	}

	void EnableControls( HWND hDlg, const UINT ctrlIds[], size_t count, bool enable /*= true*/ )
	{
		ASSERT_PTR( hDlg );
		for ( size_t i = 0; i != count; ++i )
			if ( HWND hCtrl = ::GetDlgItem( hDlg, ctrlIds[ i ] ) )
				EnableWindow( hCtrl, enable );
	}

	bool ShowWindow( HWND hWnd, bool show /*= true*/ )
	{
		ASSERT_PTR( hWnd );
		if ( show == HasFlag( GetStyle( hWnd ), WS_VISIBLE ) )
			return false;

		ShowWindow( hWnd, show ? SW_SHOWNA : SW_HIDE );
		return true;		// state changed
	}

	void ShowControls( HWND hDlg, const UINT ctrlIds[], size_t count, bool show /*= true*/ )
	{
		ASSERT_PTR( hDlg );
		for ( size_t i = 0; i != count; ++i )
			if ( HWND hCtrl = ::GetDlgItem( hDlg, ctrlIds[ i ] ) )
				ShowWindow( hCtrl, show );
	}


	bool RedrawWnd( HWND hWnd )
	{
		if ( !::IsWindowVisible( hWnd ) )
			return false;

		if ( IsTopLevel( hWnd ) )
			::SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER );

		RedrawControl( hWnd );
		return true;
	}

	bool RedrawControl( HWND hCtrl )
	{
		if ( !ui::IsVisible( hCtrl ) )
			return false;

		::RedrawWindow( hCtrl, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
		::RedrawWindow( hCtrl, NULL, NULL, RDW_INVALIDATE );
		return true;
	}


	bool BringWndUp( HWND hWnd )
	{
		if ( HWND hPrevious = ::GetWindow( hWnd, GW_HWNDPREV ) )
			if ( ( hPrevious = ::GetWindow( hPrevious, GW_HWNDPREV ) ) != NULL )
				return ::SetWindowPos( hWnd, hPrevious, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;

		return ui::BringWndToTop( hWnd );
	}

	bool BringWndDown( HWND hWnd )
	{
		if ( HWND hNext = ::GetWindow( hWnd, GW_HWNDNEXT ) )
			return ::SetWindowPos( hWnd, hNext, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;

		return false;
	}


	bool SetTopMost( HWND hWnd, bool topMost /*= true*/ )
	{
		if ( topMost == IsTopMost( hWnd ) )
			return false;

		UINT swpFlags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE;

		static DWORD appProcessId = GetWindowProcessId( AfxGetMainWnd()->GetSafeHwnd() );
		if ( GetWindowProcessId( hWnd ) != appProcessId )
			swpFlags |= SWP_ASYNCWINDOWPOS;

		return ::SetWindowPos( hWnd, topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, swpFlags ) != FALSE;
	}


	struct CWindowEnumParam
	{
		CWindowEnumParam( std::vector< HWND >* pWindows, DWORD styleFilter ) : m_pWindows( pWindows ), m_styleFilter( styleFilter ) { ASSERT_PTR( m_pWindows ); }
	public:
		std::vector< HWND >* m_pWindows;
		DWORD m_styleFilter;
	};

	BOOL CALLBACK EnumWindowProc( HWND hWnd, LPARAM lParam )
	{
		CWindowEnumParam* pParam = reinterpret_cast<CWindowEnumParam*>( lParam );
		ASSERT_PTR( pParam );

		if ( 0 == pParam->m_styleFilter || EqFlag( GetStyle( hWnd ), pParam->m_styleFilter ) )
			if ( ui::IsTopLevel( hWnd ) )
				pParam->m_pWindows->push_back( hWnd );

		return TRUE;			// continue loop
	}

	bool QueryTopLevelWindows( std::vector< HWND >& rTopWindows, DWORD styleFilter /*= WS_VISIBLE*/, DWORD dwThreadId /*= GetCurrentThreadId()*/ )
	{
		CWindowEnumParam param( &rTopWindows, styleFilter );
		return EnumThreadWindows( dwThreadId, &EnumWindowProc, reinterpret_cast<LPARAM>( &param ) ) != FALSE;
	}


	bool OwnsFocus( HWND hWnd )
	{
		if ( HWND hFocusWnd = ::GetFocus() )
			if ( hWnd != NULL )
				if ( hFocusWnd == hWnd || ::IsChild( hWnd, hFocusWnd ) )
					return true;

		return false;
	}

	bool TakeFocus( HWND hWnd )
	{
		// if for instance a list control is in inline edit mode (owns focus), leave the focus in the child edit
		if ( ui::OwnsFocus( hWnd ) )
			return false;

		if ( !HasFlag( GetStyle( hWnd ), WS_CHILD ) )
			::SetFocus( hWnd );
		else
			ui::GotoDlgCtrl( hWnd );

		return true;
	}

	void GotoDlgCtrl( HWND hCtrl )
	{
		ASSERT_PTR( hCtrl );

		if ( HWND hParent = ::GetParent( hCtrl ) )
			::SendMessage( ::GetParent( hCtrl ), WM_NEXTDLGCTL, (WPARAM)hCtrl, 1L );
	}

	bool TriggerInput( HWND hParent )
	{
		if ( hParent != NULL && ui::OwnsFocus( hParent ) )
			if ( CWnd* pFocusCtrl = CWnd::GetFocus() )
				if ( ( (CEdit*)pFocusCtrl )->GetModify() )
					if ( is_a<CEdit>( pFocusCtrl ) ||
						 IsEditLikeCtrl( pFocusCtrl->m_hWnd ) )
					{
						::SetFocus( NULL );			// this will eventually trigger input on EN_KILLFOCUS

						if ( IsWindow( pFocusCtrl->GetSafeHwnd() ) )
							TakeFocus( *pFocusCtrl );
						return true;
					}

		return false;		// nothing changed
	}


	bool IsEditLikeCtrl( HWND hCtrl )
	{
		ASSERT_PTR( hCtrl );
		return HasFlag( ::SendMessage( hCtrl, WM_GETDLGCODE, VK_RETURN, 0 ), DLGC_HASSETSEL | DLGC_WANTCHARS );
	}

	bool SelectAllText( CWnd* pCtrl )
	{
		if ( CEdit* pEdit = dynamic_cast<CEdit*>( pCtrl ) )
		{
			pEdit->SetSel( 0, -1 );
			return true;
		}
		else if ( CComboBox* pComboBox = dynamic_cast<CComboBox*>( pCtrl ) )
			return pComboBox->SetEditSel( 0, -1 ) != FALSE;

		return false;
	}

	bool RecreateControl( CWnd* pCtrl, DWORD newStyle, DWORD newStyleEx /*= -1*/ )
	{	// allows recreating a control for which is not possible to change certain styles at runtime
		ASSERT_PTR( pCtrl->GetSafeHwnd() );

		if ( -1 == newStyle )
			newStyle = pCtrl->GetStyle();

		if ( -1 == newStyleEx )
			newStyleEx = pCtrl->GetExStyle();

		CWnd* pParentDlg = pCtrl->GetParent();
		CFont* pCtrlFont = pParentDlg->GetFont();

		std::tstring className = ui::GetClassName( pCtrl->GetSafeHwnd() );
		std::tstring caption = ui::GetWindowText( pCtrl->GetSafeHwnd() );
		UINT ctrlId = pCtrl->GetDlgCtrlID();
		CRect controlRect = ui::GetControlRect( pCtrl->GetSafeHwnd() );
		CWnd* pPrevCtrl = pCtrl->GetNextWindow( GW_HWNDPREV );

		pCtrl->DestroyWindow();
		if ( !pCtrl->CreateEx( newStyleEx, className.c_str(), caption.c_str(), newStyle, controlRect, pParentDlg, ctrlId ) )
			return false;

		pCtrl->SetFont( pCtrlFont );		// note: it doesn't work for CEdit with ES_PASSWORD
		pCtrl->SetWindowPos( pPrevCtrl != NULL ? pPrevCtrl : &CWnd::wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );		// move in tab order after the previous control

		if ( ui::ILayoutEngine* pParentLayout = dynamic_cast<ui::ILayoutEngine*>( pParentDlg ) )		// a layout dialog?
			pParentLayout->GetLayoutEngine().RefreshControlHandle( ctrlId );						// we have a new m_hControl

		return true;
	}


	struct CTestCmdUI : public CCmdUI		// used to test for disabled commands before dispatching
	{
	public:
		CTestCmdUI( UINT cmdId )
			: CCmdUI()
			, m_enabled( true )			// assume it's enabled
		{
			ASSERT( cmdId != 0 );		// zero IDs for normal commands are not allowed
			m_nID = cmdId;
		}

		// base overrides
		virtual void Enable( BOOL enabled ) { m_enabled = enabled != FALSE; m_bEnableChanged = TRUE; }
		virtual void SetCheck( int check ) { check; }
		virtual void SetRadio( BOOL on ) { on; }
		virtual void SetText( const TCHAR* pText ) { pText; }
	public:
		bool m_enabled;
	};


	bool IsCommandEnabled( CCmdTarget* pCmdTarget, UINT cmdId )
	{
		ASSERT_PTR( pCmdTarget );

		// make sure command has not become disabled before routing
		CTestCmdUI state( cmdId );

		if ( !pCmdTarget->OnCmdMsg( cmdId, CN_UPDATE_COMMAND_UI, &state, NULL ) )
			return false;			// not handled by this target

		if ( state.m_enabled )
			return true;

		TRACE( "Warning: skip executing disabled command %d\n", cmdId );
		return false;
	}

	bool HandleCommand( CCmdTarget* pCmdTarget, UINT cmdId )
	{
		ASSERT_PTR( pCmdTarget );

		if ( 0 == cmdId )
			return false;		// not handled: zero IDs for normal commands are not allowed

		if ( !IsCommandEnabled( pCmdTarget, cmdId ) )
			return true;		// handled: command disabled

		return pCmdTarget->OnCmdMsg( cmdId, CN_COMMAND, NULL, NULL ) != FALSE;
	}


	HBRUSH SendCtlColor( HWND hWnd, HDC hDC, UINT message /*= WM_CTLCOLORSTATIC*/ )
	{
		ASSERT_PTR( hWnd );
		if ( WM_CTLCOLORDLG == message )		// hWnd is a dialog
			return (HBRUSH)::SendMessage( hWnd, WM_CTLCOLORDLG, (WPARAM)hDC, (LPARAM)hWnd );
		else									// hWnd is a dialog's child
			return (HBRUSH)::SendMessage( ::GetParent( hWnd ), message, (WPARAM)hDC, (LPARAM)hWnd );
	}


	// CNmHdr implementation (declared in ui_fwd.h)

	LRESULT CNmHdr::NotifyParent( void )
	{
		return ui::SendNotifyToParent( hwndFrom, code, this );		// give parent a chance to handle the notification
	}


	COLORREF AlterColorSlightly( COLORREF bkColor )
	{
		// modify slightly the background (~ white): so that themes that render with alpha blending don't show weird colours (such as for radio button)
		BYTE green = GetGValue( bkColor );
		if ( green != 0 )
			++green;
		else
			--green;

		return RGB( GetRValue( bkColor ), green, GetBValue( bkColor ) );
	}
}


// import from <afximpl.h>
const AFX_MSGMAP_ENTRY* AFXAPI AfxFindMessageEntry( const AFX_MSGMAP_ENTRY* lpEntry, UINT message, UINT notifyCode, UINT id );

namespace ui
{
	struct FriendlyCmdTarget : public CCmdTarget
	{
		using CCmdTarget::GetMessageMap;

		static const AFX_MSGMAP* _GetMessageMap( const CCmdTarget* pCmdTarget ) { return ( (const FriendlyCmdTarget*)pCmdTarget )->GetMessageMap(); }
	};

	const AFX_MSGMAP_ENTRY* FindMessageHandler( const CCmdTarget* pCmdTarget, UINT message, UINT notifyCode, UINT id )
	{
		notifyCode = LOWORD( notifyCode );			// translate to WORD for proper identification in AfxFindMessageEntry

		for ( const AFX_MSGMAP* pMessageMap = FriendlyCmdTarget::_GetMessageMap( pCmdTarget ); pMessageMap->pfnGetBaseMap != NULL; pMessageMap = (*pMessageMap->pfnGetBaseMap)() )
			if ( const AFX_MSGMAP_ENTRY* pEntry = AfxFindMessageEntry( pMessageMap->lpEntries, message, notifyCode, id ) )
				return pEntry;		// found it

		return NULL;
	}
}


namespace ui
{
	std::tstring GetClassName( HWND hWnd )
	{
		ASSERT_PTR( hWnd );

		TCHAR className[ 128 ] = { 0 };
		::GetClassName( hWnd, className, COUNT_OF( className ) );
		return className;
	}

	bool IsEditBox( HWND hCtrl )
	{
		if ( CEdit* pEdit = dynamic_cast<CEdit*>( CWnd::FromHandlePermanent( hCtrl ) ) )
			return true;

		std::tstring className = GetClassName( hCtrl );

		if ( className == WC_EDITW || className == _T( WC_EDITA ) )
			return true;

		return false;
	}

	bool IsComboWithEdit( HWND hCtrl )
	{
		DWORD comboStyle = 0;

		if ( CComboBox* pComboBox = dynamic_cast<CComboBox*>( CWnd::FromHandlePermanent( hCtrl ) ) )
			comboStyle = pComboBox->GetStyle();
		else
		{
			std::tstring className = GetClassName( hCtrl );

			if ( className == WC_COMBOBOXEXW || className == _T( WC_COMBOBOXEXA ) )
				comboStyle = GetStyle( hCtrl );
		}

		switch ( comboStyle & 0x0F )
		{
			case CBS_DROPDOWN:
			case CBS_SIMPLE:
				return true;
			default:
				return false;
		}
	}

	bool IsGroupBox( HWND hWnd )
	{
		if ( EqMaskedValue( GetStyle( hWnd ), BS_TYPEMASK, BS_GROUPBOX ) )
			return GetClassName( hWnd ) == WC_BUTTON;

		return false;
	}

	bool IsDialogBox( HWND hWnd )
	{
		TCHAR className[ 128 ];
		::GetClassName( hWnd, className, COUNT_OF( className ) );
		return _T('#') == className[ 0 ] && (ATOM)WC_DIALOG == ::GlobalFindAtom( className );
	}

	bool IsMenuWnd( HWND hWnd )
	{
		return GetClassName( hWnd ) == _T("#32768");
	}


	bool ModifyBorder( CWnd* pWnd, bool useBorder /*= true*/ )
	{
		if ( useBorder == HasFlag( pWnd->GetStyle(), WS_BORDER ) )
			return false;

		DWORD styleRemove = WS_BORDER, styleAdd = 0;
		int inflateBy = -1;
		if ( useBorder )
		{
			std::swap( styleRemove, styleAdd );
			inflateBy = -inflateBy;
		}

		pWnd->ModifyStyle( styleRemove, styleAdd );		// SWP_FRAMECHANGED

		CRect rect;
		pWnd->GetWindowRect( &rect );
		if ( HasFlag( pWnd->GetStyle(), WS_CHILD ) )
			pWnd->GetParent()->ScreenToClient( &rect );

		rect.InflateRect( inflateBy, inflateBy );
		pWnd->MoveWindow( &rect );
		return true;
	}

} // namespace ui


namespace ui
{
	bool& RefAsyncApiEnabled( void )			// some modes require disabling of asynchronous API calls due to interference (e.g. CDesktopDC)
	{
		static bool s_asyncApiEnabled = true;
		return s_asyncApiEnabled;
	}

	bool BeepSignal( UINT beep /*= MB_OK*/ )
	{
		if ( RefAsyncApiEnabled() )
			::MessageBeep( beep );
		return false;
	}

	bool ReportError( const std::tstring& message, UINT mbFlags /*= MB_OK | MB_ICONERROR*/ )
	{
		MessageBox( message, mbFlags );
		return false;
	}

	int ReportException( const std::exception& exc, UINT mbFlags /*= MB_OK | MB_ICONERROR*/ )
	{
		return MessageBox( CRuntimeException::MessageOf( exc ), mbFlags );
	}

	int ReportException( const CException* pExc, UINT mbFlags /*= MB_OK | MB_ICONERROR*/ )
	{
		return MessageBox( mfc::CRuntimeException::MessageOf( *pExc ), mbFlags );
	}


	bool ShowInputError( CWnd* pCtrl, const std::tstring& message, UINT iconFlag /*= MB_ICONERROR*/ )
	{
		ASSERT_PTR( pCtrl->GetSafeHwnd() );

		static const TCHAR s_title[] = _T("Input Validation Error");
		int ttiIcon = TTI_NONE;
		enum { MB_IconMask = MB_ICONERROR | MB_ICONWARNING | MB_ICONINFORMATION | MB_ICONQUESTION };

		switch ( iconFlag & MB_IconMask )
		{
			case MB_ICONERROR:			ttiIcon = TTI_ERROR_LARGE; break;
			case MB_ICONWARNING:		ttiIcon = TTI_WARNING_LARGE; break;
			case MB_ICONINFORMATION:	ttiIcon = TTI_INFO; break;
			case MB_ICONQUESTION:		ttiIcon = TTI_NONE; break;
			default: ASSERT( false );
		}

		if ( CEdit* pEdit = dynamic_cast<CEdit*>( pCtrl ) )
			pEdit->ShowBalloonTip( s_title,  message.c_str(), ttiIcon );
		else
			ui::ShowBalloonTip( pCtrl, s_title,  message.c_str(), (HICON)ttiIcon );

		TakeFocus( pCtrl->m_hWnd );
		SelectAllText( pCtrl );

		return BeepSignal( iconFlag );
	}


	namespace ddx
	{
		void FailInput( CDataExchange* pDX, UINT ctrlId, const std::tstring& validationError )
		{
			ASSERT( DialogSaveChanges == pDX->m_bSaveAndValidate );

			CWnd* pCtrl = pDX->m_pDlgWnd->GetDlgItem( ctrlId );

			if ( pCtrl != NULL )
				IsEditLikeCtrl( pCtrl->m_hWnd ) ? pDX->PrepareEditCtrl( ctrlId ) : pDX->PrepareCtrl( ctrlId );

			if ( !validationError.empty() )
				ShowInputError( pCtrl != NULL ? pCtrl : pDX->m_pDlgWnd, validationError );		// will also focus and select all text

			pDX->Fail();	// will throw a CUserException, handled by CWnd::UpdateData()
		}


		std::tstring GetItemText( CDataExchange* pDX, UINT ctrlId )
		{
			CWnd* pCtrl = pDX->m_pDlgWnd->GetDlgItem( ctrlId );
			ASSERT_PTR( pCtrl );

			if ( CComboBox* pCombo = dynamic_cast<CComboBox*>( pCtrl ) )
				return GetComboSelText( *pCombo );

			return GetWindowText( pCtrl );
		}

		bool SetItemText( CDataExchange* pDX, UINT ctrlId, const std::tstring& text )
		{
			CWnd* pCtrl = pDX->m_pDlgWnd->GetDlgItem( ctrlId );
			ASSERT_PTR( pCtrl );

			str::CaseType caseType = str::Case;
			if ( CHistoryComboBox* pHistoryCombo = dynamic_cast<CHistoryComboBox*>( pCtrl ) )
				caseType = pHistoryCombo->GetCaseType();

			if ( CComboBox* pCombo = dynamic_cast<CComboBox*>( pCtrl ) )
				return SetComboEditText( *pCombo, text, caseType ).first;

			return SetWindowText( pCtrl->m_hWnd, text );
		}
	}


	void DDX_Text( CDataExchange* pDX, int ctrlId, std::tstring& rValue, bool trim /*= false*/ )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ddx::SetItemText( pDX, ctrlId, rValue );
		else
		{
			rValue = ddx::GetItemText( pDX, ctrlId );
			if ( trim )
				str::Trim( rValue );
		}
	}

	void DDX_Path( CDataExchange* pDX, int ctrlId, fs::CPath& rValue )
	{
		DDX_Text( pDX, ctrlId, const_cast<std::tstring&>( rValue.Get() ), true );
	}

	void DDX_PathItem( CDataExchange* pDX, int ctrlId, CPathItemBase* pPathItem )
	{
		ASSERT_PTR( pPathItem );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ddx::SetItemText( pDX, ctrlId, pPathItem->GetFilePath().Get() );
		else
		{
			fs::CPath filePath;
			DDX_Path( pDX, ctrlId, filePath );

			pPathItem->SetFilePath( filePath );
		}
	}

	void DDX_Int( CDataExchange* pDX, int ctrlId, int& rValue, const int nullValue /*= INT_MAX*/ )
	{
		static const std::tstring empty;

		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			if ( rValue != nullValue )
				SetDlgItemInt( *pDX->m_pDlgWnd, ctrlId, rValue );
			else
				ddx::SetItemText( pDX, ctrlId, empty );
		}
		else
		{
			bool valid;
			int value = GetDlgItemInt( *pDX->m_pDlgWnd, ctrlId, &valid );
			rValue = valid && value != nullValue ? value : nullValue;
		}
	}

	void DDX_Bool( CDataExchange* pDX, int ctrlId, bool& rValue )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			BOOL checked = rValue;
			DDX_Check( pDX, ctrlId, checked );
		}
		else
		{
			BOOL checked;
			DDX_Check( pDX, ctrlId, checked );
			rValue = checked != FALSE;
		}
	}

	void DDX_BoolRadio( CDataExchange* pDX, int radioFirstId, bool& rValue, bool firstRadioIsTrue )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			int selIndex = firstRadioIsTrue ? !rValue : rValue;
			::DDX_Radio( pDX, radioFirstId, selIndex );
		}
		else
		{
			int selIndex;
			::DDX_Radio( pDX, radioFirstId, selIndex );
			rValue = firstRadioIsTrue ? ( 1 == selIndex ) : ( 0 == selIndex );
		}
	}

	void DDX_Flag( CDataExchange* pDX, int ctrlId, int& rValue, int flag )
	{
		ASSERT( flag != 0 );
		if ( DialogOutput == pDX->m_bSaveAndValidate )
		{
			BOOL checked = HasFlag( rValue, flag );
			DDX_Check( pDX, ctrlId, checked );
		}
		else
		{
			BOOL checked;
			DDX_Check( pDX, ctrlId, checked );
			SetFlag( rValue, flag, checked != FALSE );
		}
	}


	void DDX_ButtonIcon( CDataExchange* pDX, int ctrlId, const CIconId& iconId /*= CIconId( 0 )*/, bool useText /*= true*/, bool useTextSpacing /*= true*/ )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate && CImageStore::HasSharedStore() )
			if ( CButton* pButton = (CButton*)pDX->m_pDlgWnd->GetDlgItem( ctrlId ) )
				CIconButton::SetButtonIcon( pButton, CIconId( iconId.m_id != 0 ? iconId.m_id : ctrlId, iconId.m_stdSize ), useText, useTextSpacing );
			else
				ASSERT( false );
	}

	void DDX_StaticIcon( CDataExchange* pDX, int ctrlId, const CIconId& iconId /*= CIconId( 0 )*/ )
	{
		if ( DialogOutput == pDX->m_bSaveAndValidate && CImageStore::HasSharedStore() )
			if ( CStatic* pStatic = (CStatic*)pDX->m_pDlgWnd->GetDlgItem( ctrlId ) )
			{
				if ( NULL == pStatic->GetIcon() )
					if ( const CIcon* pIcon = CImageStore::GetSharedStore()->RetrieveIcon( CIconId( iconId.m_id != 0 ? iconId.m_id : ctrlId, iconId.m_stdSize ) ) )
						pStatic->SetIcon( pIcon->GetHandle() );
			}
			else
				ASSERT( false );
	}


	void SetSpinRange( CWnd* pDlg, int ctrlId, int minValue, int maxValue )
	{
		if ( CSpinButtonCtrl* pSpinButton = static_cast<CSpinButtonCtrl*>( pDlg->GetDlgItem( ctrlId ) ) )
			pSpinButton->SetRange32( minValue, maxValue );
		else
			ASSERT( false );		// spin button not found
	}

	HTREEITEM FindTreeItem( const CTreeCtrl& treeCtrl, const std::tstring& itemText, HTREEITEM hParent /*= TVI_ROOT*/ )
	{
		for ( HTREEITEM hChild = treeCtrl.GetChildItem( hParent ); hChild != NULL; hChild = treeCtrl.GetNextSiblingItem( hChild ) )
			if ( itemText == treeCtrl.GetItemText( hChild ).GetString() )
				return hChild;
			else if ( HTREEITEM hFoundItem = FindTreeItem( treeCtrl, itemText, hChild ) )
				return hFoundItem;

		return NULL;
	}

	static pred::CompareResult CALLBACK CompareSubjectProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
	{
		const pred::IComparator* pComparator = reinterpret_cast<const pred::IComparator*>( lParamSort );
		ASSERT_PTR( pComparator );

		const utl::ISubject* pLeft = reinterpret_cast<const utl::ISubject*>( lParam1 );
		const utl::ISubject* pRight = reinterpret_cast<const utl::ISubject*>( lParam2 );

		return pComparator->CompareObjects( pLeft, pRight );
	}

	bool SortTreeChildren( const pred::IComparator* pComparator, CTreeCtrl& rTreeCtrl, HTREEITEM hParent /*= TVI_ROOT*/, RecursionDepth depth /*= Shallow*/ )
	{
		static const pred::Comparator< pred::TCompareCode > compareCodeAsc;
		if ( NULL == pComparator )
			pComparator = &compareCodeAsc;

		if ( Deep == depth )
			for ( HTREEITEM hChild = rTreeCtrl.GetChildItem( hParent ); hChild != NULL; hChild = rTreeCtrl.GetNextSiblingItem( hChild ) )
				SortTreeChildren( pComparator, rTreeCtrl, hChild, depth );

		TVSORTCB sortInfo;
		sortInfo.hParent = hParent;
		sortInfo.lpfnCompare = reinterpret_cast<PFNTVCOMPARE>( CompareSubjectProc );
		sortInfo.lParam = reinterpret_cast<LPARAM>( pComparator );
		return rTreeCtrl.SortChildrenCB( &sortInfo ) != FALSE;
	}

	int FindListItem( const CComboBox& combo, const TCHAR* pItemText, str::CaseType caseType /*= str::Case*/ )
	{
		CString itemText;
		for ( int i = 0, count = combo.GetCount(); i != count; ++i )
		{
			combo.GetLBText( i, itemText );
			if ( str::Matches( itemText.GetString(), pItemText, str::Case == caseType, true ) )
				return i;
		}
		return -1;
	}

	void ReadListBoxItems( std::vector< std::tstring >& rOutItems, const CListBox& listBox )
	{
		CString itemText;
		int count = listBox.GetCount();

		rOutItems.resize( count );
		for ( int i = 0; i != count; ++i )
		{
			listBox.GetText( i, itemText );
			rOutItems[ i ] = itemText.GetString();
		}
	}

	void WriteListBoxItems( CListBox& rListBox, const std::vector< std::tstring >& items )
	{
		rListBox.ResetContent();
		for ( std::vector< std::tstring >::const_iterator it = items.begin(); it != items.end(); ++it )
			rListBox.AddString( it->c_str() );
	}

	void ReadComboItems( std::vector< std::tstring >& rOutItems, const CComboBox& combo )
	{
		CString itemText;
		int count = combo.GetCount();

		rOutItems.resize( count );
		for ( int i = 0; i != count; ++i )
		{
			combo.GetLBText( i, itemText );
			rOutItems[ i ] = itemText.GetString();
		}
	}

	void WriteComboItems( CComboBox& rCombo, const std::vector< std::tstring >& items )
	{
		rCombo.ResetContent();
		for ( std::vector< std::tstring >::const_iterator it = items.begin(); it != items.end(); ++it )
			rCombo.AddString( it->c_str() );
	}


	CEdit* GetComboEdit( const CComboBox& rCombo )
	{
		COMBOBOXINFO comboInfo = { sizeof( COMBOBOXINFO ) };
		if ( rCombo.GetComboBoxInfo( &comboInfo ) )
			if ( comboInfo.hwndItem != NULL )
				return (CEdit*)CWnd::FromHandle( comboInfo.hwndItem );

		return NULL;
	}

	CWnd* GetComboDropList( const CComboBox& rCombo )
	{
		COMBOBOXINFO comboInfo = { sizeof( COMBOBOXINFO ) };
		if ( rCombo.GetComboBoxInfo( &comboInfo ) )
			if ( comboInfo.hwndList != NULL )
				return (CEdit*)CWnd::FromHandle( comboInfo.hwndList );

		return NULL;
	}

	std::tstring GetComboSelText( const CComboBox& rCombo, ComboField byField /*= BySel*/ )
	{
		int selIndex = rCombo.GetCurSel();
		if ( ByEdit == byField || CB_ERR == selIndex )
			return ui::GetWindowText( rCombo );
		else
			return GetComboItemText( rCombo, selIndex );
	}

	std::pair<bool, ComboField> SetComboEditText( CComboBox& rCombo, const std::tstring& currText, str::CaseType caseType /*= str::Case*/ )
	{
		int oldSelPos = rCombo.GetCurSel();
		int foundListPos = ui::FindListItem( rCombo, currText.c_str(), caseType );

		if ( foundListPos != CB_ERR )
			if ( oldSelPos == foundListPos )
				return std::make_pair( false, BySel );			// no change in selection
			else if ( rCombo.SetCurSel( foundListPos ) != CB_ERR )
			{
				rCombo.UpdateWindow();							// force redraw when mouse is tracking (no WM_PAINT sent)
				return std::make_pair( true, BySel );			// select existing item from the list
			}

		if ( EqMaskedValue( rCombo.GetStyle(), 0x0F, CBS_DROPDOWNLIST ) )
			return std::make_pair( false, BySel );				// no selection hit (combo has no edit)

		// clear selected item to mark the dirty list state for the new item
		bool selChanged = oldSelPos != CB_ERR;					// had an old selection?
		if ( selChanged )
			rCombo.SetCurSel( CB_ERR );							// clear selection: will clear the edit text, but it will not notify CBN_EDITCHANGE

		// change edit text
		if ( !ui::SetWindowText( rCombo, currText ) )
			return std::make_pair( selChanged, ByEdit );		// no change in edit text

		rCombo.UpdateWindow();									// force redraw when mouse is tracking (no WM_PAINT sent)
		return std::make_pair( true, ByEdit );					// changed edit text
	}

	std::pair<bool, ComboField> ReplaceComboEditText( CComboBox& rCombo, const std::tstring& currText, str::CaseType caseType /*= str::Case*/ )
	{
		DWORD sel = rCombo.GetEditSel();
		int startPos = LOWORD( sel ), endPos = HIWORD( sel );
		ASSERT( endPos != -1 );
		bool hasSelSubstring = startPos != endPos && endPos - startPos < (int)ui::GetComboSelText( rCombo, ByEdit ).length();

		if ( !hasSelSubstring )
			return ui::SetComboEditText( rCombo, currText, caseType );

		std::tstring entireText = ui::GetWindowText( rCombo );
		entireText.replace( startPos, endPos - startPos, currText.c_str() );

		std::pair<bool, ComboField> result = ui::SetComboEditText( rCombo, entireText.c_str(), caseType );		// clears current sel if item isn't in the LBox
		rCombo.SetEditSel( startPos, startPos + static_cast<int>( currText.length() ) );							// select the new substring
		return result;
	}

	void UpdateHistoryCombo( CComboBox& rCombo, size_t maxCount, str::CaseType caseType /*= str::Case*/ )
	{
		int selIndex = rCombo.GetCurSel();
		CString currText;

		if ( CB_ERR == selIndex )
		{
			// no selection -> insert edit's text in the list
			rCombo.GetWindowText( currText );

			// allow empty items to be inserted; combo will be notified to remove invalid items through HCN_VALIDATEITEMS notification
			selIndex = FindListItem( rCombo, currText.GetString(), caseType );
			if ( CB_ERR == selIndex )
				rCombo.InsertString( 0, currText );		// add the new string on top of the list
		}

		// if any text selected, move the selection on top
		if ( selIndex != CB_ERR )
			if ( selIndex > 0 )
			{
				rCombo.GetLBText( selIndex, currText );
				rCombo.DeleteString( selIndex );
				rCombo.InsertString( 0, currText );
			}

		if ( ui::IContentValidator* pContentValidator = dynamic_cast<ui::IContentValidator*>( &rCombo ) )
			pContentValidator->ValidateContent();
		else
			ui::SendCommandToParent( rCombo, CHistoryComboBox::HCN_VALIDATEITEMS );		// give parent a chance to cleanup invalid items

		// remove exceeding items
		for ( size_t count = rCombo.GetCount(); count > maxCount; )
			rCombo.DeleteString( (unsigned int)--count );
		rCombo.SetCurSel( 0 );
	}

	void LoadHistoryCombo( CComboBox& rHistoryCombo, const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText, const TCHAR* pSep /*= _T(";")*/ )
	{
		std::vector< std::tstring > items;
		str::Split( items, AfxGetApp()->GetProfileString( pSection, pEntry, pDefaultText ).GetString(), pSep );

		if ( !items.empty() )		// keep default items if nothing saved
		{
			ui::WriteComboItems( rHistoryCombo, items );

			if ( rHistoryCombo.GetCount() != 0 )
				rHistoryCombo.SetCurSel( 0 );
		}
	}

	void SaveHistoryCombo( CComboBox& rHistoryCombo, const TCHAR* pSection, const TCHAR* pEntry,
						   const TCHAR* pSep /*= _T(";")*/, size_t maxCount /*= HistoryMaxSize*/, str::CaseType caseType /*= str::Case*/ )
	{
		ui::UpdateHistoryCombo( rHistoryCombo, maxCount, caseType );

		std::vector< std::tstring > items;
		ui::ReadComboItems( items, rHistoryCombo );

		if ( items.size() > maxCount )
			items.erase( items.begin() + maxCount, items.end() );

		AfxGetApp()->WriteProfileString( pSection, pEntry, str::Join( items, pSep ).c_str() );
	}


	std::tstring LoadHistorySelItem( const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText, const TCHAR* pSep /*= _T(";")*/ )
	{
		std::vector< std::tstring > items;
		str::Split( items, AfxGetApp()->GetProfileString( pSection, pEntry, pDefaultText ).GetString(), pSep );

		if ( items.empty() )
			return std::tstring();

		return items.front();		// top item is the selected one
	}


	void MakeStandardControlFont( CFont& rOutFont, const ui::CFontInfo& fontInfo /*= ui::CFontInfo()*/, int stockFontType /*= DEFAULT_GUI_FONT*/ )
	{
		if ( NULL == rOutFont.m_hObject )			// create font once (friendly with static font data members)
		{
			CFont* pStockFont = CFont::FromHandle( (HFONT)::GetStockObject( stockFontType ) );
			ASSERT_PTR( pStockFont );

			LOGFONT logFont;
			memset( &logFont, 0, sizeof( LOGFONT ) );
			pStockFont->GetLogFont( &logFont );

			if ( fontInfo.m_pFaceName != NULL )
			{
				logFont.lfCharSet = DEFAULT_CHARSET;
				_tcscpy( logFont.lfFaceName, fontInfo.m_pFaceName );
			}

			if ( fontInfo.m_heightPct != 100 )
				logFont.lfHeight = MulDiv( logFont.lfHeight, fontInfo.m_heightPct, 100 );
			if ( HasFlag( fontInfo.m_effect, Bold ) )
				logFont.lfWeight = FW_BOLD;
			if ( HasFlag( fontInfo.m_effect, Italic ) )
				logFont.lfItalic = TRUE;
			if ( HasFlag( fontInfo.m_effect, Underline ) )
				logFont.lfUnderline = TRUE;

			rOutFont.CreateFontIndirect( &logFont );
		}
	}

	void MakeEffectControlFont( CFont& rOutFont, CFont* pSourceFont, TFontEffect fontEffect /*= ui::Regular*/, int heightPct /*= 100*/ )
	{
		if ( rOutFont.m_hObject != NULL )
			return;				// create once

		if ( NULL == pSourceFont )
			pSourceFont = CFont::FromHandle( (HFONT)::GetStockObject( DEFAULT_GUI_FONT ) );
		ASSERT_PTR( pSourceFont->GetSafeHandle() );

		LOGFONT logFont;
		memset( &logFont, 0, sizeof( LOGFONT ) );
		pSourceFont->GetLogFont( &logFont );

		if ( heightPct != 100 )
			logFont.lfHeight = MulDiv( logFont.lfHeight, heightPct, 100 );

		if ( HasFlag( fontEffect, Bold ) )
			logFont.lfWeight = FW_BOLD;
		if ( HasFlag( fontEffect, Italic ) )
			logFont.lfItalic = TRUE;
		if ( HasFlag( fontEffect, Underline ) )
			logFont.lfUnderline = TRUE;

		rOutFont.CreateFontIndirect( &logFont );
	}


	// CFontEffectCache implementation

	CFontEffectCache::CFontEffectCache( CFont* pSourceFont )
	{
		ASSERT_PTR( pSourceFont->GetSafeHandle() );
		m_sourceFont.Attach( pSourceFont->GetSafeHandle() );
	}

	CFontEffectCache::~CFontEffectCache()
	{
		m_sourceFont.Detach();		// no ownership
		utl::ClearOwningAssocContainerValues( m_effectFonts );
	}

	CFont* CFontEffectCache::Lookup( TFontEffect fontEffect )
	{
		if ( ui::Regular == fontEffect )
			return &m_sourceFont;

		CFont*& rpFount = m_effectFonts[ fontEffect ];
		if ( NULL == rpFount )
		{
			rpFount = new CFont();
			MakeEffectControlFont( *rpFount, &m_sourceFont, fontEffect );
		}
		ASSERT_PTR( rpFount->GetSafeHandle() );
		return rpFount;
	}


	void AddSysColors( std::vector< COLORREF >& rColors, const int sysIndexes[], size_t count )
	{
		rColors.reserve( rColors.size() + count );
		for ( size_t i = 0; i != count; ++i )
			rColors.push_back( GetSysColor( sysIndexes[ i ] ) );
	}


	bool PumpPendingMessages( HWND hWnd /*= NULL*/ )
	{
		// eat all messages in queue, no OnIdle() though
		MSG msg;
		while ( ::PeekMessage( &msg, hWnd, 0, 0, PM_NOREMOVE ) )
			if ( !AfxGetThread()->PumpMessage() )
			{
				::PostQuitMessage( 0 );
				return false;
			}

		return true;
	}

	bool EatPendingMessages( HWND hWnd /*= NULL*/, UINT minMessage /*= WM_TIMER*/, UINT maxMessage /*= WM_TIMER*/ )
	{
		int msgCount = 0;
		MSG msg;

		while ( ::PeekMessage( &msg, hWnd, minMessage, maxMessage, PM_REMOVE ) )
			++msgCount;

		return msgCount > 0;
	}
}
