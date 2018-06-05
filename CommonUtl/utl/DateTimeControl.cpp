
#include "stdafx.h"
#include "DateTimeControl.h"
#include "AccelTable.h"
#include "Clipboard.h"
#include "MenuUtilities.h"
#include "Utilities.h"
#include "TimeUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CDateTimeControl::s_dateTimeFormat[] = _T("d'-'MM'-'yyy  H':'mm':'ss");
const TCHAR CDateTimeControl::s_nullFormat[] = _T(" ");
static CAccelTable s_accel;


CDateTimeControl::CDateTimeControl( const TCHAR* pValidFormat /*= s_dateTimeFormat*/, const TCHAR* pNullFormat /*= s_nullFormat*/ )
	: CDateTimeCtrl()
	, m_pValidFormat( pValidFormat )
	, m_pNullFormat( pNullFormat )
	, m_pLastFormat( NULL )
{
}

CDateTimeControl::~CDateTimeControl()
{
}

CAccelTable& CDateTimeControl::GetAccelTable( void )
{
	static ACCEL s_keys[] =
	{
		{ FVIRTKEY, VK_DELETE, ID_RESET_DEFAULT },
		{ FVIRTKEY, VK_HOME, ID_EDIT_ITEM }
	};
	static CAccelTable s_accel( s_keys, COUNT_OF( s_keys ) );
	return s_accel;
}

void CDateTimeControl::SetValidFormat( const TCHAR* pValidFormat )
{
	m_pValidFormat = pValidFormat;

	if ( m_hWnd != NULL )
		SetDateTime( GetDateTime() );
}

void CDateTimeControl::SetNullFormat( const TCHAR* pNullFormat )
{
	bool isNull = IsNullDateTime();
	m_pNullFormat = pNullFormat;

	if ( m_hWnd != NULL && isNull )
		SetNullDateTime();
}

CTime CDateTimeControl::GetDateTime( void ) const
{
	CTime dateTime;

	// Issue: when the user picks a date from the month calendar drop-down, DTN_CLOSEUP is received after DTN_DATETIMECHANGE (sent twice) by the common control.
	// Since we flip to a normal format only later on receiving DTN_CLOSEUP, we need to return the actual current time while month calendar is visible, regardless of current format.

	if ( m_pLastFormat != m_pNullFormat || GetMonthCalCtrl() != NULL )
		if ( GetTime( dateTime ) != GDT_VALID )
			dateTime = CTime();

	return dateTime;
}

bool CDateTimeControl::SetDateTime( CTime dateTime )
{
	bool isValid = !IsNullDateTime( dateTime );
	FlipFormat( isValid );

	if ( !isValid )
		dateTime = CTime::GetCurrentTime();		// use current time as NULL value, so that user can pick from today's context

	return SetTime( &dateTime ) != FALSE;
}

bool CDateTimeControl::IsNullDateTime( void ) const
{
	if ( m_pNullFormat == m_pLastFormat )
		return true;

	if ( m_hWnd != NULL && IsNullDateTime( GetDateTime() ) )
		return true;

	return false;
}

bool CDateTimeControl::UserSetDateTime( const CTime& dateTime )
{
	CTime oldDateTime = GetDateTime();

	bool succeeded = SetDateTime( dateTime );

	if ( GetDateTime() != oldDateTime )
		SendNotifyDateTimeChange();

	return succeeded;
}

Range< CTime > CDateTimeControl::GetDateRange( void ) const
{
	Range< CTime > dtRange;
	DWORD result = GetRange( &dtRange.m_start, &dtRange.m_end );

	if ( !HasFlag( result, GDTR_MIN ) )
		dtRange.m_start = CTime();

	if ( !HasFlag( result, GDTR_MAX ) )
		dtRange.m_end = CTime();

	return dtRange;
}

bool CDateTimeControl::SetDateRange( const Range< CTime >& dateTimeRange )
{
	return SetRange( &dateTimeRange.m_start, &dateTimeRange.m_end ) != FALSE;
}

bool CDateTimeControl::FlipFormat( bool isValid )
{
	const TCHAR* pDesiredFormat = isValid ? m_pValidFormat : m_pNullFormat;

	if ( pDesiredFormat == m_pLastFormat )
		return false;			// no change, so no need to flip the format

	if ( !SetFormat( pDesiredFormat ) )
		return false;

	m_pLastFormat = pDesiredFormat;
	return true;
}

bool CDateTimeControl::SendNotifyDateTimeChange( void )
{
	NMDATETIMECHANGE changeInfo;

	CTime dateTime;
	changeInfo.dwFlags = GetTime( dateTime );
	dateTime.GetAsSystemTime( changeInfo.st );

	// notify parent of date-time value change
	return 0 == ui::SendNotifyToParent( m_hWnd, DTN_DATETIMECHANGE, &changeInfo.nmhdr );		// false if an error occured
}

void CDateTimeControl::PreSubclassWindow( void )
{
	CDateTimeCtrl::PreSubclassWindow();

	CScopedInternalChange internalChange( this );
	SetNullDateTime();			// reflect the initial unassigned state
}

BOOL CDateTimeControl::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( GetAccelTable().Translate( pMsg, m_hWnd ) )
			return true;

	return CDateTimeCtrl::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CDateTimeControl, CDateTimeCtrl )
	ON_WM_CONTEXTMENU()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND( ID_EDIT_CUT, OnCut )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CUT, OnUpdateValid )
	ON_COMMAND( ID_EDIT_COPY, OnCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateValid )
	ON_COMMAND( ID_EDIT_PASTE, OnPaste )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdatePaste )
	ON_COMMAND( ID_EDIT_ITEM, OnSetNow )
	ON_COMMAND( ID_RESET_DEFAULT, OnClear )
	ON_UPDATE_COMMAND_UI_RANGE( ID_RESET_DEFAULT, ID_LIST_VIEW_TILE, OnUpdateValid )
	ON_NOTIFY_REFLECT_EX( DTN_DATETIMECHANGE, OnDtnDateTimeChange_Reflect )
	ON_NOTIFY_REFLECT_EX( DTN_CLOSEUP, OnDtnCloseup_Reflect )
END_MESSAGE_MAP()

void CDateTimeControl::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		CMenu popupMenu;
		ui::LoadPopupMenu( popupMenu, IDR_STD_CONTEXT_MENU, ui::DateTimePopup );
		ui::TrackPopupMenu( popupMenu, this, screenPos );
		return;					// supress rising WM_CONTEXTMENU to the parent
	}

	Default();
}

void CDateTimeControl::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	CDateTimeCtrl::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

BOOL CDateTimeControl::OnDtnDateTimeChange_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMDATETIMECHANGE* pChange = (NMDATETIMECHANGE*)pNmHdr;
	pChange;
	*pResult = 0L;
	return IsInternalChange();		// don't raise the notification to parent during an internal change
}

BOOL CDateTimeControl::OnDtnCloseup_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;

	//TRACE( _T(" - CDateTimeControl::OnDtnCloseup_Reflect\n") );
	if ( IsNullDateTime() )
		FlipFormat( true );

	*pResult = 0L;
	return IsInternalChange();		// don't raise the notification to parent during an internal change
}

void CDateTimeControl::OnCut( void )
{
	OnCopy();
	OnClear();
}

void CDateTimeControl::OnCopy( void )
{
	std::tstring text = time_utl::FormatTimestamp( GetDateTime() );
	CClipboard::CopyText( text, this );
}

void CDateTimeControl::OnPaste( void )
{
	std::tstring text;
	if ( CClipboard::PasteText( text, this ) )
		if ( !UserSetDateTime( time_utl::ParseTimestamp( text ) ) )
			ui::BeepSignal( MB_ICONWARNING );
}

void CDateTimeControl::OnUpdatePaste( CCmdUI* pCmdUI )
{
	bool enable = false;
	std::tstring text;
	if ( CClipboard::PasteText( text, this ) )
		enable = !IsNullDateTime( time_utl::ParseTimestamp( text ) );

	pCmdUI->Enable( enable );
}

void CDateTimeControl::OnSetNow( void )
{
	UserSetDateTime( CTime::GetCurrentTime() );
}

void CDateTimeControl::OnClear( void )
{
	UserSetDateTime( CTime() );
}

void CDateTimeControl::OnUpdateValid( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !IsNullDateTime() );
}
