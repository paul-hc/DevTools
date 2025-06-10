
#include "pch.h"
#include "ResizeFrameStatic.h"
#include "LayoutEngine.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_splitPct[] = _T("SplitPct");
	static const TCHAR entry_collapsed[] = _T("Collapsed");
}


// CResizeFrameStatic implementation

CResizeFrameStatic::CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl,
										resize::Orientation orientation /*= resize::NorthSouth*/, resize::ToggleStyle toggleStyle /*= resize::ToggleSecond*/ )
	: CStatic()
	, m_pGripBar( new CResizeGripBar( pFirstCtrl, pSecondCtrl, orientation, toggleStyle ) )
{
}

CResizeFrameStatic::~CResizeFrameStatic()
{
}

CWnd* CResizeFrameStatic::GetControl( void ) const implements(ui::ILayoutFrame)
{
	return const_cast<CResizeFrameStatic*>( this );
}

CWnd* CResizeFrameStatic::GetDialog( void ) const implements(ui::ILayoutFrame)
{
	return GetParent();
}

void CResizeFrameStatic::OnControlResized( void ) implements(ui::ILayoutFrame)
{
	if ( m_pGripBar->m_hWnd != nullptr )
		m_pGripBar->LayoutProportionally();
}

bool CResizeFrameStatic::ShowPane( bool show ) implements(ui::ILayoutFrame)
{
	return ui::ShowWindow( m_hWnd, show );
}

void CResizeFrameStatic::NotifyParent( Notification notification )
{
	ui::SendCommandToParent( *this, notification );
}

void CResizeFrameStatic::PreSubclassWindow( void )
{
	if ( !m_regSection.empty() )
	{
		m_pGripBar->SetFirstExtentPercentage( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_splitPct, m_pGripBar->GetFirstExtentPercentage() ) );
		m_pGripBar->SetCollapsed( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_collapsed, m_pGripBar->IsCollapsed() ) != FALSE );
	}

	__super::PreSubclassWindow();

	ui::ShowWindow( m_hWnd, false );		// hide the frame just in case is visible in dialog resource

	ASSERT_NULL( m_pGripBar->m_hWnd );
	VERIFY( m_pGripBar->CreateGripper( this ) );
}


// CResizeFrameStatic message handlers

BEGIN_MESSAGE_MAP( CResizeFrameStatic, CStatic )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CResizeFrameStatic::OnDestroy( void )
{
	if ( !m_regSection.empty() )
	{
		AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_splitPct, m_pGripBar->GetFirstExtentPercentage() );
		AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_collapsed, m_pGripBar->IsCollapsed() );
	}

	__super::OnDestroy();
}


// CLayoutStatic implementation

CLayoutStatic::CLayoutStatic( void )
	: CStatic()
	, m_pPaneLayoutEngine( new CPaneLayoutEngine() )
	, m_pResizeGripBar( nullptr )
{
}

CLayoutStatic::~CLayoutStatic()
{
}

void CLayoutStatic::SetUseSmoothTransparentGroups( bool useSmoothTransparentGroups /*= true*/ )
{
	if ( useSmoothTransparentGroups )
		m_pPaneLayoutEngine->ModifyFlags( 0, CLayoutEngine::SmoothTransparentGroups );	// groups will use WS_EX_TRANSPARENT styleEx, parent dialog uses WS_CLIPCHILDREN style
	else
		m_pPaneLayoutEngine->ModifyFlags( CLayoutEngine::GroupsTransparentEx, 0 );
}

CLayoutEngine& CLayoutStatic::GetLayoutEngine( void ) implements(ui::ILayoutEngine)
{
	return *m_pPaneLayoutEngine;
}

void CLayoutStatic::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) implements(ui::ILayoutEngine)
{
	m_pPaneLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutStatic::HasControlLayout( void ) const implements(ui::ILayoutEngine)
{
	return m_pPaneLayoutEngine->HasCtrlLayout();
}

CWnd* CLayoutStatic::GetControl( void ) const implements(ui::ILayoutFrame)
{
	return const_cast<CLayoutStatic*>( this );
}

CWnd* CLayoutStatic::GetDialog( void ) const implements(ui::ILayoutFrame)
{
	return GetParent();
}

void CLayoutStatic::OnControlResized( void ) implements(ui::ILayoutFrame)
{
	if ( !m_pPaneLayoutEngine->IsInitialized() )
	{
		m_pPaneLayoutEngine->InitializePane( this );

		if ( m_pResizeGripBar != nullptr )
			if ( m_pResizeGripBar->IsCollapsed() )
				ShowPane( false );					// initially hide this collapsed pane's controls
	}

	ENSURE( m_pPaneLayoutEngine->IsInitialized() );
	m_pPaneLayoutEngine->LayoutControls();
}

bool CLayoutStatic::ShowPane( bool show ) implements(ui::ILayoutFrame)
{
	return m_pPaneLayoutEngine->ShowPaneControls( show );
}

CResizeGripBar* CLayoutStatic::GetSplitterGripBar( void ) const implements( ui::ILayoutFrame )
{
	return m_pResizeGripBar;
}

void CLayoutStatic::SetSplitterGripBar( CResizeGripBar* pResizeGripBar ) implements( ui::ILayoutFrame )
{
	m_pResizeGripBar = pResizeGripBar;
}


// CLayoutStatic message handlers

BEGIN_MESSAGE_MAP( CLayoutStatic, CStatic )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CLayoutStatic::OnEraseBkgnd( CDC* pDC )
{
	pDC;
	return TRUE;		// prevent default erase
}

void CLayoutStatic::OnPaint( void )
{
	CPaintDC dc( this );
	CRect clientRect;

	GetClientRect( &clientRect );
	ui::FrameRect( dc, clientRect, color::Red );
}
