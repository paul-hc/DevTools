
#include "stdafx.h"
#include "TestTaskDialog.h"
#include "CustomTaskDialogs.h"
#include "utl/EnumTags.h"
#include "utl/TaskDialog.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("dialogUsage");
	static const TCHAR entry_dialogUsage[] = _T("dialogUsage");
}


const std::tstring CTestTaskDialog::s_mainInstruction = _T("Important!\nPlease read!");
const std::tstring CTestTaskDialog::s_footer = _T("Here is some supplementary information for the user.");
const std::tstring CTestTaskDialog::s_verificationText = _T("Remember the user's settings.");

CTestTaskDialog::CTestTaskDialog( CWnd* pParent )
	: CLayoutDialog( IDD_TEST_TASK_DIALOG, pParent )
	, m_outcome( 0 )
	, m_radioButtonId( 0 )
	, m_verificationChecked( false )
	, m_usageButton( &GetTags_TaskDialogUsage() )
{
	m_regSection = reg::section_dialog;
	m_usageButton.SetUseTextSpacing();
}

CTestTaskDialog::~CTestTaskDialog()
{
}

const CEnumTags& CTestTaskDialog::GetTags_TaskDialogUsage( void )
{
	static const CEnumTags tags( _T("Basic|Command Links|Message Box|Progress Bar|Marquee Progress Bar|Navigation|Complete") );
	return tags;
}

void CTestTaskDialog::DisplayOutcome( void )
{
	ui::SetDlgItemText( m_hWnd, IDC_OUTCOME_STATIC, str::Format( _T("%d\n%d\n%s"), m_outcome, m_radioButtonId, m_verificationChecked ? _T("[X]") : _T("[ ]") ) );
}

void CTestTaskDialog::ClearOutcome( void )
{
	m_radioButtonId = 0;

	ui::SetDlgItemText( m_hWnd, IDC_OUTCOME_STATIC, std::tstring() );
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_Basic( void ) const
{
	return new CTaskDialog(
		GetTags_TaskDialogUsage().FormatUi( TD_Basic ),			// title
		s_mainInstruction,
		_T("This is an important message to the user."),		// content
		TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
		TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION );
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_CommandLinks( void ) const
{
	CTaskDialog* pDlg = new CTaskDialog(
		GetTags_TaskDialogUsage().FormatUi( TD_CommandLinks ),	// title
		s_mainInstruction,
		_T("This is an important message to the user."),		// content
		ID_COMMAND_LINK1, ID_COMMAND_LINK2, TDCBF_CANCEL_BUTTON );

	pDlg->SetMainIcon( TD_SHIELD_ICON );
	return pDlg;
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_MessageBox( void ) const
{
	return new CTaskDialog(
		GetTags_TaskDialogUsage().FormatUi( TD_MessageBox ),	// title
		std::tstring(),											// main instruction
		_T("Do you like the MFCTaskDialog sample?"),			// content
		0,
		0,
		TDCBF_YES_BUTTON | TDCBF_NO_BUTTON );
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_ProgressBar( void ) const
{
	return new my::CProgressBarTaskDialog;
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_MarqueeProgressBar( void ) const
{
	CTaskDialog* pDlg = new CTaskDialog(
		GetTags_TaskDialogUsage().FormatUi( TD_MarqueeProgressBar ),	// title
		s_mainInstruction,
		_T("This is an important message to the user."),
		TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
		TDF_ENABLE_HYPERLINKS | TDF_SHOW_MARQUEE_PROGRESS_BAR );

	pDlg->SetProgressBarMarquee( true, 2 );
	return pDlg;
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_Navigation( void ) const
{
	return new my::CFirstNavigationDialog;
}

CTaskDialog* CTestTaskDialog::MakeTaskDialog_Complete( void ) const
{
	CTaskDialog* pDlg = new CTaskDialog(
		GetTags_TaskDialogUsage().FormatUi( TD_Complete ),		// title
		s_mainInstruction,
		_T("I've got news for you:  <a href=\"https://www.theguardian.com\">The Guardian</a>"),		// Content
		TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON,
		TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_SHOW_MARQUEE_PROGRESS_BAR,
		s_footer );

	// icons
	pDlg->SetMainIcon( TD_WARNING_ICON );
	pDlg->SetFooterIcon( TD_INFORMATION_ICON );

	// marquee progress bar
	pDlg->SetProgressBarMarquee( true, 2 );

	// radio buttons
	pDlg->AddRadioButton( ID_RADIO_BUTTON1, str::Load( ID_RADIO_BUTTON1 ) );
	pDlg->AddRadioButton( ID_RADIO_BUTTON2, str::Load( ID_RADIO_BUTTON2 ) );

	// command buttons
	pDlg->AddButton( ID_COMMAND_LINK1, str::Load( ID_COMMAND_LINK1 ) );
	pDlg->AddButton( ID_COMMAND_LINK2, str::Load( ID_COMMAND_LINK2 ) );

	// expansion area
	pDlg->SetExpansionArea(
		_T("Supplementary to the user\ntyped over two lines."),
		_T("Get some additional information."),
		_T("Hide the additional information.") );

	// verification checkbox
	pDlg->SetVerificationText( s_verificationText );
	pDlg->SetVerificationChecked( m_verificationChecked );
	return pDlg;
}

void CTestTaskDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_usageButton.m_hWnd;

	DDX_Control( pDX, IDC_TASK_DIALOG_USAGE, m_usageButton );

	if ( firstInit )
	{
		m_usageButton.SetSelValue( static_cast< TaskDialogUsage >( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_dialogUsage, TD_Basic ) ) );
		ui::EnableWindow( m_usageButton, CTaskDialog::IsSupported() );
	}

	CLayoutDialog::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CTestTaskDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED( IDC_TASK_DIALOG_USAGE, OnBnClicked_TaskUsage )
END_MESSAGE_MAP()

void CTestTaskDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_dialogUsage, m_usageButton.GetSelValue() );

	__super::OnDestroy();
}

void CTestTaskDialog::OnBnClicked_TaskUsage( void )
{
	std::auto_ptr< CTaskDialog > pDlg;

	switch ( m_usageButton.GetSelEnum< TaskDialogUsage >() )
	{
		case TD_Basic:				pDlg.reset( MakeTaskDialog_Basic() ); break;
		case TD_CommandLinks:		pDlg.reset( MakeTaskDialog_CommandLinks() ); break;
		case TD_MessageBox:			pDlg.reset( MakeTaskDialog_MessageBox() ); break;
		case TD_ProgressBar:		pDlg.reset( MakeTaskDialog_ProgressBar() ); break;
		case TD_MarqueeProgressBar:	pDlg.reset( MakeTaskDialog_MarqueeProgressBar() ); break;
		case TD_Navigation:			pDlg.reset( MakeTaskDialog_Navigation() ); break;
		case TD_Complete:			pDlg.reset( MakeTaskDialog_Complete() ); break;
	}

	if ( NULL == pDlg.get() )
	{
		ASSERT( false );
		return;
	}

	ClearOutcome();
	m_outcome = pDlg->DoModal( this );

	m_radioButtonId = pDlg->GetSelectedRadioButtonID();
	if ( pDlg->UseVerification() )
		m_verificationChecked = pDlg->IsVerificationChecked() != FALSE;

	DisplayOutcome();
}
