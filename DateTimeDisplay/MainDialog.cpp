
#include "stdafx.h"
#include "Application.h"
#include "MainDialog.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("MainDialog");
	static const TCHAR entry_inputText[] = _T("Input Text");
	static const TCHAR entry_inputSel[] = _T("Input Sel");


	template< typename IntType >
	void SaveRange( const Range<IntType>& range, const TCHAR* pSection, const TCHAR* pEntry )
	{
		std::tostringstream oss;
		oss << range.m_start << _T(" ") << range.m_end;
		AfxGetApp()->WriteProfileString( pSection, pEntry, oss.str().c_str() );
	}

	template< typename IntType >
	bool LoadRange( Range<IntType>& rRange, const TCHAR* pSection, const TCHAR* pEntry )
	{
		std::tstring text = (LPCTSTR)AfxGetApp()->GetProfileString( pSection, pEntry );
		if ( text.empty() )
			return false;

		Range<IntType> range;
		std::tistringstream iss( text );
		iss >> range.m_start >> std::skipws >> range.m_end;
		if ( iss.fail() )
			return false;
		rRange = range;
		return true;
	}
}


static const TCHAR lineEnd[] = _T("\r\n");


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_SOURCE_TEXT_EDIT, SizeY },
		{ IDC_DATE_TIME_RESULTS_LABEL, SizeX },
		{ IDC_DATE_TIME_RESULTS_EDIT, Size },
		{ IDC_CURRLINE_STATIC, MoveY },
		{ IDC_CURRLINE_PICKER, MoveY },
		{ IDC_LINE_FORMAT_STATIC, MoveY },
		{ IDC_LINE_FORMAT_COMBO, MoveY },
		{ IDC_CURRLINE_APPLY_BUTTON, MoveY },
		{ IDC_CLEAR_BUTTON, Move },
		{ IDOK, Move }
	};
}

const TCHAR* CMainDialog::m_format[] = { _T("HH:mm:ss  ddd dd MMM yyy"), _T("HH:mm:ss"), _T(" ") };
const CTime CMainDialog::m_midnight( 2016, 1, 1, 0, 0, 0 );						// "Jan 1, 2016 - 00:00:00"

CMainDialog::CMainDialog( CWnd* pParent /*= NULL*/ )
	: CBaseMainDialog( IDD_MAIN_DIALOG, pParent )
	, m_inputText( (LPCTSTR)AfxGetApp()->GetProfileString( reg::section, reg::entry_inputText, GetDefaultInputText().c_str() ) )
	, m_inputSel( 0, -1 )
	, m_inputEdit( true )
	, m_currTypeCombo( &CDateTimeInfo::GetTags_Type() )
	, m_syncScrolling( SB_VERT )
{
	m_regSection = reg::section;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	reg::LoadRange( m_inputSel, reg::section, reg::entry_inputSel );

	m_inputEdit.SetKeepSelOnFocus();
	m_resultsEdit.SetKeepSelOnFocus();
}

const std::tstring CMainDialog::GetDefaultInputText( const CTime& dateTime /*= CTime( 1446294619 )*/ )
{
	static const CTimeSpan span( 1, 2, 35, 0 );
	const CTime before = dateTime - span;
	return str::Format( _T("%I64d\r\n%I64d\r\n#%I64d"), before.GetTime(), dateTime.GetTime(), span.GetTimeSpan() );
}

void CMainDialog::SaveToRegistry( void )
{
	CBaseMainDialog::SaveToRegistry();
	AfxGetApp()->WriteProfileString( reg::section, reg::entry_inputText, m_inputText.c_str() );
	reg::SaveRange( m_inputSel, reg::section, reg::entry_inputSel );
}

void CMainDialog::ParseInput( void )
{
	std::vector< std::tstring > lines;
	str::Split( lines, m_inputText.c_str(), lineEnd );

	m_infos.clear();
	m_infos.reserve( lines.size() );

	for ( std::vector< std::tstring >::const_iterator itLine = lines.begin(); itLine != lines.end(); ++itLine )
		m_infos.push_back( CDateTimeInfo( *itLine ) );

	std::vector< std::tstring > outputLines;
	outputLines.reserve( m_infos.size() );

	const CDateTimeInfo* pPrevInfo = NULL;

	for ( std::vector< CDateTimeInfo >::const_iterator itInfo = m_infos.begin(); itInfo != m_infos.end(); ++itInfo )
	{
		std::tstring text = itInfo->Format();

		if ( pPrevInfo != NULL )
		{
			CDurationInfo durationInfo( *pPrevInfo, *itInfo );
			if ( durationInfo.m_isValid )
				text += str::Format( _T("	 %s"), durationInfo.Format().c_str() );
		}

		outputLines.push_back( text );
		pPrevInfo = &*itInfo;
	}

	SetResultsText( str::Join( outputLines, lineEnd ) );
}

void CMainDialog::ReadInputSelLine( void )
{
	std::tstring caretLineText = m_inputEdit.GetLineTextAt();
	if ( caretLineText == m_inputCaretLineText )
		return;

	m_inputCaretLineText = caretLineText;

	static const CTime* pNullTime = NULL;
	CDateTimeInfo lineInfo( m_inputCaretLineText );

	CScopedInternalChange internalChange( &m_lineChange );
	if ( lineInfo.IsTimeField() )
	{
		SetFormatCurrLinePicker( FmtDateTime );
		m_currPicker.SetTime( &lineInfo.m_time );
	}
	else if ( lineInfo.IsDurationField() )
	{
		SetFormatCurrLinePicker( FmtDuration, &lineInfo.m_duration );
		CTime durationAsTime = m_midnight + lineInfo.m_duration;
		m_currPicker.SetTime( &durationAsTime );
	}
	else
	{
		SetFormatCurrLinePicker( FmtEmpty );
		m_currPicker.SetTime( pNullTime );
	}

	m_currTypeCombo.SetValue( lineInfo.m_type );
	ui::EnableControl( m_hWnd, IDC_CURRLINE_APPLY_BUTTON, false );
}

bool CMainDialog::SetFormatCurrLinePicker( PickerFormat pickerFormat, const CTimeSpan* pDuration /*= NULL*/ )
{
	std::tstring format = m_format[ pickerFormat ];
	if ( FmtDuration == pickerFormat && pDuration != NULL && pDuration->GetDays() != 0 )
		format += str::Format( _T(" '(+ %d days)'"), pDuration->GetDays() );			// add total days suffix

	if ( format == m_currFormat )
		return false;					// same format, no change

	m_currFormat = format;
	m_currPicker.SetFormat( m_currFormat.c_str() );
	return true;
}

void CMainDialog::EnableApplyButton( void )
{
	m_inputEdit.SetSelRange( m_inputEdit.GetLineRangeAt() );
	ui::EnableControl( m_hWnd, IDC_CURRLINE_APPLY_BUTTON, true );
}

void CMainDialog::SetResultsText( const std::tstring& outputText )
{
	int startChar, endChar;
	m_resultsEdit.GetSel( startChar, endChar );

	ui::SetWindowText( m_resultsEdit, outputText );
	m_resultsEdit.SetSel( startChar, endChar );

	m_syncScrolling.Synchronize( &m_inputEdit );			// sync v-scroll pos
}

void CMainDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_inputEdit.m_hWnd;

	DDX_Control( pDX, IDC_SOURCE_TEXT_EDIT, m_inputEdit );
	DDX_Control( pDX, IDC_DATE_TIME_RESULTS_EDIT, m_resultsEdit );
	DDX_Control( pDX, IDC_CURRLINE_PICKER, m_currPicker );
	DDX_Control( pDX, IDC_LINE_FORMAT_COMBO, m_currTypeCombo );
	ui::DDX_Text( pDX, IDC_SOURCE_TEXT_EDIT, m_inputText );
	ui::DDX_EditSel( pDX, IDC_SOURCE_TEXT_EDIT, m_inputSel );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_syncScrolling
				.AddCtrl( &m_inputEdit )
				.AddCtrl( &m_resultsEdit );

			SetFormatCurrLinePicker( FmtDateTime );
			m_inputEdit.EnsureCaretVisible();
		}
	}

	ParseInput();

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainDialog, CBaseMainDialog )
	ON_MESSAGE( WM_KICKIDLE, OnKickIdle )
	ON_EN_CHANGE( IDC_SOURCE_TEXT_EDIT, OnEnChangeSourceTextEdit )
	ON_CONTROL_RANGE( EN_VSCROLL, IDC_SOURCE_TEXT_EDIT, IDC_DATE_TIME_RESULTS_EDIT, OnEnVScroll )
	ON_CBN_SELCHANGE( IDC_LINE_FORMAT_COMBO, OnCbnSelChange_CurrTypeCombo )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnSelectAll )
	ON_COMMAND( IDC_CURRLINE_APPLY_BUTTON, OnCurrLineApply )
	ON_COMMAND( IDC_CLEAR_BUTTON, OnClearAll )
	ON_NOTIFY( DTN_DATETIMECHANGE, IDC_CURRLINE_PICKER, OnDateTimeChange_CurrLinePicker )
END_MESSAGE_MAP()

LRESULT CMainDialog::OnKickIdle( WPARAM, LPARAM )
{
	m_inputEdit.GetSel( m_inputSel.m_start, m_inputSel.m_end );
	ReadInputSelLine();
	return Default();
}

void CMainDialog::OnEnChangeSourceTextEdit( void )
{
	UpdateData( DialogSaveChanges );
}

void CMainDialog::OnEnVScroll( UINT editId )
{
	m_syncScrolling.Synchronize( GetDlgItem( editId ) );
}

void CMainDialog::OnCbnSelChange_CurrTypeCombo( void )
{
	EnableApplyButton();
}

void CMainDialog::OnDateTimeChange_CurrLinePicker( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	*pResult = 0;

	if ( !m_lineChange.IsInternalChange() )
		EnableApplyButton();
}

void CMainDialog::OnSelectAll( void )
{
	m_inputEdit.SetSel( 0, -1 );
}

void CMainDialog::OnCurrLineApply( void )
{
	static const std::tstring sharp = _T("#");
	std::tstring lineText;
	CTime time;

	if ( GDT_VALID == m_currPicker.GetTime( time ) )
		switch ( m_currTypeCombo.GetEnum< CDateTimeInfo::Type >() )
		{
			case CDateTimeInfo::Null: break;
			case CDateTimeInfo::Time: lineText = num::FormatNumber( time.GetTime() ); break;
			case CDateTimeInfo::OleTime: lineText = num::FormatNumber( time_utl::ToOleTime( time ).m_dt ); break;
			case CDateTimeInfo::Duration: lineText = sharp + num::FormatNumber( ( time - m_midnight ).GetTimeSpan() ); break;
			case CDateTimeInfo::OleDuration: lineText = sharp + num::FormatNumber( time_utl::ToOleTimeSpan( time - m_midnight ).m_span ); break;
		}

	m_inputEdit.SetSelRange( m_inputEdit.GetLineRangeAt() );
	m_inputEdit.ReplaceSel( lineText.c_str(), TRUE );
	m_inputEdit.SetSelRange( m_inputEdit.GetLineRangeAt() );

	UpdateData( DialogSaveChanges );
	GotoDlgCtrl( &m_inputEdit );
}

void CMainDialog::OnClearAll( void )
{
	ui::SetWindowText( m_inputEdit, std::tstring() );
}
