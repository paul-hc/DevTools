
#include "pch.h"
#include "LayoutFormView.h"
#include "LayoutEngine.h"
#include "CmdTagStore.h"
#include "CmdUpdate.h"
#include "ui_fwd.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CLayoutFormView::CLayoutFormView( UINT templateId )
	: CFormView( templateId )
	, m_pLayoutEngine( new CLayoutEngine() )
{
}

CLayoutFormView::CLayoutFormView( const TCHAR* pTemplateName )
	: CFormView( pTemplateName )
	, m_pLayoutEngine( new CLayoutEngine() )
{
}

CLayoutFormView::~CLayoutFormView()
{
}

CLayoutEngine& CLayoutFormView::GetLayoutEngine( void )
{
	return *m_pLayoutEngine;
}

void CLayoutFormView::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutFormView::HasControlLayout( void ) const
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutFormView::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	rText, cmdId, pTooltip;
}

void CLayoutFormView::OnIdleUpdateControls( void )
{
	// note: child toolbars receive WM_IDLEUPDATECMDUI from MFC by default
}

void CLayoutFormView::DoDataExchange( CDataExchange* pDX )
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

BOOL CLayoutFormView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	if ( CWinThread* pThread = AfxGetThread() )
		if ( pThread->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )			// last crack goes to the current CWinThread object
			return TRUE;

	return FALSE;
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutFormView, CFormView )
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_INITMENUPOPUP()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_MESSAGE_VOID( WM_IDLEUPDATECMDUI, OnIdleUpdateControls )
END_MESSAGE_MAP()

int CLayoutFormView::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == __super::OnCreate( pCreateStruct ) )
		return -1;

	ASSERT( !m_pLayoutEngine->IsInitialized() );
	m_pLayoutEngine->StoreInitialSize( this );
	return 0;
}

void CLayoutFormView::OnSize( UINT sizeType, int cx, int cy )
{
	if ( sizeType != SIZE_MINIMIZED && cx > 0 && cy > 0 )
		if ( m_pLayoutEngine->IsInitialized() )
			m_pLayoutEngine->LayoutControls();

	__super::OnSize( sizeType, cx, cy );
}

void CLayoutFormView::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

BOOL CLayoutFormView::OnEraseBkgnd( CDC* pDC )
{
	return
		m_pLayoutEngine->HandleEraseBkgnd( pDC ) ||
		__super::OnEraseBkgnd( pDC );
}

BOOL CLayoutFormView::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	if ( ui::CCmdTagStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// handled

	return Default() != 0;
}
