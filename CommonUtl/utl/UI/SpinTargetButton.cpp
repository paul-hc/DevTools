
#include "stdafx.h"
#include "SpinTargetButton.h"
#include "SpinEdit.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSpinTargetButton::CSpinTargetButton( CWnd* pBuddyCtrl, ui::ISpinTarget* pSpinTarget )
	: m_pBuddyCtrl( pBuddyCtrl )
	, m_pSpinTarget( pSpinTarget )
{
	ASSERT_PTR( m_pBuddyCtrl );
}

CSpinTargetButton::~CSpinTargetButton()
{
}

bool CSpinTargetButton::IsBuddyEditable( void ) const
{
	ASSERT_PTR( m_pBuddyCtrl->GetSafeHwnd() );
	if ( !m_pBuddyCtrl->IsWindowEnabled() )
		return false;
	if ( is_a< CEdit >( m_pBuddyCtrl ) && HasFlag( m_pBuddyCtrl->GetStyle(), ES_READONLY ) )
		return false;
	return true;
}

bool CSpinTargetButton::Create( DWORD alignment /*= UDS_ALIGNRIGHT*/ )
{
	ASSERT_NULL( GetSafeHwnd() );
	ASSERT_PTR( m_pBuddyCtrl->GetSafeHwnd() );

	DWORD style = WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_NOTHOUSANDS | alignment;
	if ( !CSpinButtonCtrl::Create( style, CRect( 0, 0, 0, 0 ), m_pBuddyCtrl->GetParent(), UINT_MAX ) )
		return false;

	SetWindowPos( m_pBuddyCtrl, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE );
	SetBuddy( m_pBuddyCtrl );
	SetRange32( INT_MIN, INT_MAX );
	UpdateState();
	return true;
}

void CSpinTargetButton::UpdateState( void )
{
	ASSERT( GetSafeHwnd() != NULL && m_pBuddyCtrl->GetSafeHwnd() != NULL );

	ui::EnableWindow( *this, IsBuddyEditable() );
	Invalidate();
	m_pBuddyCtrl->Invalidate();
}

void CSpinTargetButton::Layout( void )
{
	ASSERT( GetSafeHwnd() != NULL && m_pBuddyCtrl->GetSafeHwnd() != NULL );

	CRect spinRect = ui::GetControlRect( *this );
	CRect editRect = ui::GetControlRect( *m_pBuddyCtrl );

	if ( HasFlag( GetStyle(), UDS_ALIGNRIGHT ) )
		spinRect += CPoint( editRect.right - 1, editRect.top ) - spinRect.TopLeft();
	else	// UDS_ALIGNLEFT
		spinRect += editRect.TopLeft() - spinRect.TopLeft() - CSize( spinRect.Width(), 0 );

	MoveWindow( &spinRect );		// buddy edit gets moved automatically to the left
}


// message handlers

BEGIN_MESSAGE_MAP( CSpinTargetButton, CSpinButtonCtrl )
	ON_NOTIFY_REFLECT_EX( UDN_DELTAPOS, OnUdnDeltaPos )
END_MESSAGE_MAP()

BOOL CSpinTargetButton::OnUdnDeltaPos( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMUPDOWN* pUpDown = (NMUPDOWN*)pNmHdr;

	if ( m_pSpinTarget != NULL )
		*pResult = !IsBuddyEditable() || m_pSpinTarget->SpinBy( pUpDown->iDelta );	// skip default processing if spinned
	else
		*pResult = FALSE;		// do default incrementing
	return FALSE;				// pass notification to parent 
}
