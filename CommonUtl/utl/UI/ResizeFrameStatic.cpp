
#include "StdAfx.h"
#include "ResizeFrameStatic.h"
#include "ResizeGripBar.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_splitPct[] = _T("SplitPct");
	static const TCHAR entry_collapsed[] = _T("Collapsed");
}


CResizeFrameStatic::CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl, CResizeGripBar* pGripper )
	: CStatic()
	, m_pGripBar( pGripper )
	, m_pFirstCtrl( pFirstCtrl )
	, m_pSecondCtrl( pSecondCtrl )
{
	ASSERT_PTR( m_pFirstCtrl );
	ASSERT_PTR( m_pSecondCtrl );
	ASSERT_PTR( m_pGripBar.get() );
}

CResizeFrameStatic::~CResizeFrameStatic()
{
}

void CResizeFrameStatic::OnControlResized( UINT ctrlId )
{
	if ( GetDlgCtrlID() == ui::ToCmdId( ctrlId ) )
		if ( m_pGripBar->m_hWnd != NULL )
			m_pGripBar->LayoutProportionally();
}

void CResizeFrameStatic::NotifyParent( Notification notification )
{
	ui::SendCommandToParent( *this, notification );
}

void CResizeFrameStatic::PreSubclassWindow( void )
{
	ASSERT_PTR( m_pFirstCtrl->GetSafeHwnd() );
	ASSERT_PTR( m_pSecondCtrl->GetSafeHwnd() );

	if ( !m_regSection.empty() )
	{
		m_pGripBar->SetFirstExtentPercentage( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_splitPct, m_pGripBar->GetFirstExtentPercentage() ) );
		m_pGripBar->SetCollapsed( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_collapsed, m_pGripBar->IsCollapsed() ) != FALSE );
	}

	__super::PreSubclassWindow();

	if ( NULL == m_pGripBar->m_hWnd )
		VERIFY( m_pGripBar->CreateGripper( this, m_pFirstCtrl, m_pSecondCtrl ) );
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
