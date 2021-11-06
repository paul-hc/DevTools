// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "TaskDialog.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


HRESULT CALLBACK TaskDialogCallback( HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	CTaskDialog *pTaskDialog = reinterpret_cast<CTaskDialog*>( dwRefData );
	ASSERT_PTR( pTaskDialog );

	HRESULT hRes = S_OK;

	switch ( uNotification )
	{
		case TDN_BUTTON_CLICKED:
			// wParam = Button ID
			pTaskDialog->m_buttonId = static_cast<int>(wParam);
			hRes = pTaskDialog->OnButtonClick(static_cast<int>(wParam));
			break;

		case TDN_HYPERLINK_CLICKED:
			// lParam = (LPCWSTR)pszHREF
			hRes = pTaskDialog->OnHyperlinkClick( reinterpret_cast<LPCWSTR>( lParam ) );
			break;

		case TDN_TIMER:
			// wParam = Milliseconds since dialog created or timer reset
			hRes = pTaskDialog->OnTimer(static_cast<long>(wParam));
			break;

		case TDN_DESTROYED:
			hRes = pTaskDialog->OnDestroy();
			pTaskDialog->m_hWnd = 0; //disable runtime
			break;

		case TDN_NAVIGATED:
			hRes = pTaskDialog->OnNavigatePage();
			break;

		case TDN_RADIO_BUTTON_CLICKED:
			// wParam = Radio Button ID
			pTaskDialog->m_radioId = static_cast<int>(wParam);
			hRes = pTaskDialog->OnRadioButtonClick(static_cast<int>(wParam));
			break;

		case TDN_CREATED:
			// Sending TDM_CLICK_BUTTON and TDM_CLICK_RADIO_BUTTON do under OnCreated method.
			hRes = pTaskDialog->OnCreate();
			break;

		case TDN_DIALOG_CONSTRUCTED:
			pTaskDialog->m_hWnd = hWnd;

			if ( HasFlag( pTaskDialog->m_optionFlags, TDF_SHOW_PROGRESS_BAR ) )
			{
				if ( HasFlag( pTaskDialog->m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR ) )
				{
					SendMessage( hWnd, TDM_SET_PROGRESS_BAR_MARQUEE,
						static_cast<WPARAM>( pTaskDialog->m_progressState ), static_cast<LPARAM>( pTaskDialog->m_progressPos ) );
				}
				else
				{
					SendMessage( hWnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM( pTaskDialog->m_progressRange.m_start, pTaskDialog->m_progressRange.m_end ) );
					SendMessage( hWnd, TDM_SET_PROGRESS_BAR_POS, static_cast<WPARAM>( pTaskDialog->m_progressPos ), 0 );
					SendMessage( hWnd, TDM_SET_PROGRESS_BAR_STATE, static_cast<WPARAM>( pTaskDialog->m_progressState ), 0 );
				}
			}

			if ( !pTaskDialog->m_radioButtons.empty() )
			{
				for ( size_t i = 0; i != pTaskDialog->m_radioButtons.size(); ++i )
					if ( !HasFlag( pTaskDialog->m_radioButtons[ i ].m_state, CTaskDialog::BUTTON_ENABLED ) )
						SendMessage( hWnd, TDM_ENABLE_RADIO_BUTTON,
							static_cast<WPARAM>( pTaskDialog->m_radioButtons[ i ].m_buttonId ), static_cast<LPARAM>( FALSE ) );
			}

			if ( !pTaskDialog->m_buttons.empty() )
			{
				for ( size_t i = 0; i != pTaskDialog->m_buttons.size(); ++i )
				{
					if ( !HasFlag( pTaskDialog->m_buttons[i].m_state, CTaskDialog::BUTTON_ENABLED ) )
						SendMessage( hWnd, TDM_ENABLE_BUTTON,
							static_cast<WPARAM>( pTaskDialog->m_buttons[ i ].m_buttonId ), static_cast<LPARAM>( FALSE ) );

					if ( HasFlag( pTaskDialog->m_buttons[ i ].m_state, CTaskDialog::BUTTON_ELEVATION ) )
						SendMessage( hWnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,
							static_cast<WPARAM>( pTaskDialog->m_buttons[ i ].m_buttonId ), static_cast<LPARAM>( TRUE ) );
				}
			}

			if ( pTaskDialog->m_buttonDisabled || pTaskDialog->m_buttonElevation )
			{
				UINT buttonFlag = TDCBF_OK_BUTTON;

				for(int i = 0; i < pTaskDialog->GetCommonButtonCount(); i++)
				{
					if (pTaskDialog->m_buttonDisabled & buttonFlag)
					{
						//Make sure that button id is defined
						ASSERT(pTaskDialog->GetCommonButtonId(buttonFlag));

						SendMessage(hWnd, TDM_ENABLE_BUTTON,
							static_cast<WPARAM>(pTaskDialog->GetCommonButtonId(buttonFlag)), static_cast<LPARAM>(FALSE));
					}

					if (pTaskDialog->m_buttonElevation & buttonFlag)
					{
						//Make sure that button id is defined
						ASSERT(pTaskDialog->GetCommonButtonId(buttonFlag));

						SendMessage(hWnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,
							static_cast<WPARAM>(pTaskDialog->GetCommonButtonId(buttonFlag)), static_cast<LPARAM>(TRUE));
					}

					buttonFlag <<= 1;
				}
			}

			hRes = pTaskDialog->OnInit();
			break;

		case TDN_VERIFICATION_CLICKED:
			// wParam = 1 if checkbox checked, 0 if not, lParam is unused and always 0
			pTaskDialog->m_verificationChecked = wParam != 0;
			hRes = pTaskDialog->OnVerificationCheckboxClick( wParam != 0 );
			break;

		case TDN_HELP:
			hRes = pTaskDialog->OnHelp();
			break;

		case TDN_EXPANDO_BUTTON_CLICKED:
			// wParam = 0 (dialog is now collapsed), wParam != 0 (dialog is now expanded)
			hRes = pTaskDialog->OnExpandButtonClick( wParam != 0 );
			break;
	}
	return hRes;
}

const std::tstring CTaskDialog::s_empty;

IMPLEMENT_DYNAMIC( CTaskDialog, CObject )

CTaskDialog::CTaskDialog( const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
						  int commonButtonFlags /*= TDCBF_YES_BUTTON | TDCBF_NO_BUTTON*/,
						  int optionFlags /*= TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS*/,
						  const std::tstring& footerText /*= s_empty*/ )
	: CObject()
	, m_hWnd( NULL )
	, m_title( title )
	, m_mainInstructionText( mainInstructionText )
	, m_contentText( contentText )
	, m_footerText( footerText )
	, m_commonButtonFlags( commonButtonFlags )
	, m_buttonDisabled( 0 )
	, m_buttonElevation( 0 )
	, m_optionFlags( optionFlags )
	, m_width( 0 )
	, m_defaultButton( 0 )
	, m_defaultRadioButton( 0 )
	, m_progressRange( 0, 100 )
	, m_progressState( PBST_NORMAL )
	, m_progressPos( m_progressRange.m_start )
	, m_verificationChecked( HasFlag( m_optionFlags, TDF_VERIFICATION_FLAG_CHECKED ) )
	, m_radioId( 0 )
	, m_buttonId( 0 )
{
	m_mainIcon.pszIcon = NULL;
	m_footerIcon.pszIcon = NULL;
}

CTaskDialog::CTaskDialog( const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
						  int firstButtonId, int lastButtonId,
						  int commonButtonFlags,
						  int optionFlags /*= TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS*/,
						  const std::tstring& footerText /*= s_empty*/ )
	: CObject()
	, m_hWnd( NULL )
	, m_title( title )
	, m_mainInstructionText( mainInstructionText )
	, m_contentText( contentText )
	, m_footerText( footerText )
	, m_commonButtonFlags( commonButtonFlags )
	, m_buttonDisabled( 0 )
	, m_buttonElevation( 0 )
	, m_optionFlags( optionFlags )
	, m_width( 0 )
	, m_defaultButton( 0 )
	, m_defaultRadioButton( 0 )
	, m_progressRange( 0, 100 )
	, m_progressState( PBST_NORMAL )
	, m_progressPos( m_progressRange.m_start )
	, m_verificationChecked( HasFlag( m_optionFlags, TDF_VERIFICATION_FLAG_CHECKED ) )
	, m_radioId( 0 )
	, m_buttonId( 0 )
{
	m_mainIcon.pszIcon = NULL;
	m_footerIcon.pszIcon = NULL;

	LoadButtons( firstButtonId, lastButtonId );
}

CTaskDialog::~CTaskDialog()
{
}

bool CTaskDialog::IsSupported( void )
{
#if _WIN32_WINNT >= 0x0600     // Windows Vista
	return true;
#else
	return false;
#endif
}

INT_PTR CTaskDialog::ShowDialog( CWnd* pParent, const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
								 int firstButtonId, int lastButtonId,
								 int commonButtonFlags /* = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON */,
								 int optionFlags /* = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS */,
								 const std::tstring& footerText /*= s_empty*/ )
{
	CTaskDialog dlg( title, mainInstructionText, contentText, firstButtonId, lastButtonId, commonButtonFlags, optionFlags, footerText );
	return dlg.DoModal( pParent );
}

void CTaskDialog::LoadButtons( int firstButtonId, int lastButtonId )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT( firstButtonId <= lastButtonId );
	ASSERT( firstButtonId >= 0 && lastButtonId >= 0 );

	m_buttons.clear();
	CString strTmp;

	for ( int i = firstButtonId; i <= lastButtonId; ++i )
		if ( strTmp.LoadString( i ) )
			AddButton( i, strTmp.GetString() );
}

void CTaskDialog::LoadRadioButtons( int firstRadioButtonId, int lastRadioButtonId )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT( firstRadioButtonId <= lastRadioButtonId );
	ASSERT( firstRadioButtonId >= 0 && lastRadioButtonId >= 0 );

	m_radioButtons.clear();
	CString strTmp;

	for ( int i = firstRadioButtonId; i <= lastRadioButtonId; ++i )
		if ( strTmp.LoadString( i ) )
			AddRadioButton( i, strTmp.GetString() );
}

void CTaskDialog::SetTitle( const std::tstring& title )
{
	m_title = title;
	if ( m_hWnd != NULL )
		ui::SetWindowText( m_hWnd, m_title );
}

void CTaskDialog::SetMainInstructionText( const std::tstring& mainInstructionText )
{
	m_mainInstructionText = mainInstructionText;
	Notify( TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>( m_mainInstructionText.c_str() ) );
}

void CTaskDialog::SetContentText( const std::tstring& contentText )
{
	m_contentText = contentText;
	Notify( TDM_SET_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>( m_contentText.c_str() ) );
}

void CTaskDialog::SetMainIcon(HICON hMainIcon)
{
	ASSERT(hMainIcon != NULL);

	// If the icon was initially set by HICON, allow only HICON setter method after the window has been created.
	ASSERT(NULL == m_hWnd || m_optionFlags & TDF_USE_HICON_MAIN);

	m_mainIcon.hIcon = hMainIcon;
	m_optionFlags |= TDF_USE_HICON_MAIN;

	Notify(TDM_UPDATE_ICON, TDIE_ICON_MAIN, reinterpret_cast<LPARAM>(m_mainIcon.hIcon));
}

void CTaskDialog::SetMainIcon(LPCWSTR lpszMainIcon)
{
	ASSERT(lpszMainIcon != NULL);

	// If the icon was initially set by LPWSTR, allow only LPWSTR setter method after the window has been created.
	ASSERT(NULL == m_hWnd || !(m_optionFlags & TDF_USE_HICON_MAIN));

	m_mainIcon.pszIcon = lpszMainIcon;
	m_optionFlags &= ~TDF_USE_HICON_MAIN;

	Notify(TDM_UPDATE_ICON, TDIE_ICON_MAIN, reinterpret_cast<LPARAM>(m_mainIcon.pszIcon));
}

void CTaskDialog::SetFooterIcon(HICON hFooterIcon)
{
	ASSERT(hFooterIcon != NULL);

	// If the icon was initially set by HICON, allow only HICON setter method after the window has been created.
	ASSERT(NULL == m_hWnd || m_optionFlags & TDF_USE_HICON_FOOTER);

	m_footerIcon.hIcon = hFooterIcon;
	m_optionFlags |= TDF_USE_HICON_FOOTER;

	Notify(TDM_UPDATE_ICON, TDIE_ICON_FOOTER, reinterpret_cast<LPARAM>(m_footerIcon.hIcon));
}

void CTaskDialog::SetFooterIcon( LPCWSTR lpszFooterIcon )
{
	ASSERT(lpszFooterIcon != NULL);

	// If the icon was initially set by LPWSTR, allow only LPWSTR setter method after the window has been created.
	ASSERT(NULL == m_hWnd || !(m_optionFlags & TDF_USE_HICON_FOOTER));

	m_footerIcon.pszIcon = lpszFooterIcon;
	m_optionFlags &= ~TDF_USE_HICON_FOOTER;

	Notify(TDM_UPDATE_ICON, TDIE_ICON_FOOTER, reinterpret_cast<LPARAM>(m_footerIcon.pszIcon));
}

void CTaskDialog::SetFooterText( const std::tstring& footerText )
{
	m_footerText = footerText;
	Notify( TDM_SET_ELEMENT_TEXT, TDE_FOOTER, reinterpret_cast<LPARAM>( m_footerText.c_str() ) );
}

void CTaskDialog::SetExpansionArea( const std::tstring& expandedInfoText, const std::tstring& collapsedLabel /*= s_empty*/, const std::tstring& expandedLabel /*= s_empty*/ )
{
	m_expandedInfoText = expandedInfoText;
	m_collapsedLabel = collapsedLabel;
	m_expandedLabel = expandedLabel;

	Notify( TDM_SET_ELEMENT_TEXT, TDE_EXPANDED_INFORMATION, reinterpret_cast<LPARAM>( m_expandedInfoText.c_str() ) );
}

void CTaskDialog::SetCommonButtons( int buttonMask, int disabledButtonMask /* = 0 */, int elevationButtonMask /* = 0 */ )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation

	m_commonButtonFlags = buttonMask;

	// Verify disabled command controls
	ASSERT( 0 == disabledButtonMask || HasFlag( buttonMask, disabledButtonMask ) );

	m_buttonDisabled = disabledButtonMask;

	// Verify definiton of elevation
	ASSERT( 0 == elevationButtonMask || HasFlag( buttonMask, elevationButtonMask ) );

	m_buttonElevation = elevationButtonMask;
}

void CTaskDialog::SetCommonButtonOptions( int disabledButtonMask, int elevationButtonMask /* = 0 */ )
{
	UINT buttonFlag = TDCBF_OK_BUTTON;

	for ( int i = 0; i < GetCommonButtonCount(); ++i )
	{
		if ( HasFlag( buttonFlag, m_commonButtonFlags ) )
		{
			int buttonId = GetCommonButtonId( buttonFlag );
			INT_PTR buttonIndex = FindButtonIndex( m_buttons, buttonId );

			bool enabled = !HasFlag( disabledButtonMask, buttonFlag );
			bool requiresElevation = HasFlag( elevationButtonMask, buttonFlag );

			if ( buttonIndex != -1 )
			{
				SetFlag( m_buttons[ buttonIndex ].m_state, BUTTON_ENABLED, enabled );
				SetFlag( m_buttons[ buttonIndex ].m_state, BUTTON_ELEVATION, requiresElevation );
			}

			Notify( TDM_ENABLE_BUTTON, static_cast<WPARAM>( buttonId ), static_cast<LPARAM>( enabled ) );
			Notify( TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, static_cast<WPARAM>( buttonId ), static_cast<LPARAM>( requiresElevation ) );
		}
		else
		{
			ASSERT( 0 == disabledButtonMask || !HasFlag( disabledButtonMask, buttonFlag ) );		// don't disable buttons which are not defined
			ASSERT( 0 == elevationButtonMask || !HasFlag( elevationButtonMask, buttonFlag ) );		// don't elevate buttons which are not defined
		}

		buttonFlag <<= 1;
	}

	m_buttonDisabled = disabledButtonMask;
	m_buttonElevation = elevationButtonMask;
}

void CTaskDialog::AddButton( int commandControlId, const std::tstring& caption, bool enabled /*= true*/, bool requiresElevation /*= false*/ )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT( commandControlId > 0 );
	ASSERT( !caption.empty() );

	unsigned char state = 0;
	SetFlag( state, BUTTON_ENABLED, enabled );
	SetFlag( state, BUTTON_ELEVATION, requiresElevation );

	m_buttons.push_back( CButtonInfo( commandControlId, caption, state ) );
}

void CTaskDialog::AddRadioButton( int radioButtonId, const std::tstring& caption, bool enabled /*= true*/ )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT(radioButtonId > 0);
	ASSERT( !caption.empty() );

	m_radioButtons.push_back( CButtonInfo( radioButtonId, caption, enabled ? BUTTON_ENABLED : 0 ) );
}

void CTaskDialog::SetProgressBarRange( int rangeMin, int rangeMax )
{
	ASSERT( NULL == m_hWnd || ( !HasFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR ) && HasFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR ) ) );	// before dialog creation
	ASSERT( rangeMin < rangeMax );

	m_progressRange.m_start = rangeMin;
	m_progressRange.m_end = rangeMax;

	m_optionFlags &= ~TDF_SHOW_MARQUEE_PROGRESS_BAR;
	m_optionFlags |= TDF_SHOW_PROGRESS_BAR;

	Notify( TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM( m_progressRange.m_start, m_progressRange.m_end ) );
}

void CTaskDialog::SetProgressBarPosition( int progressPos )
{
	ASSERT( NULL == m_hWnd || ( !HasFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR ) && HasFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR ) ) );	// before dialog creation
	ASSERT( m_progressRange.m_start <= progressPos && progressPos <= m_progressRange.m_end );

	m_progressPos = progressPos;
	SetFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR );
	ClearFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR );

	Notify( TDM_SET_PROGRESS_BAR_POS, static_cast<WPARAM>( m_progressPos ), 0 );
}

void CTaskDialog::SetProgressBarState( int state /*= PBST_NORMAL*/ )
{
	ASSERT( NULL == m_hWnd || ( !HasFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR ) && HasFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR ) ) );	// before dialog creation

	m_progressState = state;
	SetFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR );
	ClearFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR );

	Notify( TDM_SET_PROGRESS_BAR_STATE, static_cast<WPARAM>( m_progressState ), 0 );
}

void CTaskDialog::SetProgressBarMarquee( bool enabled /*= true*/, int marqueeSpeed /*= 0*/ )
{
	// Marquee cannot be defined after the window has been created.
	ASSERT( NULL == m_hWnd || ( HasFlag( m_optionFlags, TDF_SHOW_MARQUEE_PROGRESS_BAR ) && HasFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR ) ) );
	ASSERT( marqueeSpeed >= 0 );

	m_progressState = enabled;
	m_progressPos = marqueeSpeed;

	SetFlag( m_optionFlags, TDF_SHOW_PROGRESS_BAR | TDF_SHOW_MARQUEE_PROGRESS_BAR );

	Notify( TDM_SET_PROGRESS_BAR_MARQUEE, static_cast<WPARAM>( m_progressState ), static_cast<LPARAM>( m_progressPos ) );
}

void CTaskDialog::SetOptions( int optionFlags )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation

	m_optionFlags = optionFlags;
	m_verificationChecked = HasFlag( m_optionFlags, TDF_VERIFICATION_FLAG_CHECKED );
}

void CTaskDialog::SetButtonOptions( int commandControlId, bool enabled, bool requiresElevation /*= false*/ )
{
	INT_PTR index = FindButtonIndex( m_buttons, commandControlId );
	ASSERT( index != -1 );

	if ( index != -1 )
	{
		SetFlag( m_buttons[index].m_state, BUTTON_ENABLED, enabled );
		SetFlag( m_buttons[index].m_state, BUTTON_ELEVATION, requiresElevation );
	}

	int buttonFlag = GetCommonButtonFlag( commandControlId );

	if ( buttonFlag != 0 )
	{
		SetFlag( m_buttonDisabled, buttonFlag, !enabled );
		SetFlag( m_buttonElevation, BUTTON_ELEVATION, requiresElevation );
	}

	Notify( TDM_ENABLE_BUTTON, static_cast<WPARAM>( commandControlId ), static_cast<LPARAM>( enabled ) );
	Notify( TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, static_cast<WPARAM>( commandControlId ), static_cast<LPARAM>( requiresElevation ) );
}

bool CTaskDialog::IsButtonEnabled( int commandControlId ) const
{
	INT_PTR index = FindButtonIndex( m_buttons, commandControlId );
	int buttonFlag = GetCommonButtonFlag( commandControlId );

	ASSERT( index != -1 || buttonFlag & m_commonButtonFlags );

	if ( index != -1 )
		return HasFlag( m_buttons[ index ].m_state, BUTTON_ENABLED );
	else
		return !HasFlag( m_buttonDisabled, buttonFlag );
}

void CTaskDialog::ClickButton( int commandControlId ) const
{
	ASSERT( FindButtonIndex( m_buttons, commandControlId ) != -1 || HasFlag( GetCommonButtonFlag( commandControlId ), m_commonButtonFlags ) );

	Notify( TDM_CLICK_BUTTON, static_cast<WPARAM>( commandControlId ), 0 );
}

void CTaskDialog::SetDefaultButton( int commandControlId )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT( FindButtonIndex( m_buttons, commandControlId ) != -1 || HasFlag( GetCommonButtonFlag( commandControlId ), m_commonButtonFlags ) );

	m_defaultButton = commandControlId;
}

void CTaskDialog::SetRadioButtonOptions( int radioButtonId, bool enabled )
{
	INT_PTR index = FindButtonIndex( m_radioButtons, radioButtonId );
	ASSERT( index != -1 );

	SetFlag( m_radioButtons[ index ].m_state, BUTTON_ENABLED, enabled );

	Notify( TDM_ENABLE_RADIO_BUTTON, static_cast<WPARAM>( radioButtonId ), static_cast<LPARAM>( enabled ) );
}

void CTaskDialog::ClickRadioButton( int radioButtonId ) const
{
	ASSERT( FindButtonIndex( m_radioButtons, radioButtonId ) != -1 );

	Notify( TDM_CLICK_RADIO_BUTTON, static_cast<WPARAM>( radioButtonId ), 0 );
}

bool CTaskDialog::IsRadioButtonEnabled( int radioButtonId ) const
{
	INT_PTR index = FindButtonIndex( m_radioButtons, radioButtonId );
	ASSERT( index != -1 );

	return HasFlag( m_radioButtons[ index ].m_state, BUTTON_ENABLED );
}

void CTaskDialog::SetDefaultRadioButton( int radioButtonId )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation
	ASSERT( FindButtonIndex( m_radioButtons, radioButtonId ) != -1 );

	m_defaultRadioButton = radioButtonId;
}

void CTaskDialog::RemoveAllRadioButtons( void )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation

	m_radioButtons.clear();
}

void CTaskDialog::SetVerificationText( const std::tstring& verificationText )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation

	m_verificationText = verificationText;
}

void CTaskDialog::SetVerificationChecked( bool verificationChecked /*= true*/ )
{
	ASSERT( !m_verificationText.empty() );

	m_verificationChecked = verificationChecked;

	SetFlag( m_optionFlags, TDF_VERIFICATION_FLAG_CHECKED, verificationChecked );
	Notify( TDM_CLICK_VERIFICATION, static_cast<WPARAM>( m_verificationChecked ), 0 );
}


void CTaskDialog::NavigateTo( CTaskDialog& rTaskDialog ) const
{
	TASKDIALOGCONFIG config = { 0 };

	rTaskDialog.FillStruct( config );
	Notify( TDM_NAVIGATE_PAGE, 0, reinterpret_cast<LPARAM>( &config ) );
	rTaskDialog.FreeStruct( config );
}

void CTaskDialog::FillStruct( TASKDIALOGCONFIG& rConfig )
{
	rConfig.dwCommonButtons = m_commonButtonFlags;
	rConfig.dwFlags = m_optionFlags;

	if ( HasFlag( rConfig.dwFlags, TDF_USE_COMMAND_LINKS | TDF_USE_COMMAND_LINKS_NO_ICON ) && m_buttons.empty() )
		ClearFlag( rConfig.dwFlags, TDF_USE_COMMAND_LINKS | TDF_USE_COMMAND_LINKS_NO_ICON );

	// ensure that there is footer field and we can set the footer icon.
	if ( m_footerText.empty() && m_footerIcon.pszIcon != NULL )
		m_footerText = _T(" ");

	rConfig.cxWidth = m_width;

	rConfig.nDefaultButton = m_defaultButton;
	rConfig.nDefaultRadioButton = m_defaultRadioButton;

	rConfig.pszWindowTitle = m_title.c_str();
	rConfig.pszMainInstruction = m_mainInstructionText.c_str();
	rConfig.pszContent = m_contentText.c_str();
	rConfig.pszFooter = m_footerText.c_str();

	rConfig.pszExpandedInformation = m_expandedInfoText.c_str();
	rConfig.pszExpandedControlText = m_expandedLabel.c_str();
	rConfig.pszCollapsedControlText = m_collapsedLabel.c_str();

	if (m_optionFlags & TDF_USE_HICON_MAIN)
	{
		rConfig.hMainIcon = m_mainIcon.hIcon;
	}
	else
	{
		rConfig.pszMainIcon = m_mainIcon.pszIcon;
	}

	if (m_optionFlags & TDF_USE_HICON_FOOTER)
	{
		rConfig.hFooterIcon = m_footerIcon.hIcon;
	}
	else
	{
		rConfig.pszFooterIcon = m_footerIcon.pszIcon;
	}

	if ( !m_verificationText.empty() )
		rConfig.pszVerificationText = m_verificationText.c_str();
	else
	{
		rConfig.dwFlags &= ~TDF_VERIFICATION_FLAG_CHECKED;
		rConfig.pszVerificationText = NULL;
	}

	if ( !m_radioButtons.empty() )
	{
		rConfig.pRadioButtons = MakeButtonData(m_radioButtons);
		rConfig.cRadioButtons = static_cast<UINT>( m_radioButtons.size() );
	}

	if ( !m_buttons.empty() )
	{
		rConfig.pButtons = MakeButtonData(m_buttons);
		rConfig.cButtons = static_cast<UINT>( m_buttons.size() );
	}

	rConfig.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
	rConfig.pfCallback = TaskDialogCallback;
	rConfig.cbSize =  sizeof(TASKDIALOGCONFIG);
}

void CTaskDialog::FreeStruct( TASKDIALOGCONFIG& rConfig )
{
	if ( rConfig.pButtons != NULL )
	{
		delete[] rConfig.pButtons;
		rConfig.pButtons = NULL;
	}

	if ( rConfig.pRadioButtons != NULL )
	{
		delete [] rConfig.pRadioButtons;
		rConfig.pRadioButtons = NULL;
	}
}


int CTaskDialog::GetCommonButtonId( int flag ) const
{
	switch ( flag )
	{
		case TDCBF_OK_BUTTON:
			return IDOK;
		case TDCBF_YES_BUTTON:
			return IDYES;
		case TDCBF_NO_BUTTON:
			return IDNO;
		case TDCBF_CANCEL_BUTTON:
			return IDCANCEL;
		case TDCBF_RETRY_BUTTON:
			return IDRETRY;
		case TDCBF_CLOSE_BUTTON:
			return IDCLOSE;
		default:
			return 0;
	}
}

int CTaskDialog::GetCommonButtonFlag( int buttonId ) const
{
	switch ( buttonId )
	{
		case IDOK:
			return TDCBF_OK_BUTTON;
		case IDYES:
			return TDCBF_YES_BUTTON;
		case IDNO:
			return TDCBF_NO_BUTTON;
		case IDCANCEL:
			return TDCBF_CANCEL_BUTTON;
		case IDRETRY:
			return TDCBF_RETRY_BUTTON;
		case IDCLOSE:
			return TDCBF_CLOSE_BUTTON;
		default:
			return 0;
	}
}

int CTaskDialog::GetCommonButtonCount( void ) const
{
	return 6;		// it's the amount of common buttons in commctrl.h
}


TASKDIALOG_BUTTON* CTaskDialog::MakeButtonData( const std::vector< CButtonInfo >& buttons ) const
{
	TASKDIALOG_BUTTON* pResult = new TASKDIALOG_BUTTON[ buttons.size() ];

	for ( size_t i = 0; i != buttons.size(); ++i )
	{
		pResult[ i ].nButtonID = buttons[ i ].m_buttonId;
		pResult[ i ].pszButtonText = buttons[ i ].m_caption.c_str();
	}

	return pResult;
}

INT_PTR CTaskDialog::FindButtonIndex( const std::vector< CButtonInfo >& buttons, int buttonId )
{
	for ( INT_PTR i = 0; i != static_cast<INT_PTR>( buttons.size() ); ++i )
		if ( buttons[ i ].m_buttonId == buttonId )
			return i;

	return -1;
}

void CTaskDialog::Notify( UINT uMsg, WPARAM wParam, LPARAM lParam ) const
{
	if ( m_hWnd != NULL )
		SendMessage( m_hWnd, uMsg, wParam, lParam );
}

INT_PTR CTaskDialog::DoModal( CWnd* pParent )
{
	ASSERT_NULL( m_hWnd );		// before dialog creation

	TASKDIALOGCONFIG config = { 0 };
	config.hwndParent = pParent->GetSafeHwnd();

	FillStruct( config );
	HRESULT hResult = TaskDialogIndirect( &config, &m_buttonId, &m_radioId, &m_verificationChecked );
	FreeStruct( config );

	if ( S_OK == hResult )
		return static_cast<INT_PTR>( m_buttonId );
	else
		return -1;
}

HRESULT CTaskDialog::OnInit( void )
{
	return S_OK;
}

HRESULT CTaskDialog::OnDestroy( void )
{
	return S_OK;
}

HRESULT CTaskDialog::OnButtonClick( int buttonId )
{
	buttonId;
	return S_OK;
}

HRESULT CTaskDialog::OnRadioButtonClick( int radioId )
{
	radioId;
	return S_OK;
}

HRESULT CTaskDialog::OnVerificationCheckboxClick( bool verificationChecked )
{
	verificationChecked;
	return S_OK;
}

HRESULT CTaskDialog::OnExpandButtonClick( bool expanded )
{
	expanded;
	return S_OK;
}

HRESULT CTaskDialog::OnHyperlinkClick( const std::tstring& href )
{
	ShellExecute( m_hWnd, NULL, href.c_str(), NULL, NULL, SW_SHOW );
	return S_OK;
}

HRESULT CTaskDialog::OnHelp( void )
{
	return S_FALSE;
}

HRESULT CTaskDialog::OnTimer( long lTime )
{
	lTime;
	return S_OK;
}

HRESULT CTaskDialog::OnNavigatePage( void )
{
	return S_OK;
}

HRESULT CTaskDialog::OnCreate( void )
{
	return S_OK;
}
