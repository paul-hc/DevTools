
#include "pch.h"
#include "TestDoc.h"
#include "TestFormView.h"
#include "TestTaskDialog.h"
#include "TestToolbarDialog.h"
#include "DemoTemplate.h"
#include "ImageDialog.h"
#include "FileListDialog.h"
#include "FileChecksumsDialog.h"
#include "BuddyControlsDialog.h"
#include "TestColorsDialog.h"
#include "test/ImageTests.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_formView[] = _T("TestForm");
	static const TCHAR entry_modelessBuddyDlg[] = _T("ModelessBuddyDlg");
	static const TCHAR entry_miscDlgButton[] = _T("MiscDlgButton");
}


IMPLEMENT_DYNCREATE( CTestFormView, CFormView )

CTestFormView::CTestFormView( void )
	: CLayoutFormView( IDD_DEMO_FORM )
	, m_pDemo( new CDemoTemplate( this ) )
	, m_miscDlgButton( &GetTags_MiscDialog() )
{
	m_miscDlgButton.SetSelValue( AfxGetApp()->GetProfileInt( reg::section_formView, reg::entry_miscDlgButton, m_miscDlgButton.GetSelValue() ) );
}

CTestFormView::~CTestFormView()
{
}

const CEnumTags& CTestFormView::GetTags_MiscDialog( void )
{
	static const CEnumTags s_tags( _T("Toolbar Dialog|Colors Dialog") );
	return s_tags;
}

void CTestFormView::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	m_pDemo->QueryTooltipText( rText, cmdId, pTooltip );

	if ( rText.empty() )
		__super::QueryTooltipText( rText, cmdId, pTooltip );
}

void CTestFormView::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_pDemo->m_formatCombo.m_hWnd;

	m_pDemo->DoDataExchange( pDX );
	ui::DDX_ButtonIcon( pDX, IDC_RUN_IMAGE_TESTS, ID_RUN_TESTS );
	ui::DDX_ButtonIcon( pDX, ID_STUDY_IMAGE );
	DDX_Control( pDX, ID_STUDY_MISC_DIALOG, m_miscDlgButton );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		if ( firstInit )
			CheckDlgButton( IDC_BUDDY_MODELESS_CHECK, AfxGetApp()->GetProfileInt( reg::section_formView, reg::entry_modelessBuddyDlg, BST_UNCHECKED ) );

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

void CTestFormView::OnIdleUpdateControls( void )
{
	__super::OnIdleUpdateControls();
	ui::UpdateDlgItemUI( this, IDC_PASTE_FILES_BUTTON );
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
	ON_BN_CLICKED( ID_STUDY_FILE_CHECKSUM, OnStudyFileChecksums )
	ON_BN_CLICKED( ID_STUDY_BUDDY_CONTROLS, OnStudyBuddyControls )
	ON_BN_CLICKED( ID_STUDY_TASK_DIALOG, OnStudyTaskDialog )
	ON_BN_CLICKED( ID_STUDY_MISC_DIALOG, OnStudyMiscDialog )
	ON_BN_CLICKED( IDC_BUDDY_MODELESS_CHECK, OnToggle_ModelessBuddyDlg )
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
		CFileListDialog dlg( AfxGetMainWnd() );
		btnId = dlg.DoModal();
	}
}

void CTestFormView::OnStudyFileChecksums( void )
{
	CFileChecksumsDialog dlg( AfxGetMainWnd() );
	dlg.DoModal();
}

void CTestFormView::OnStudyBuddyControls( void )
{
	if ( !IsDlgButtonChecked( IDC_BUDDY_MODELESS_CHECK ) )
	{
		CBuddyControlsDialog dlg( AfxGetMainWnd() );
		dlg.DoModal();
	}
	else
	{
		CBuddyControlsDialog* pDlg = new CBuddyControlsDialog( nullptr );
		pDlg->CreateModeless();
	}
}

void CTestFormView::OnStudyTaskDialog( void )
{
	CTestTaskDialog dlg( this );
	dlg.DoModal();
}

void CTestFormView::OnStudyMiscDialog( void )
{
	MiscDialog miscDlg = m_miscDlgButton.GetSelEnum<MiscDialog>();
	AfxGetApp()->WriteProfileInt( reg::section_formView, reg::entry_miscDlgButton, miscDlg );

	std::auto_ptr<CDialog> pDlg;

	switch ( miscDlg )
	{
		case ToolbarDialog:
			pDlg.reset( new CTestToolbarDialog( this ) );
			break;
		case TestColorsDialog:
			pDlg.reset( new CTestColorsDialog( this ) );
			break;
		default:
			break;
	}

	if ( pDlg.get() != nullptr )
		pDlg->DoModal();
}

void CTestFormView::OnToggle_ModelessBuddyDlg( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_formView, reg::entry_modelessBuddyDlg, IsDlgButtonChecked( IDC_BUDDY_MODELESS_CHECK ) );
}
