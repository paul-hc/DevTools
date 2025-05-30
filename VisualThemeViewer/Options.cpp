
#include "pch.h"
#include "Options.h"
#include "utl/UI/Color.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/VisualTheme.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("Settings\\ThemeSample");
	static const TCHAR entry_enableThemes[] = _T("EnableThemes");
	static const TCHAR entry_enableThemesFallback[] = _T("EnableThemesFallback");
}


COptions::COptions( void )
	: CRegistryOptions( reg::section, SaveOnModify )
	, m_pCallback( NULL )
	, m_useBorder( false )
	, m_preBkGuides( false )
	, m_postBkGuides( false )
	, m_previewThemeGlyphs( true )
	, m_useExplorerTheme( true )
	, m_enableThemes( *CVisualTheme::GetEnabledPtr() )						// acts directly on target bool (by reference)
	, m_enableThemesFallback( *CVisualTheme::GetFallbackEnabledPtr() )		// acts directly by target bool (by reference)
{
	AddOption( MAKE_OPTION( &m_bkColorText ) );
	AddOption( MAKE_OPTION( &m_useBorder ), IDC_USE_BORDER_CHECK );
	AddOption( MAKE_OPTION( &m_preBkGuides ), IDC_PRE_BK_GUIDES_CHECK );
	AddOption( MAKE_OPTION( &m_postBkGuides ), IDC_POST_BK_GUIDES_CHECK );
	AddOption( MAKE_OPTION( &m_previewThemeGlyphs ), ID_PREVIEW_THEME_GLYPHS_CHECK );
	AddOption( MAKE_OPTION( &m_useExplorerTheme ), ID_USE_EXPLORER_THEME_CHECK );
	AddOption( MAKE_OPTION( &m_enableThemes ), IDC_ENABLE_THEMES_CHECK );
	AddOption( MAKE_OPTION( &m_enableThemesFallback ), IDC_ENABLE_THEMES_FALLBACK_CHECK );

	LoadAll();
}

COptions::~COptions()
{
	SaveAll();
}

COLORREF COptions::GetBkColor( void ) const
{
	if ( m_bkColorText.empty() )
		return ::GetSysColor( COLOR_BTNFACE );

	enum { Salmon = RGB( 255, 145, 164 ) };
	COLORREF bkColor;
	return ui::ParseColor( &bkColor, m_bkColorText.c_str() ) ? bkColor : Salmon;
}

CHistoryComboBox* COptions::GetBkColorCombo( void ) const
{
	return static_cast< CHistoryComboBox* >( m_pCallback->GetWnd()->GetDlgItem( IDC_BK_COLOR_COMBO ) );
}

void COptions::OnOptionChanged( const void* pDataMember )
{
	__super::OnOptionChanged( pDataMember );

	if ( pDataMember == &m_enableThemes || pDataMember == &m_enableThemesFallback )
		m_pCallback->GetWnd()->Invalidate();		// redraw the resize gripper
	else if ( pDataMember == &m_previewThemeGlyphs )
		m_pCallback->UpdateGlyphPreview();
	else if ( pDataMember == &m_useExplorerTheme )
		m_pCallback->UpdateExplorerTheme();

	m_pCallback->RedrawSamples();
}


BEGIN_MESSAGE_MAP( COptions, CRegistryOptions )
	ON_CBN_EDITCHANGE( IDC_BK_COLOR_COMBO, OnChange_BkColor )
	ON_CBN_SELCHANGE( IDC_BK_COLOR_COMBO, OnChange_BkColor )
END_MESSAGE_MAP()

void COptions::OnChange_BkColor( void )
{
	CHistoryComboBox* pCombo = GetBkColorCombo();
	std::tstring bkColorText = ui::GetComboSelText( *pCombo );

	COLORREF bkColor;
	if ( bkColorText.empty() || ui::ParseColor( &bkColor, bkColorText.c_str() ) )
	{
		m_bkColorText = bkColorText;
		m_pCallback->RedrawSamples();
		pCombo->SetFrameColor( CLR_NONE );
	}
	else
		pCombo->SetFrameColor( RGB( 255, 0, 0 ) );
}
