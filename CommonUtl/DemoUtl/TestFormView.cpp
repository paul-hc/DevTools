
#include "stdafx.h"
#include "TestDoc.h"
#include "TestFormView.h"
#include "DemoTemplate.h"
#include "ImageDialog.h"
#include "FileStatesDialog.h"
#include "ImageTests.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CTestFormView, CFormView )

CTestFormView::CTestFormView( void )
	: CLayoutFormView( IDD_DEMO_FORM )
	, m_pDemo( new CDemoTemplate( this ) )
{
}

CTestFormView::~CTestFormView()
{
}

void CTestFormView::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	m_pDemo->QueryTooltipText( rText, cmdId, pTooltip );
	if ( rText.empty() )
		CLayoutFormView::QueryTooltipText( rText, cmdId, pTooltip );
}

void CTestFormView::DoDataExchange( CDataExchange* pDX )
{
	m_pDemo->DoDataExchange( pDX );
	ui::DDX_ButtonIcon( pDX, IDC_RUN_IMAGE_TESTS, ID_RUN_TESTS );
	ui::DDX_ButtonIcon( pDX, ID_STUDY_IMAGE );

#ifndef _DEBUG
	ui::EnableControl( m_hWnd, ID_RUN_TESTS, false );
	ui::EnableControl( m_hWnd, IDC_RUN_IMAGE_TESTS, false );
#endif

	CLayoutFormView::DoDataExchange( pDX );
}

BOOL CTestFormView::PreCreateWindow( CREATESTRUCT& cs )
{
	return CLayoutFormView::PreCreateWindow( cs );
}

BOOL CTestFormView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pDemo->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		CLayoutFormView::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CTestFormView::OnInitialUpdate( void )
{
	CLayoutFormView::OnInitialUpdate();
	ResizeParentToFit();
}

BEGIN_MESSAGE_MAP( CTestFormView, CLayoutFormView )
	ON_BN_CLICKED( IDC_RUN_IMAGE_TESTS, OnRunImageUnitTests )
	ON_BN_CLICKED( ID_STUDY_IMAGE, OnStudyImage )
	ON_BN_CLICKED( ID_STUDY_LIST_DIFFS, OnStudyListDiffs )
END_MESSAGE_MAP()

void CTestFormView::OnRunImageUnitTests( void )
{
#ifdef _DEBUG
	CImageTests::Instance().Run();
#endif
}

void CTestFormView::OnStudyImage( void )
{
	CImageDialog dlg( AfxGetMainWnd() );
	dlg.DoModal();
}

void CTestFormView::OnStudyListDiffs( void )
{
	for ( INT_PTR btnId = IDRETRY; IDRETRY == btnId; )
	{
		CFileStatesDialog dlg( AfxGetMainWnd() );
		btnId = dlg.DoModal();
	}
}
