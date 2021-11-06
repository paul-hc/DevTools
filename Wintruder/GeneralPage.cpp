
#include "stdafx.h"
#include "GeneralPage.h"
#include "AppService.h"
#include "Application.h"
#include "resource.h"
#include "wnd/FlagRepository.h"
#include "wnd/WindowClass.h"
#include "utl/StringUtilities.h"
#include "utl/UI/LayoutBasePropertySheet.h"
#include "utl/UI/Utilities.h"
#include "wnd/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CInfoEditBox class

class CInfoEditBox : public CEdit
{
public:
	CInfoEditBox( CGeneralPage* pOwner ) : m_pOwner( pOwner ), m_lastSel( 0 ), m_lastCaretLine( -1 ), m_lineHeight( -1 ) { ASSERT_PTR( m_pOwner ); }

	void SetupWindow( void );
	bool UpdateTooltipInfo( const std::vector< std::tstring >& toolInfos );

	CRect GetLineRect( int lineIndex );
	void MoveButtonToLine( int lineIndex );
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );

	bool CheckCaretPosition( void );
private:
	CGeneralPage* m_pOwner;
	DWORD m_lastSel;
	int m_lastCaretLine;
	int m_lineHeight;
	static UINT m_toolLine[];
};


UINT CInfoEditBox::m_toolLine[] = { PF_Handle, PF_ID, PF_Style, PF_StyleEx, PF_WindowRect, PF_ClientRect };


void CInfoEditBox::SetupWindow( void )
{
	CRect rectEmpty( 0, 0, 0, 0 );

	SetWindowText( _T("#\r\n#\r\n") );
	m_lineHeight = PosFromChar( LineIndex( 1 ) ).y - PosFromChar( LineIndex( 0 ) ).y;
	ASSERT( IsWindow( m_pOwner->GetSafeHwnd() ) && IsWindow( *m_pOwner->GetButton() ) );
	m_pOwner->GetButton()->SetParent( this );

	CToolTipCtrl* pMainTooltip = app::GetMainTooltip();
	for ( int i = 0; i != COUNT_OF( m_toolLine ); ++i )
		pMainTooltip->AddTool( this, _T(""), rectEmpty, IDC_GENERAL_INFO_EDIT + m_toolLine[ i ] );
	pMainTooltip->AddTool( m_pOwner->GetButton(), _T("") );
}

bool CInfoEditBox::UpdateTooltipInfo( const std::vector< std::tstring >& toolInfos )
{
	bool valid = toolInfos.size() == COUNT_OF( m_toolLine );
	CToolTipCtrl* pMainTooltip = app::GetMainTooltip();

	// update tip rects
	for ( int i = 0; i != COUNT_OF( m_toolLine ); ++i )
		if ( valid )
		{
			pMainTooltip->SetToolRect( this, IDC_GENERAL_INFO_EDIT + m_toolLine[ i ], GetLineRect( m_toolLine[ i ] ) );
			pMainTooltip->UpdateTipText( toolInfos[ i ].c_str(), this, IDC_GENERAL_INFO_EDIT + m_toolLine[ i ] );
		}
		else
			pMainTooltip->UpdateTipText( _T(""), this, IDC_GENERAL_INFO_EDIT + m_toolLine[ i ] );
	return true;
}

void CInfoEditBox::MoveButtonToLine( int lineIndex )
{
	CRect rectLine( GetLineRect( lineIndex ) );
	CRect buttonRect;
	m_pOwner->GetButton()->GetWindowRect( &buttonRect );
	m_pOwner->ScreenToClient( &buttonRect );
	CPoint origin( rectLine.right + 5, rectLine.top );

	CRect alignRect;
	GetClientRect( &alignRect );
	origin.x = __min( origin.x, alignRect.right - buttonRect.Width() );
	buttonRect += ( origin - buttonRect.TopLeft() );

	BOOL btnVisible = m_pOwner->GetButton()->IsWindowVisible();
	m_pOwner->GetButton()->MoveWindow( &buttonRect, btnVisible );
	if ( !btnVisible )
		m_pOwner->GetButton()->ShowWindow( SW_SHOWNA );

	UINT tipTextResID = 0;

	// update m_pOwner->GetButton() tooltip text
	switch ( lineIndex )
	{
		case PF_Caption:	tipTextResID = CM_EDIT_CAPTION; break;
		case PF_ID:			tipTextResID = CM_EDIT_IDENT; break;
		case PF_Style:		tipTextResID = CM_EDIT_STYLE; break;
		case PF_StyleEx:	tipTextResID = CM_EDIT_STYLE_EX; break;
		case PF_WindowRect:	tipTextResID = CM_EDIT_PLACE; break;
	}
	if ( tipTextResID != 0 )
		app::GetMainTooltip()->UpdateTipText( tipTextResID, m_pOwner->GetButton() );
}

CRect CInfoEditBox::GetLineRect( int lineIndex )
{
	int start = LineIndex( lineIndex ), end = start + LineLength( start );
	int right = PosFromChar( end ).x;

	ASSERT( lineIndex >= 0 && lineIndex < GetLineCount() );
	if ( right == -1 )
		right = PosFromChar( --end ).x;
	return CRect( PosFromChar( start ), CSize( right, m_lineHeight ) );
}

bool CInfoEditBox::CheckCaretPosition( void )
{
	int start, end, caretLine = m_lastCaretLine;
	int lineStart, lineEnd;

	GetSel( start, end );
	lineStart = lineEnd = LineFromChar( start );
	if ( start != end )
		lineEnd = LineFromChar( end );
	if ( lineStart != lineEnd )
		caretLine = -1;		// multi-line selection, avoid prompting
	else
		caretLine = lineStart;
	if ( caretLine != m_lastCaretLine )
	{
		m_lastCaretLine = caretLine;
		// notify parent about caret line changing
		m_pOwner->OnCaretLineChanged( m_lastCaretLine );
	}
	return caretLine != m_lastCaretLine;
}

LRESULT CInfoEditBox::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	LRESULT result = CEdit::WindowProc( message, wParam, lParam );

	switch ( message )
	{
		case EM_SETSEL:
		case WM_KEYDOWN:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
			CheckCaretPosition();
			m_pOwner->GetButton()->Invalidate( FALSE );
			break;
		case WM_SETFOCUS:
			SetSel( m_lastSel, TRUE );	// make selection recover
			break;
		case WM_KILLFOCUS:
			m_lastSel = GetSel();			// make selection backup
			break;
		case WM_COMMAND:
			if ( LOWORD( wParam ) == IDC_FIELD_EDIT_BUTTON )
				m_pOwner->SendMessage( WM_COMMAND, wParam, lParam );	// route m_pOwner->GetButton() commands to the owner
			break;
	}
	return result;
}


// CGeneralPage implementation

namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_GENERAL_INFO_EDIT, Size }
	};
}


CGeneralPage::CGeneralPage( void )
	: CLayoutPropertyPage( IDD_GENERAL_PAGE )
	, m_promptField( PF_None )
	, m_pInfoEdit( new CInfoEditBox( this ) )
{
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	app::GetSvc().AddObserver( this );

	m_accelPool.AddAccelTable( new CAccelTable( IDD_GENERAL_PAGE ) );
}

CGeneralPage::~CGeneralPage()
{
	app::GetSvc().RemoveObserver( this );
}

void CGeneralPage::OnCaretLineChanged( int caretLine )
{
	if ( PF_PastEnd == caretLine )
		return;

	BOOL btnVisible = m_drillButton.IsWindowVisible();

	m_promptField = caretLine != -1 ? (PromptField)caretLine : PF_None;
	if ( !IsFieldEditable( m_promptField ) && btnVisible )
		m_drillButton.ShowWindow( SW_HIDE );
	if ( IsFieldEditable( m_promptField ) )
		m_pInfoEdit->MoveButtonToLine( m_promptField );
}

bool CGeneralPage::IsFieldEditable( PromptField field ) const
{
	if ( app::IsValidTargetWnd() )
		switch ( field )
		{
			case PF_ID:
			case PF_Caption:
			case PF_Style:
			case PF_StyleEx:
			case PF_WindowRect:
				return true;
		}

	return false;
}

bool CGeneralPage::DrillDownField( PromptField field )
{
	if ( !app::CheckValidTargetWnd() )
		return false;					// window no longer exists, refresh all pages

	if ( IsFieldEditable( m_promptField ) )
		m_pInfoEdit->MoveButtonToLine( m_promptField );

	switch ( field )
	{
		case PF_Caption:	app::DrillDownDetail( CaptionDetail ); return true;
		case PF_ID:			app::DrillDownDetail( IdMenuDetail ); return true;
		case PF_Style:		app::DrillDownDetail( StyleDetail ); return true;
		case PF_StyleEx:	app::DrillDownDetail( ExtendedStyleDetail ); return true;
		case PF_WindowRect:	app::DrillDownDetail( PlacementDetail ); return true;
		default:
			ui::BeepSignal();
			return false;
	}
}

namespace hlp
{
	std::tstring FormatIdent( HWND hWnd )
	{
		if ( HasFlag( ui::GetStyle( hWnd ), WS_CHILD ) )
		{
			short ctrlId = static_cast<short>( ::GetDlgCtrlID( hWnd ) );
			return str::Format( _T("%d (0x%X)"), ctrlId, static_cast<unsigned short>( ctrlId ) );
		}
		else
			return str::Format( _T("Menu Handle=0x%X"), ::GetMenu( hWnd ) );
	}
}

void CGeneralPage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	DWORD style = 0, styleEx = 0;
	CRect rw( 0, 0, 0, 0 ), rc( 0, 0, 0, 0 );
	std::tstring info;

	if ( !targetWnd.IsValid() )
		info = _T("<n/a>");
	else
	{
		style = targetWnd.GetStyle();
		styleEx = targetWnd.GetExStyle();
		rw = ui::GetControlRect( targetWnd );
		targetWnd.GetClientRect( &rc );

		info = str::Format(
			_T("%08X\r\n")
			_T("%s\r\n")
			_T("\"%s\"\r\n")
			_T("%s\r\n")
			_T("0x%08X\r\n")
			_T("0x%08X\r\n")
			_T("(L=%d, T=%d) (R=%d, B=%d),  W=%d, H=%d\r\n")
			_T("(L=%d, T=%d) (R=%d, B=%d),  W=%d, H=%d"),
			 targetWnd.m_hWnd,
			 wc::FormatClassName( targetWnd ).c_str(),
			 wnd::FormatWindowTextLine( targetWnd ).c_str(),
			 hlp::FormatIdent( targetWnd ).c_str(),
			 style,
			 styleEx,
			 rw.left, rw.top, rw.right, rw.bottom, rw.Width(), rw.Height(),
			 rc.left, rc.top, rc.right, rc.bottom, rc.Width(), rc.Height() );
	}

	if ( !ui::SetWindowText( *m_pInfoEdit, info ) )
		return;				// text not changed

	int caretLine = m_promptField, lineStart;

	lineStart = m_pInfoEdit->LineIndex( caretLine );
	m_pInfoEdit->SetSel( lineStart, lineStart );
	OnCaretLineChanged( caretLine );

	std::vector< std::tstring > toolInfos;

	if ( targetWnd.IsValid() )
	{	// build tooltip text array
		std::tstring textBuffer; textBuffer.reserve( 512 );
		const TCHAR sep[] = _T(", ");

		if ( HWND hParent = ::GetParent( targetWnd ) )
			stream::Tag( textBuffer, str::Format( _T("Parent=%s"), wnd::FormatBriefWndInfo( hParent ).c_str() ), sep );
		if ( HWND hOwner = ::GetWindow( targetWnd, GW_OWNER ) )
			stream::Tag( textBuffer, str::Format( _T("Owner=%s"), wnd::FormatBriefWndInfo( hOwner ).c_str() ), sep );
		if ( HWND hNext = ::GetWindow( targetWnd, GW_HWNDNEXT ) )
			stream::Tag( textBuffer, str::Format( _T("Next=%s"), wnd::FormatBriefWndInfo( hNext ).c_str() ), sep );
		if ( HWND hPrevious = ::GetWindow( targetWnd, GW_HWNDPREV ) )
			stream::Tag( textBuffer, str::Format( _T("Previous=%s"), wnd::FormatBriefWndInfo( hPrevious ).c_str() ), sep );
		if ( HWND hChild = ::GetWindow( targetWnd, GW_CHILD ) )
			stream::Tag( textBuffer, str::Format( _T("First Child=%s"), wnd::FormatBriefWndInfo( hChild ).c_str() ), sep );
		toolInfos.push_back( textBuffer );

		toolInfos.push_back( std::tstring() );

		textBuffer.clear();
		toolInfos.push_back( CStyleRepository::Instance().StreamStyle( textBuffer, style, wc::GetClassName( targetWnd ) ) );

		textBuffer.clear();
		toolInfos.push_back( CStyleExRepository::Instance().StreamStyleEx( textBuffer, styleEx ) );
		toolInfos.push_back( str::Format( _T("Width=%d, Height=%d"), rw.Width(), rw.Height() ) );
		toolInfos.push_back( str::Format( _T("Width=%d, Height=%d"), rc.Width(), rc.Height() ) );
	}
	m_pInfoEdit->UpdateTooltipInfo( toolInfos );
}

void CGeneralPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_pInfoEdit->m_hWnd;
	DDX_Control( pDX, IDC_GENERAL_INFO_EDIT, *m_pInfoEdit );
	DDX_Control( pDX, IDC_FIELD_EDIT_BUTTON, m_drillButton );

	if ( firstInit )
	{
		m_pInfoEdit->SetupWindow();
		OutputTargetWnd();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CGeneralPage, CLayoutPropertyPage )
	ON_BN_CLICKED( IDC_FIELD_EDIT_BUTTON, CmFieldSetup )
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CGeneralPage::OnInitDialog( void )
{
	CLayoutPropertyPage::OnInitDialog();
	return TRUE;
}

void CGeneralPage::CmFieldSetup( void )
{
	m_pInfoEdit->SetFocus();
	DrillDownField( m_promptField );
}

HBRUSH CGeneralPage::OnCtlColor( CDC* dc, CWnd* pWnd, UINT ctlColor )
{
	HBRUSH hBrush = CLayoutPropertyPage::OnCtlColor( dc, pWnd, ctlColor );
	if ( pWnd == m_pInfoEdit.get() )
		dc->SetTextColor( HotFieldColor );
	return hBrush;
}
