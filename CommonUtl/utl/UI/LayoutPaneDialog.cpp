
#include "pch.h"
#include "LayoutPaneDialog.h"
#include "LayoutEngine.h"
#include "CmdTagStore.h"
#include "CmdUpdate.h"
#include "ui_fwd.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_SERIAL( CLayoutPaneDialog, CPaneDialog, VERSIONABLE_SCHEMA | 1 )

CLayoutPaneDialog::CLayoutPaneDialog( void )
	: CPaneDialog()
	, m_pLayoutEngine( new CLayoutEngine() )
	, m_fillToolBarBkgnd( false )
	, m_showControlBarMenu( true )
{
	// Note: if you want that the panes are not persisted to registry, call in the parent frame:
	//	GetDockingManager()->DisableRestoreDockState( TRUE );		// to disable loading of docking layout from the Registry
}

CLayoutPaneDialog::~CLayoutPaneDialog()
{
}

CLayoutEngine& CLayoutPaneDialog::GetLayoutEngine( void ) implements( ui::ILayoutEngine )
{
	return *m_pLayoutEngine;
}

void CLayoutPaneDialog::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) implements(ui::ILayoutEngine)
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutPaneDialog::HasControlLayout( void ) const implements(ui::ILayoutEngine)
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutPaneDialog::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const implements(ui::ICustomCmdInfo)
{
	rText, cmdId, pTooltip;
}

BOOL CLayoutPaneDialog::OnShowControlBarMenu( CPoint point ) overrides( CPane )
{
	if ( !m_showControlBarMenu )
		return TRUE;		// prevent displaying the panel context menu

	return __super::OnShowControlBarMenu( point );
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

	CRect windowsRect;
	GetWindowRect( &windowsRect );
	SetMinSize( windowsRect.Size() );
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
	if ( ui::CCmdTagStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// handled

	return Default() != 0;
}
