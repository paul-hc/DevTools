
#include "stdafx.h"
#include "LoggerSetupDialog.h"
#include "Application.h"
#include "resource.h"
#include "utl/Logger.h"
#include "utl/Path.h"
#include "utl/UI/ShellUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CLoggerSetupDialog::CLoggerSetupDialog( CWnd* pParent /*=nullptr*/ )
	: CDialog( IDD_LOGGER_DIALOG, pParent )
	, m_pCurrLogger( nullptr )
{
}

void CLoggerSetupDialog::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_LOGGER_COMBO, m_selLogCombo);
}


// message handlers

BEGIN_MESSAGE_MAP( CLoggerSetupDialog, CDialog )
	ON_CBN_SELCHANGE( IDC_LOGGER_COMBO, OnCBnSelChangeLogger )
	ON_BN_CLICKED( IDC_LOGGER_ENABLED_CHECK, OnToggle_LoggerEnabled )
	ON_BN_CLICKED( IDC_TIMESTAMP_PREFIX_CHECK, OnToggle_TimestampPrefix )
	ON_BN_CLICKED( IDC_VIEW_LOG_FILE_BUTTON, OnViewLogFileButton )
	ON_BN_CLICKED( IDC_CLEAR_LOG_FILE_BUTTON, OnClearLogFileButton )
END_MESSAGE_MAP()

BOOL CLoggerSetupDialog::OnInitDialog( void )
{
	CDialog::OnInitDialog();

	m_selLogCombo.SetItemDataPtr( 0, &app::GetApp()->GetLogger() );
	m_selLogCombo.SetItemDataPtr( 1, &app::GetApp()->GetEventLogger() );
	m_selLogCombo.SetCurSel( AfxGetApp()->GetProfileInt( _T("Settings"), _T("LastActiveLogger"), 0 ) );
	OnCBnSelChangeLogger();
	return TRUE;
}

void CLoggerSetupDialog::OnOK( void )
{
	AfxGetApp()->WriteProfileInt( _T("Settings"), _T("LastActiveLogger"), m_selLogCombo.GetCurSel() );
	CDialog::OnOK();
}

void CLoggerSetupDialog::OnCancel( void )
{
	OnOK();
}

void CLoggerSetupDialog::OnCBnSelChangeLogger( void )
{
	m_pCurrLogger = (CLogger*)m_selLogCombo.GetItemDataPtr( m_selLogCombo.GetCurSel() );
	ASSERT_PTR( m_pCurrLogger );

	CheckDlgButton( IDC_LOGGER_ENABLED_CHECK, m_pCurrLogger->m_enabled );
	CheckDlgButton( IDC_TIMESTAMP_PREFIX_CHECK, m_pCurrLogger->m_prependTimestamp );
	GetDlgItem( IDC_VIEW_LOG_FILE_BUTTON )->EnableWindow( m_pCurrLogger->GetLogFilePath().FileExist() );
}

void CLoggerSetupDialog::OnToggle_LoggerEnabled( void )
{
	ASSERT_PTR( m_pCurrLogger );
	m_pCurrLogger->m_enabled = IsDlgButtonChecked( IDC_LOGGER_ENABLED_CHECK ) != FALSE;
}

void CLoggerSetupDialog::OnToggle_TimestampPrefix( void )
{
	ASSERT_PTR( m_pCurrLogger );
	m_pCurrLogger->m_prependTimestamp = IsDlgButtonChecked( IDC_TIMESTAMP_PREFIX_CHECK ) != FALSE;
}

void CLoggerSetupDialog::OnViewLogFileButton( void )
{
	// use text key (.txt) for text view, or the default for run
	shell::Execute( this, m_pCurrLogger->GetLogFilePath().GetPtr(), nullptr, SEE_MASK_FLAG_DDEWAIT, nullptr, nullptr, _T(".txt") );
}

void CLoggerSetupDialog::OnClearLogFileButton( void )
{
	m_pCurrLogger->Clear();
}
