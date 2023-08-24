
#include "pch.h"
#include "LayoutPaneDialog.h"
#include "LayoutEngine.h"
#include "CmdInfoStore.h"
#include "CmdUpdate.h"
#include "ui_fwd.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_SERIAL( CLayoutPaneDialog, CPaneDialog, VERSIONABLE_SCHEMA | 1 )

CLayoutPaneDialog::CLayoutPaneDialog( bool fillToolBarBkgnd /*= true*/ )
	: CPaneDialog()
	, m_pLayoutEngine( new CLayoutEngine() )
	, m_fillToolBarBkgnd( fillToolBarBkgnd )
{
}

CLayoutPaneDialog::~CLayoutPaneDialog()
{
}

CLayoutEngine& CLayoutPaneDialog::GetLayoutEngine( void )
{
	return *m_pLayoutEngine;
}

void CLayoutPaneDialog::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutPaneDialog::HasControlLayout( void ) const
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutPaneDialog::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	rText, cmdId, pTooltip;
}

void CLayoutPaneDialog::Serialize( CArchive& archive ) overrides(CDockablePane)
{
	__super::Serialize( archive );

	archive & m_fillToolBarBkgnd;
}

void CLayoutPaneDialog::CopyState( CDockablePane* pSrc ) overrides(CDockablePane)
{
	__super::CopyState( pSrc );

	CLayoutPaneDialog* pSrcDlgBar = checked_static_cast<CLayoutPaneDialog*>( pSrc );

	m_fillToolBarBkgnd = pSrcDlgBar->m_fillToolBarBkgnd;

}

void CLayoutPaneDialog::DoDataExchange( CDataExchange* pDX )
{
	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( !m_pLayoutEngine->IsInitialized() )
		{
			m_pLayoutEngine->Initialize( this );
			m_pLayoutEngine->LayoutControls();			// initial layout
			EnableToolTips( TRUE );
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutPaneDialog, CPaneDialog )
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_INITMENUPOPUP()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXT, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
END_MESSAGE_MAP()

int CLayoutPaneDialog::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == __super::OnCreate( pCreateStruct ) )
		return -1;

	ASSERT( !m_pLayoutEngine->IsInitialized() );
	m_pLayoutEngine->StoreInitialSize( this );
	return 0;
}

void CLayoutPaneDialog::OnSize( UINT sizeType, int cx, int cy )
{
	if ( sizeType != SIZE_MINIMIZED && cx > 0 && cy > 0 )
		if ( m_pLayoutEngine->IsInitialized() )
			m_pLayoutEngine->LayoutControls();

	__super::OnSize( sizeType, cx, cy );
}

void CLayoutPaneDialog::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

BOOL CLayoutPaneDialog::OnEraseBkgnd( CDC* pDC )
{
	if ( m_fillToolBarBkgnd )
	{
		CRect clientRect;
		GetClientRect( &clientRect );

		CMFCVisualManager::GetInstance()->OnFillBarBackground( pDC, this, clientRect, clientRect );
		return true;
	}
	else
		return
			m_pLayoutEngine->HandleEraseBkgnd( pDC ) ||
			__super::OnEraseBkgnd( pDC );
}

BOOL CLayoutPaneDialog::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// handled

	return Default() != 0;
}
