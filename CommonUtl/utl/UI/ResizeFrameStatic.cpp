
#include "pch.h"
#include "ResizeFrameStatic.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_splitPct[] = _T("SplitPct");
	static const TCHAR entry_collapsed[] = _T("Collapsed");
}


CResizeFrameStatic::CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl,
										resize::Orientation orientation /*= resize::NorthSouth*/, resize::ToggleStyle toggleStyle /*= resize::ToggleSecond*/ )
	: CStatic()
	, m_pGripBar( new CResizeGripBar( pFirstCtrl, pSecondCtrl, orientation, toggleStyle ) )
{
}

CResizeFrameStatic::~CResizeFrameStatic()
{
}

void CResizeFrameStatic::OnControlResized( UINT ctrlId )
{
	if ( GetDlgCtrlID() == ui::ToIntCmdId( ctrlId ) )
		if ( m_pGripBar->m_hWnd != nullptr )
			m_pGripBar->LayoutProportionally();
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


// message handlers

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
