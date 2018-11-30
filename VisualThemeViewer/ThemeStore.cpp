
#include "stdafx.h"
#include "ThemeStore.h"
#include "utl/ScopedValue.h"
#include "utl/VisualTheme.h"
#include <vsstyle.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ID_TAG( Id ) Id, L#Id


// missing part & state definitions

enum GLOBALS_PARTS { GP_BORDER = 1, GP_LINEHORZ, GP_LINEVERT };

enum BORDER_STATES { BSS_FLAT = 1, BSS_RAISED, BSS_SUNKEN };
enum LINEHORZ_STATES { LHS_FLAT = 1, LHS_RAISED, LHS_SUNKEN };
enum LINEVERT_STATES { LVS_FLAT = 1, LVS_RAISED, LVS_SUNKEN };

enum SEARCHEDITBOX_PARTS { SEBP_SEARCHEDITBOXTEXT = 1 };
enum SEARCHEDITBOXTEXT_STATES { SEBTS_FORMATTED = 1 };


const CEnumTags& GetTags_Relevance( void )
{
	static const CEnumTags s_tags( _T("High|Medium|Obscure") );
	return s_tags;
}


// CThemeState class

CThemeItem CThemeState::MakeThemeItem( void ) const
{
	const CThemePart* pPart = checked_static_cast< const CThemePart* >( GetParentNode() );
	const CThemeClass* pClass = checked_static_cast< const CThemeClass* >( pPart->GetParentNode() );

	return CThemeItem( pClass->m_className.c_str(), pPart->m_partId, m_stateId );
}


// CThemePart class

CThemePart* CThemePart::AddState( int stateId, const std::wstring& stateName, Relevance relevance /*= HighRelevance*/ )
{
	CThemeState* pNewState = new CThemeState( stateId, stateName, relevance );
	pNewState->SetParentNode( this );
	m_states.push_back( pNewState );
	return this;
}

CThemeItem CThemePart::MakeThemeItem( void ) const
{
	const CThemeClass* pClass = checked_static_cast< const CThemeClass* >( GetParentNode() );

	return CThemeItem( pClass->m_className.c_str(), m_partId, !m_states.empty() ? m_states.front()->m_stateId : 0 );
}

bool CThemePart::SetupNotImplemented( CVisualTheme& rTheme, HDC hDC )
{
	static const CRect rect( 0, 0, 32, 32 );

	int implCount = 0;

	if ( m_states.empty() )
	{
		if ( rTheme.DrawThemeBackground( hDC, m_partId, 0, rect ) )
			++implCount;
	}
	else
		for ( auto itState = m_states.begin(); itState != m_states.end(); ++itState )
			if ( rTheme.DrawThemeBackground( hDC, m_partId, ( *itState )->m_stateId, rect ) )
				++implCount;
			else
				( *itState )->SetRelevance( NotImplemented );

	return implCount != 0;
}


// CThemeClass class

CThemePart* CThemeClass::AddPart( int partId, const std::wstring& partName, Relevance relevance /*= HighRelevance*/ )
{
	CThemePart* pPart = new CThemePart( partId, partName, relevance );
	pPart->SetParentNode( this );
	m_parts.push_back( pPart );
	return pPart;
}

CThemeItem CThemeClass::MakeThemeItem( void ) const
{
	const CThemePart* pFirstPart = !m_parts.empty() ? m_parts.front() : NULL;
	const CThemeState* pFirstState = pFirstPart != NULL && !pFirstPart->m_states.empty() ? pFirstPart->m_states.front() : NULL;

	return CThemeItem( m_className.c_str(), pFirstPart != NULL ? pFirstPart->m_partId : 1, pFirstState != NULL ? pFirstState->m_stateId : 0 );
}

bool CThemeClass::SetupNotImplemented( CVisualTheme& rTheme, HDC hDC )
{
	int implCount = 0;
	for ( std::vector< CThemePart* >::const_iterator itPart = m_parts.begin(); itPart != m_parts.end(); ++itPart )
		if ( ( *itPart )->SetupNotImplemented( rTheme, hDC ) )
			++implCount;
		else
			( *itPart )->SetRelevance( NotImplemented );

	return implCount != 0;
}


// CThemeStore class

CThemeClass* CThemeStore::FindClass( const wchar_t* pClassName ) const
{
	for ( std::vector< CThemeClass* >::const_iterator itClass = m_classes.begin(); itClass != m_classes.end(); ++itClass )
		if ( pred::Equal == _wcsicmp( ( *itClass )->m_className.c_str(), pClassName ) )
			return *itClass;

	return NULL;
}

bool CThemeStore::SetupNotImplementedThemes( void )
{
    CDC memoryDC;
	memoryDC.CreateCompatibleDC( NULL );

	CScopedValue< bool > scopedThemesEnabled( CVisualTheme::GetEnabledPtr(), true );
	CScopedValue< bool > scopedThemeFallbackEnabled( CVisualTheme::GetFallbackEnabledPtr(), false );
	int implCount = 0;

	for ( std::vector< CThemeClass* >::const_iterator itClass = m_classes.begin(); itClass != m_classes.end(); ++itClass )
	{
		CVisualTheme theme( ( *itClass )->m_className.c_str() );
		if ( theme.IsValid() && ( *itClass )->SetupNotImplemented( theme, memoryDC ) )
			++implCount;
		else
			( *itClass )->SetRelevance( NotImplemented );
	}
	return implCount != 0;
}

CThemeClass* CThemeStore::AddClass( const std::wstring& className, Relevance relevance /*= HighRelevance*/ )
{
	CThemeClass* pClass = new CThemeClass( className, relevance );
	m_classes.push_back( pClass );
	return pClass;
}

void CThemeStore::RegisterStandardClasses( void )
{
	{
		CThemeClass* pClass = AddClass( L"BUTTON" );

		pClass->AddPart( ID_TAG( BP_PUSHBUTTON ) )
			->AddState( ID_TAG( PBS_NORMAL ) )
			->AddState( ID_TAG( PBS_HOT ) )
			->AddState( ID_TAG( PBS_PRESSED ) )
			->AddState( ID_TAG( PBS_DISABLED ) )
			->AddState( ID_TAG( PBS_DEFAULTED ) )
			->AddState( ID_TAG( PBS_DEFAULTED_ANIMATING ) )
		;
		pClass->AddPart( ID_TAG( BP_RADIOBUTTON ) )
			->AddState( ID_TAG( RBS_UNCHECKEDNORMAL ) )
			->AddState( ID_TAG( RBS_UNCHECKEDHOT ) )
			->AddState( ID_TAG( RBS_UNCHECKEDPRESSED ) )
			->AddState( ID_TAG( RBS_UNCHECKEDDISABLED ) )
			->AddState( ID_TAG( RBS_CHECKEDNORMAL ) )
			->AddState( ID_TAG( RBS_CHECKEDHOT ) )
			->AddState( ID_TAG( RBS_CHECKEDPRESSED ) )
			->AddState( ID_TAG( RBS_CHECKEDDISABLED ) )
		;
		pClass->AddPart( ID_TAG( BP_CHECKBOX ) )
			->AddState( ID_TAG( CBS_UNCHECKEDNORMAL ) )
			->AddState( ID_TAG( CBS_UNCHECKEDHOT ) )
			->AddState( ID_TAG( CBS_UNCHECKEDPRESSED ) )
			->AddState( ID_TAG( CBS_UNCHECKEDDISABLED ) )
			->AddState( ID_TAG( CBS_CHECKEDNORMAL ) )
			->AddState( ID_TAG( CBS_CHECKEDHOT ) )
			->AddState( ID_TAG( CBS_CHECKEDPRESSED ) )
			->AddState( ID_TAG( CBS_CHECKEDDISABLED ) )
			->AddState( ID_TAG( CBS_MIXEDNORMAL ) )
			->AddState( ID_TAG( CBS_MIXEDHOT ) )
			->AddState( ID_TAG( CBS_MIXEDPRESSED ) )
			->AddState( ID_TAG( CBS_MIXEDDISABLED ) )
			->AddState( ID_TAG( CBS_IMPLICITNORMAL ) )
			->AddState( ID_TAG( CBS_IMPLICITHOT ) )
			->AddState( ID_TAG( CBS_IMPLICITPRESSED ) )
			->AddState( ID_TAG( CBS_IMPLICITDISABLED ) )
			->AddState( ID_TAG( CBS_EXCLUDEDNORMAL ) )
			->AddState( ID_TAG( CBS_EXCLUDEDHOT ) )
			->AddState( ID_TAG( CBS_EXCLUDEDPRESSED ) )
			->AddState( ID_TAG( CBS_EXCLUDEDDISABLED ) )
		;
		pClass->AddPart( ID_TAG( BP_GROUPBOX ) )
			->AddState( ID_TAG( GBS_NORMAL ) )
			->AddState( ID_TAG( GBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( BP_USERBUTTON ) );
		pClass->AddPart( ID_TAG( BP_COMMANDLINK ) )
			->AddState( ID_TAG( CMDLS_NORMAL ) )
			->AddState( ID_TAG( CMDLS_HOT ) )
			->AddState( ID_TAG( CMDLS_PRESSED ) )
			->AddState( ID_TAG( CMDLS_DISABLED ) )
			->AddState( ID_TAG( CMDLS_DEFAULTED ) )
			->AddState( ID_TAG( CMDLS_DEFAULTED_ANIMATING ) )
		;
		pClass->AddPart( ID_TAG( BP_COMMANDLINKGLYPH ) )
			->AddState( ID_TAG( CMDLGS_NORMAL ) )
			->AddState( ID_TAG( CMDLGS_HOT ) )
			->AddState( ID_TAG( CMDLGS_PRESSED ) )
			->AddState( ID_TAG( CMDLGS_DISABLED ) )
			->AddState( ID_TAG( CMDLGS_DEFAULTED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"CLOCK" );		// ObscureRelevance
		pClass->AddPart( ID_TAG( CLP_TIME ) )
			->AddState( ID_TAG( CLS_NORMAL ) )
			->AddState( CLS_HOT, L"CLS_HOT (Windows 7)" )
			->AddState( CLS_PRESSED, L"CLS_PRESSED (Windows 7)" )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"COMBOBOX" );

		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTON ) )
			->AddState( ID_TAG( CBXS_NORMAL ) )
			->AddState( ID_TAG( CBXS_HOT ) )
			->AddState( ID_TAG( CBXS_PRESSED ) )
			->AddState( ID_TAG( CBXS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_BACKGROUND ) );
		pClass->AddPart( ID_TAG( CP_TRANSPARENTBACKGROUND ) )
			->AddState( ID_TAG( CBTBS_NORMAL ) )
			->AddState( ID_TAG( CBTBS_HOT ) )
			->AddState( ID_TAG( CBTBS_FOCUSED ) )
			->AddState( ID_TAG( CBTBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_BORDER ) )
			->AddState( ID_TAG( CBB_NORMAL ) )
			->AddState( ID_TAG( CBB_HOT ) )
			->AddState( ID_TAG( CBB_FOCUSED ) )
			->AddState( ID_TAG( CBB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_READONLY ) )
			->AddState( ID_TAG( CBRO_NORMAL ) )
			->AddState( ID_TAG( CBRO_HOT ) )
			->AddState( ID_TAG( CBRO_PRESSED ) )
			->AddState( ID_TAG( CBRO_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTONRIGHT ) )
			->AddState( ID_TAG( CBXSR_NORMAL ) )
			->AddState( ID_TAG( CBXSR_HOT ) )
			->AddState( ID_TAG( CBXSR_PRESSED ) )
			->AddState( ID_TAG( CBXSR_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTONLEFT ) )
			->AddState( ID_TAG( CBXSL_NORMAL ) )
			->AddState( ID_TAG( CBXSL_HOT ) )
			->AddState( ID_TAG( CBXSL_PRESSED ) )
			->AddState( ID_TAG( CBXSL_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_CUEBANNER ) )
			->AddState( ID_TAG( CBCB_NORMAL ) )
			->AddState( ID_TAG( CBCB_HOT ) )
			->AddState( ID_TAG( CBCB_PRESSED ) )
			->AddState( ID_TAG( CBCB_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"COMMUNICATIONS" );		// ObscureRelevance
		pClass->AddPart( ID_TAG( CSST_TAB ) )
			->AddState( ID_TAG( CSTB_NORMAL ) )
			->AddState( ID_TAG( CSTB_HOT ) )
			->AddState( ID_TAG( CSTB_SELECTED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"CONTROLPANEL", MediumRelevance );

		pClass->AddPart( ID_TAG( CPANEL_NAVIGATIONPANE ) );
		pClass->AddPart( ID_TAG( CPANEL_CONTENTPANE ) );
		pClass->AddPart( ID_TAG( CPANEL_NAVIGATIONPANELABEL ) );
		pClass->AddPart( ID_TAG( CPANEL_CONTENTPANELABEL ) );
		pClass->AddPart( ID_TAG( CPANEL_TITLE ) );
		pClass->AddPart( ID_TAG( CPANEL_BODYTEXT ) );
		pClass->AddPart( ID_TAG( CPANEL_HELPLINK ) )
			->AddState( ID_TAG( CPHL_NORMAL ) )
			->AddState( ID_TAG( CPHL_HOT ) )
			->AddState( ID_TAG( CPHL_PRESSED ) )
			->AddState( ID_TAG( CPHL_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CPANEL_TASKLINK ) )
			->AddState( ID_TAG( CPTL_NORMAL ) )
			->AddState( ID_TAG( CPTL_HOT ) )
			->AddState( ID_TAG( CPTL_PRESSED ) )
			->AddState( ID_TAG( CPTL_DISABLED ) )
			->AddState( ID_TAG( CPTL_PAGE ) )
		;
		pClass->AddPart( ID_TAG( CPANEL_GROUPTEXT ) );
		pClass->AddPart( ID_TAG( CPANEL_CONTENTLINK ) )
			->AddState( ID_TAG( CPCL_NORMAL ) )
			->AddState( ID_TAG( CPCL_HOT ) )
			->AddState( ID_TAG( CPCL_PRESSED ) )
			->AddState( ID_TAG( CPCL_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CPANEL_SECTIONTITLELINK ) )
			->AddState( ID_TAG( CPSTL_NORMAL ) )
			->AddState( ID_TAG( CPSTL_HOT ) )
		;
		pClass->AddPart( ID_TAG( CPANEL_LARGECOMMANDAREA ) );
		pClass->AddPart( ID_TAG( CPANEL_SMALLCOMMANDAREA ) );
		pClass->AddPart( ID_TAG( CPANEL_BUTTON ) );
		pClass->AddPart( ID_TAG( CPANEL_MESSAGETEXT ) );
		pClass->AddPart( ID_TAG( CPANEL_NAVIGATIONPANELINE ) );
		pClass->AddPart( ID_TAG( CPANEL_CONTENTPANELINE ) );
		pClass->AddPart( ID_TAG( CPANEL_BANNERAREA ) );
		pClass->AddPart( ID_TAG( CPANEL_BODYTITLE ) );
	}

	{
		CThemeClass* pClass = AddClass( L"DATEPICKER", MediumRelevance );

		pClass->AddPart( ID_TAG( DP_DATETEXT ) )
			->AddState( ID_TAG( DPDT_NORMAL ) )
			->AddState( ID_TAG( DPDT_SELECTED ) )
			->AddState( ID_TAG( DPDT_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( DP_DATEBORDER ) )
			->AddState( ID_TAG( DPDB_NORMAL ) )
			->AddState( ID_TAG( DPDB_HOT ) )
			->AddState( ID_TAG( DPDB_FOCUSED ) )
			->AddState( ID_TAG( DPDB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( DP_SHOWCALENDARBUTTONRIGHT ) )
			->AddState( ID_TAG( DPSCBR_NORMAL ) )
			->AddState( ID_TAG( DPSCBR_HOT ) )
			->AddState( ID_TAG( DPSCBR_PRESSED ) )
			->AddState( ID_TAG( DPSCBR_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"DRAGDROP", MediumRelevance );

		pClass->AddPart( ID_TAG( DD_COPY ) )
			->AddState( ID_TAG( DDCOPY_HIGHLIGHT ) )
			->AddState( ID_TAG( DDCOPY_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_MOVE ) )
			->AddState( ID_TAG( DDMOVE_HIGHLIGHT ) )
			->AddState( ID_TAG( DDMOVE_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_UPDATEMETADATA ) )
			->AddState( ID_TAG( DDUPDATEMETADATA_HIGHLIGHT ) )
			->AddState( ID_TAG( DDUPDATEMETADATA_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_CREATELINK ) )
			->AddState( ID_TAG( DDCREATELINK_HIGHLIGHT ) )
			->AddState( ID_TAG( DDCREATELINK_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_WARNING ) )
			->AddState( ID_TAG( DDWARNING_HIGHLIGHT ) )
			->AddState( ID_TAG( DDWARNING_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_NONE ) )
			->AddState( ID_TAG( DDNONE_HIGHLIGHT ) )
			->AddState( ID_TAG( DDNONE_NOHIGHLIGHT ) )
		;
		pClass->AddPart( ID_TAG( DD_IMAGEBG ) );
		pClass->AddPart( ID_TAG( DD_TEXTBG ) );
	}

	{
		CThemeClass* pClass = AddClass( L"EDIT" );

		pClass->AddPart( ID_TAG( EP_EDITTEXT ) )
			->AddState( ID_TAG( ETS_NORMAL ) )
			->AddState( ID_TAG( ETS_HOT ) )
			->AddState( ID_TAG( ETS_SELECTED ) )
			->AddState( ID_TAG( ETS_DISABLED ) )
			->AddState( ID_TAG( ETS_FOCUSED ) )
			->AddState( ID_TAG( ETS_READONLY ) )
			->AddState( ID_TAG( ETS_ASSIST ) )
			->AddState( ID_TAG( ETS_CUEBANNER ) )
		;
		pClass->AddPart( ID_TAG( EP_CARET ) );
		pClass->AddPart( ID_TAG( EP_BACKGROUND ) )
			->AddState( ID_TAG( EBS_NORMAL ) )
			->AddState( ID_TAG( EBS_HOT ) )
			->AddState( ID_TAG( EBS_DISABLED ) )
			->AddState( ID_TAG( EBS_FOCUSED ) )
			->AddState( ID_TAG( EBS_READONLY ) )
			->AddState( ID_TAG( EBS_ASSIST ) )
		;
		pClass->AddPart( ID_TAG( EP_PASSWORD ) );
		pClass->AddPart( ID_TAG( EP_BACKGROUNDWITHBORDER ) )
			->AddState( ID_TAG( EBWBS_NORMAL ) )
			->AddState( ID_TAG( EBWBS_HOT ) )
			->AddState( ID_TAG( EBWBS_DISABLED ) )
			->AddState( ID_TAG( EBWBS_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_NOSCROLL ) )
			->AddState( ID_TAG( EPSN_NORMAL ) )
			->AddState( ID_TAG( EPSN_HOT ) )
			->AddState( ID_TAG( EPSN_FOCUSED ) )
			->AddState( ID_TAG( EPSN_DISABLED ) )
		;

		pClass->AddPart( ID_TAG( EP_EDITBORDER_HSCROLL ) )
			->AddState( ID_TAG( EPSH_NORMAL ) )
			->AddState( ID_TAG( EPSH_HOT ) )
			->AddState( ID_TAG( EPSH_FOCUSED ) )
			->AddState( ID_TAG( EPSH_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_VSCROLL ) )
			->AddState( ID_TAG( EPSV_NORMAL ) )
			->AddState( ID_TAG( EPSV_HOT ) )
			->AddState( ID_TAG( EPSV_FOCUSED ) )
			->AddState( ID_TAG( EPSV_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_HVSCROLL ) )
			->AddState( ID_TAG( EPSHV_NORMAL ) )
			->AddState( ID_TAG( EPSHV_HOT ) )
			->AddState( ID_TAG( EPSHV_FOCUSED ) )
			->AddState( ID_TAG( EPSHV_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"EXPLORERBAR", MediumRelevance );

		pClass->AddPart( ID_TAG( EBP_HEADERBACKGROUND ) );
		pClass->AddPart( ID_TAG( EBP_HEADERCLOSE ) )
			->AddState( ID_TAG( EBHC_NORMAL ) )
			->AddState( ID_TAG( EBHC_HOT ) )
			->AddState( ID_TAG( EBHC_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_HEADERPIN ) )
			->AddState( ID_TAG( EBHP_NORMAL ) )
			->AddState( ID_TAG( EBHP_HOT ) )
			->AddState( ID_TAG( EBHP_PRESSED ) )
			->AddState( ID_TAG( EBHP_SELECTEDNORMAL ) )
			->AddState( ID_TAG( EBHP_SELECTEDHOT ) )
			->AddState( ID_TAG( EBHP_SELECTEDPRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_IEBARMENU ) )
			->AddState( ID_TAG( EBM_NORMAL ) )
			->AddState( ID_TAG( EBM_HOT ) )
			->AddState( ID_TAG( EBM_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPBACKGROUND ) );
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPCOLLAPSE ) )
			->AddState( ID_TAG( EBNGC_NORMAL ) )
			->AddState( ID_TAG( EBNGC_HOT ) )
			->AddState( ID_TAG( EBNGC_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPEXPAND ) )
			->AddState( ID_TAG( EBNGE_NORMAL ) )
			->AddState( ID_TAG( EBNGE_HOT ) )
			->AddState( ID_TAG( EBNGE_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPHEAD ) );
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPBACKGROUND ) );
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPCOLLAPSE ) )
			->AddState( ID_TAG( EBSGC_NORMAL ) )
			->AddState( ID_TAG( EBSGC_HOT ) )
			->AddState( ID_TAG( EBSGC_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPEXPAND ) )
			->AddState( ID_TAG( EBSGE_NORMAL ) )
			->AddState( ID_TAG( EBSGE_HOT ) )
			->AddState( ID_TAG( EBSGE_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPHEAD ) );
	}

	{
		CThemeClass* pClass = AddClass( L"FLYOUT", MediumRelevance );

		pClass->AddPart( ID_TAG( FLYOUT_HEADER ) );
		pClass->AddPart( ID_TAG( FLYOUT_BODY ) )
			->AddState( ID_TAG( FBS_NORMAL ) )
			->AddState( ID_TAG( FBS_EMPHASIZED ) )
		;
		pClass->AddPart( ID_TAG( FLYOUT_LABEL ) )
			->AddState( ID_TAG( FLS_NORMAL ) )
			->AddState( ID_TAG( FLS_SELECTED ) )
			->AddState( ID_TAG( FLS_EMPHASIZED ) )
			->AddState( ID_TAG( FLS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( FLYOUT_LINK ) )
			->AddState( ID_TAG( FLYOUTLINK_NORMAL ) )
			->AddState( ID_TAG( FLYOUTLINK_HOVER ) )
		;
		pClass->AddPart( ID_TAG( FLYOUT_DIVIDER ) );
		pClass->AddPart( ID_TAG( FLYOUT_WINDOW ) );
		pClass->AddPart( ID_TAG( FLYOUT_LINKAREA ) );
		pClass->AddPart( ID_TAG( FLYOUT_LINKHEADER ) )
			->AddState( ID_TAG( FLH_NORMAL ) )
			->AddState( ID_TAG( FLH_HOVER ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"GLOBALS", MediumRelevance );

		pClass->AddPart( ID_TAG( GP_BORDER ) )
			->AddState( ID_TAG( BSS_FLAT ) )
			->AddState( ID_TAG( BSS_RAISED ) )
			->AddState( ID_TAG( BSS_SUNKEN ) )
		;
		pClass->AddPart( ID_TAG( GP_LINEHORZ ) )
			->AddState( ID_TAG( LHS_FLAT ) )
			->AddState( ID_TAG( LHS_RAISED ) )
			->AddState( ID_TAG( LHS_SUNKEN ) )
		;
		pClass->AddPart( ID_TAG( GP_LINEVERT ) )
			->AddState( ID_TAG( LVS_FLAT ) )
			->AddState( ID_TAG( LVS_RAISED ) )
			->AddState( ID_TAG( LVS_SUNKEN ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"HEADER" );

		pClass->AddPart( ID_TAG( HP_HEADERITEM ) )
			->AddState( ID_TAG( HIS_NORMAL ) )
			->AddState( ID_TAG( HIS_HOT ) )
			->AddState( ID_TAG( HIS_PRESSED ) )
			->AddState( ID_TAG( HIS_SORTEDNORMAL ) )
			->AddState( ID_TAG( HIS_SORTEDHOT ) )
			->AddState( ID_TAG( HIS_SORTEDPRESSED ) )
			->AddState( ID_TAG( HIS_ICONNORMAL ) )
			->AddState( ID_TAG( HIS_ICONHOT ) )
			->AddState( ID_TAG( HIS_ICONPRESSED ) )
			->AddState( ID_TAG( HIS_ICONSORTEDNORMAL ) )
			->AddState( ID_TAG( HIS_ICONSORTEDHOT ) )
			->AddState( ID_TAG( HIS_ICONSORTEDPRESSED ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERITEMLEFT ) )
			->AddState( ID_TAG( HILS_NORMAL ) )
			->AddState( ID_TAG( HILS_HOT ) )
			->AddState( ID_TAG( HILS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERITEMRIGHT ) )
			->AddState( ID_TAG( HIRS_NORMAL ) )
			->AddState( ID_TAG( HIRS_HOT ) )
			->AddState( ID_TAG( HIRS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERSORTARROW ) )
			->AddState( ID_TAG( HSAS_SORTEDUP ) )
			->AddState( ID_TAG( HSAS_SORTEDDOWN ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERDROPDOWN ) )
			->AddState( ID_TAG( HDDS_NORMAL ) )
			->AddState( ID_TAG( HDDS_SOFTHOT ) )
			->AddState( ID_TAG( HDDS_HOT ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERDROPDOWNFILTER ) )
			->AddState( ID_TAG( HDDFS_NORMAL ) )
			->AddState( ID_TAG( HDDFS_SOFTHOT ) )
			->AddState( ID_TAG( HDDFS_HOT ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADEROVERFLOW ) )
			->AddState( ID_TAG( HOFS_NORMAL ) )
			->AddState( ID_TAG( HOFS_HOT ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"LISTBOX", MediumRelevance );

		pClass->AddPart( ID_TAG( LBCP_BORDER_HSCROLL ), MediumRelevance )
			->AddState( ID_TAG( LBPSH_NORMAL ) )
			->AddState( ID_TAG( LBPSH_FOCUSED ) )
			->AddState( ID_TAG( LBPSH_HOT ) )
			->AddState( ID_TAG( LBPSH_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_HVSCROLL ), MediumRelevance )
			->AddState( ID_TAG( LBPSHV_NORMAL ) )
			->AddState( ID_TAG( LBPSHV_FOCUSED ) )
			->AddState( ID_TAG( LBPSHV_HOT ) )
			->AddState( ID_TAG( LBPSHV_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_NOSCROLL ), MediumRelevance )
			->AddState( ID_TAG( LBPSN_NORMAL ) )
			->AddState( ID_TAG( LBPSN_FOCUSED ) )
			->AddState( ID_TAG( LBPSN_HOT ) )
			->AddState( ID_TAG( LBPSN_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_VSCROLL ), MediumRelevance )
			->AddState( ID_TAG( LBPSV_NORMAL ) )
			->AddState( ID_TAG( LBPSV_FOCUSED ) )
			->AddState( ID_TAG( LBPSV_HOT ) )
			->AddState( ID_TAG( LBPSV_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_ITEM ) )
			->AddState( ID_TAG( LBPSI_HOT ) )
			->AddState( ID_TAG( LBPSI_HOTSELECTED ) )
			->AddState( ID_TAG( LBPSI_SELECTED ) )
			->AddState( ID_TAG( LBPSI_SELECTEDNOTFOCUS ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"LISTVIEW" );

		pClass->AddPart( ID_TAG( LVP_LISTITEM ), MediumRelevance )
			->AddState( ID_TAG( LISS_NORMAL ) )
			->AddState( ID_TAG( LISS_HOT ) )
			->AddState( ID_TAG( LISS_SELECTED ) )
			->AddState( ID_TAG( LISS_DISABLED ) )
			->AddState( ID_TAG( LISS_SELECTEDNOTFOCUS ) )
			->AddState( ID_TAG( LISS_HOTSELECTED ) )
		;
		pClass->AddPart( ID_TAG( LVP_LISTGROUP ), MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_LISTDETAIL ), MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_LISTSORTEDDETAIL ), MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_EMPTYTEXT ), MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_GROUPHEADER ) )
			->AddState( ID_TAG( LVGH_OPEN ) )
			->AddState( ID_TAG( LVGH_OPENHOT ) )
			->AddState( ID_TAG( LVGH_OPENSELECTED ) )
			->AddState( ID_TAG( LVGH_OPENSELECTEDHOT ) )
			->AddState( ID_TAG( LVGH_OPENSELECTEDNOTFOCUSED ) )
			->AddState( ID_TAG( LVGH_OPENSELECTEDNOTFOCUSEDHOT ) )
			->AddState( ID_TAG( LVGH_OPENMIXEDSELECTION ) )
			->AddState( ID_TAG( LVGH_OPENMIXEDSELECTIONHOT ) )
			->AddState( ID_TAG( LVGH_CLOSE ) )
			->AddState( ID_TAG( LVGH_CLOSEHOT ) )
			->AddState( ID_TAG( LVGH_CLOSESELECTED ) )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDHOT ) )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDNOTFOCUSED ) )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDNOTFOCUSEDHOT ) )
			->AddState( ID_TAG( LVGH_CLOSEMIXEDSELECTION ) )
			->AddState( ID_TAG( LVGH_CLOSEMIXEDSELECTIONHOT ) )
		;
		pClass->AddPart( ID_TAG( LVP_GROUPHEADERLINE ) )
			->AddState( ID_TAG( LVGHL_OPEN ) )
			->AddState( ID_TAG( LVGHL_OPENHOT ) )
			->AddState( ID_TAG( LVGHL_OPENSELECTED ) )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDHOT ) )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDNOTFOCUSED ) )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDNOTFOCUSEDHOT ) )
			->AddState( ID_TAG( LVGHL_OPENMIXEDSELECTION ) )
			->AddState( ID_TAG( LVGHL_OPENMIXEDSELECTIONHOT ) )
			->AddState( ID_TAG( LVGHL_CLOSE ) )
			->AddState( ID_TAG( LVGHL_CLOSEHOT ) )
			->AddState( ID_TAG( LVGHL_CLOSESELECTED ) )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDHOT ) )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDNOTFOCUSED ) )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDNOTFOCUSEDHOT ) )
			->AddState( ID_TAG( LVGHL_CLOSEMIXEDSELECTION ) )
			->AddState( ID_TAG( LVGHL_CLOSEMIXEDSELECTIONHOT ) )
		;
		pClass->AddPart( ID_TAG( LVP_EXPANDBUTTON ) )
			->AddState( ID_TAG( LVEB_NORMAL ) )
			->AddState( ID_TAG( LVEB_HOVER ) )
			->AddState( ID_TAG( LVEB_PUSHED ) )
		;
		pClass->AddPart( ID_TAG( LVP_COLLAPSEBUTTON ) )
			->AddState( ID_TAG( LVCB_NORMAL ) )
			->AddState( ID_TAG( LVCB_HOVER ) )
			->AddState( ID_TAG( LVCB_PUSHED ) )
		;
		pClass->AddPart( ID_TAG( LVP_COLUMNDETAIL ) );
	}

	{
		CThemeClass* pClass = AddClass( L"MENU" );

		pClass->AddPart( ID_TAG( MENU_MENUITEM_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUDROPDOWN_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUBARITEM_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUBARDROPDOWN_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_CHEVRON_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_SEPARATOR_TMSCHEMA ), ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_BARBACKGROUND ) )
			->AddState( ID_TAG( MB_ACTIVE ) )
			->AddState( ID_TAG( MB_INACTIVE ) )
		;
		pClass->AddPart( ID_TAG( MENU_BARITEM ) )
			->AddState( ID_TAG( MBI_NORMAL ) )
			->AddState( ID_TAG( MBI_HOT ) )
			->AddState( ID_TAG( MBI_PUSHED ) )
			->AddState( ID_TAG( MBI_DISABLED ) )
			->AddState( ID_TAG( MBI_DISABLEDHOT ) )
			->AddState( ID_TAG( MBI_DISABLEDPUSHED ) )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPBACKGROUND ) );
		pClass->AddPart( ID_TAG( MENU_POPUPBORDERS ) );

		pClass->AddPart( ID_TAG( MENU_POPUPCHECK ) )
			->AddState( ID_TAG( MC_CHECKMARKNORMAL ) )
			->AddState( ID_TAG( MC_CHECKMARKDISABLED ) )
			->AddState( ID_TAG( MC_BULLETNORMAL ) )
			->AddState( ID_TAG( MC_BULLETDISABLED ) )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPCHECKBACKGROUND ) )
			->AddState( ID_TAG( MCB_DISABLED ) )
			->AddState( ID_TAG( MCB_NORMAL ) )
			->AddState( ID_TAG( MCB_BITMAP ) )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPGUTTER ) );
		pClass->AddPart( ID_TAG( MENU_POPUPITEM ) )
			->AddState( ID_TAG( MPI_NORMAL ) )
			->AddState( ID_TAG( MPI_HOT ) )
			->AddState( ID_TAG( MPI_DISABLED ) )
			->AddState( ID_TAG( MPI_DISABLEDHOT ) )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPSEPARATOR ) );

		pClass->AddPart( ID_TAG( MENU_POPUPSUBMENU ) )
			->AddState( ID_TAG( MSM_NORMAL ) )
			->AddState( ID_TAG( MSM_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMCLOSE ), MediumRelevance )
			->AddState( ID_TAG( MSYSC_NORMAL ) )
			->AddState( ID_TAG( MSYSC_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMMAXIMIZE ), MediumRelevance )
			->AddState( ID_TAG( MSYSMX_NORMAL ) )
			->AddState( ID_TAG( MSYSMX_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMMINIMIZE ), MediumRelevance )
			->AddState( ID_TAG( MSYSMN_NORMAL ) )
			->AddState( ID_TAG( MSYSMN_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMRESTORE ), MediumRelevance )
			->AddState( ID_TAG( MSYSR_NORMAL ) )
			->AddState( ID_TAG( MSYSR_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"MENUBAND" );		// ObscureRelevance

		pClass->AddPart( ID_TAG( MDP_NEWAPPBUTTON ) )
			->AddState( ID_TAG( MDS_NORMAL ) )
			->AddState( ID_TAG( MDS_HOT ) )
			->AddState( ID_TAG( MDS_PRESSED ) )
			->AddState( ID_TAG( MDS_DISABLED ) )
			->AddState( ID_TAG( MDS_CHECKED ) )
			->AddState( ID_TAG( MDS_HOTCHECKED ) )
		;
		pClass->AddPart( ID_TAG( MDP_SEPERATOR ) );
	}

	{
		CThemeClass* pClass = AddClass( L"NAVIGATION", MediumRelevance );

		pClass->AddPart( ID_TAG( NAV_BACKBUTTON ) )
			->AddState( ID_TAG( NAV_BB_NORMAL ) )
			->AddState( ID_TAG( NAV_BB_HOT ) )
			->AddState( ID_TAG( NAV_BB_PRESSED ) )
			->AddState( ID_TAG( NAV_BB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( NAV_FORWARDBUTTON ) )
			->AddState( ID_TAG( NAV_FB_NORMAL ) )
			->AddState( ID_TAG( NAV_FB_HOT ) )
			->AddState( ID_TAG( NAV_FB_PRESSED ) )
			->AddState( ID_TAG( NAV_FB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( NAV_MENUBUTTON ) )
			->AddState( ID_TAG( NAV_MB_NORMAL ) )
			->AddState( ID_TAG( NAV_MB_HOT ) )
			->AddState( ID_TAG( NAV_MB_PRESSED ) )
			->AddState( ID_TAG( NAV_MB_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"PAGE" );		// ObscureRelevance

		pClass->AddPart( ID_TAG( PGRP_UP ) )
			->AddState( ID_TAG( UPS_NORMAL ) )
			->AddState( ID_TAG( UPS_HOT ) )
			->AddState( ID_TAG( UPS_PRESSED ) )
			->AddState( ID_TAG( UPS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( PGRP_DOWN ) )
			->AddState( ID_TAG( DNS_NORMAL ) )
			->AddState( ID_TAG( DNS_HOT ) )
			->AddState( ID_TAG( DNS_PRESSED ) )
			->AddState( ID_TAG( DNS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( PGRP_UPHORZ ) )
			->AddState( ID_TAG( UPHZS_NORMAL ) )
			->AddState( ID_TAG( UPHZS_HOT ) )
			->AddState( ID_TAG( UPHZS_PRESSED ) )
			->AddState( ID_TAG( UPHZS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( PGRP_DOWNHORZ ) )
			->AddState( ID_TAG( DNHZS_NORMAL ) )
			->AddState( ID_TAG( DNHZS_HOT ) )
			->AddState( ID_TAG( DNHZS_PRESSED ) )
			->AddState( ID_TAG( DNHZS_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"PROGRESS" );

		pClass->AddPart( ID_TAG( PP_BAR ) );
		pClass->AddPart( ID_TAG( PP_BARVERT ) );
		pClass->AddPart( ID_TAG( PP_CHUNK ) );
		pClass->AddPart( ID_TAG( PP_CHUNKVERT ) );
		pClass->AddPart( ID_TAG( PP_FILL ) )
			->AddState( ID_TAG( PBFS_NORMAL ) )
			->AddState( ID_TAG( PBFS_ERROR ) )
			->AddState( ID_TAG( PBFS_PAUSED ) )
			->AddState( ID_TAG( PBFS_PARTIAL ) )
		;
		pClass->AddPart( ID_TAG( PP_FILLVERT ) )
			->AddState( ID_TAG( PBFVS_NORMAL ) )
			->AddState( ID_TAG( PBFVS_ERROR ) )
			->AddState( ID_TAG( PBFVS_PAUSED ) )
			->AddState( ID_TAG( PBFVS_PARTIAL ) )
		;
		pClass->AddPart( ID_TAG( PP_PULSEOVERLAY ) );
		pClass->AddPart( ID_TAG( PP_MOVEOVERLAY ) );
		pClass->AddPart( ID_TAG( PP_PULSEOVERLAYVERT ) );
		pClass->AddPart( ID_TAG( PP_MOVEOVERLAYVERT ) );
		pClass->AddPart( ID_TAG( PP_TRANSPARENTBAR ) )
			->AddState( ID_TAG( PBBS_NORMAL ) )
			->AddState( ID_TAG( PBBS_PARTIAL ) )
		;
		pClass->AddPart( ID_TAG( PP_TRANSPARENTBARVERT ) )
			->AddState( ID_TAG( PBBVS_NORMAL ) )
			->AddState( ID_TAG( PBBVS_PARTIAL ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"REBAR" );

		pClass->AddPart( ID_TAG( RP_GRIPPER ) );
		pClass->AddPart( ID_TAG( RP_GRIPPERVERT ) );
		pClass->AddPart( ID_TAG( RP_BAND ) );
		pClass->AddPart( ID_TAG( RP_CHEVRON ), MediumRelevance )
			->AddState( ID_TAG( CHEVS_NORMAL ) )
			->AddState( ID_TAG( CHEVS_HOT ) )
			->AddState( ID_TAG( CHEVS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( RP_CHEVRONVERT ), MediumRelevance )
			->AddState( ID_TAG( CHEVSV_NORMAL ) )
			->AddState( ID_TAG( CHEVSV_HOT ) )
			->AddState( ID_TAG( CHEVSV_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( RP_BACKGROUND ) );
		pClass->AddPart( ID_TAG( RP_SPLITTER ) )
			->AddState( ID_TAG( SPLITS_NORMAL ) )
			->AddState( ID_TAG( SPLITS_HOT ) )
			->AddState( ID_TAG( SPLITS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( RP_SPLITTERVERT ) )
			->AddState( ID_TAG( SPLITSV_NORMAL ) )
			->AddState( ID_TAG( SPLITSV_HOT ) )
			->AddState( ID_TAG( SPLITSV_PRESSED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"SCROLLBAR" );

		pClass->AddPart( ID_TAG( SBP_ARROWBTN ) )
			->AddState( ID_TAG( ABS_UPNORMAL ) )
			->AddState( ID_TAG( ABS_UPHOT ) )
			->AddState( ID_TAG( ABS_UPPRESSED ) )
			->AddState( ID_TAG( ABS_UPDISABLED ) )
			->AddState( ID_TAG( ABS_UPHOVER ) )
			->AddState( ID_TAG( ABS_DOWNNORMAL ) )
			->AddState( ID_TAG( ABS_DOWNHOT ) )
			->AddState( ID_TAG( ABS_DOWNPRESSED ) )
			->AddState( ID_TAG( ABS_DOWNDISABLED ) )
			->AddState( ID_TAG( ABS_DOWNHOVER ) )
			->AddState( ID_TAG( ABS_LEFTNORMAL ) )
			->AddState( ID_TAG( ABS_LEFTHOT ) )
			->AddState( ID_TAG( ABS_LEFTPRESSED ) )
			->AddState( ID_TAG( ABS_LEFTDISABLED ) )
			->AddState( ID_TAG( ABS_LEFTHOVER ) )
			->AddState( ID_TAG( ABS_RIGHTNORMAL ) )
			->AddState( ID_TAG( ABS_RIGHTHOT ) )
			->AddState( ID_TAG( ABS_RIGHTPRESSED ) )
			->AddState( ID_TAG( ABS_RIGHTDISABLED ) )
			->AddState( ID_TAG( ABS_RIGHTHOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_THUMBBTNHORZ ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_THUMBBTNVERT ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_LOWERTRACKHORZ ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_UPPERTRACKHORZ ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_LOWERTRACKVERT ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_UPPERTRACKVERT ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_GRIPPERHORZ ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_GRIPPERVERT ) )
			->AddState( ID_TAG( SCRBS_NORMAL ) )
			->AddState( ID_TAG( SCRBS_HOT ) )
			->AddState( ID_TAG( SCRBS_PRESSED ) )
			->AddState( ID_TAG( SCRBS_DISABLED ) )
			->AddState( ID_TAG( SCRBS_HOVER ) )
		;
		pClass->AddPart( ID_TAG( SBP_SIZEBOX ) )
			->AddState( ID_TAG( SZB_RIGHTALIGN ) )
			->AddState( ID_TAG( SZB_LEFTALIGN ) )
			->AddState( ID_TAG( SZB_TOPRIGHTALIGN ) )
			->AddState( ID_TAG( SZB_TOPLEFTALIGN ) )
			->AddState( ID_TAG( SZB_HALFBOTTOMRIGHTALIGN ) )
			->AddState( ID_TAG( SZB_HALFBOTTOMLEFTALIGN ) )
			->AddState( ID_TAG( SZB_HALFTOPRIGHTALIGN ) )
			->AddState( ID_TAG( SZB_HALFTOPLEFTALIGN ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"SEARCHEDITBOX", MediumRelevance );

		pClass->AddPart( SEBP_SEARCHEDITBOXTEXT, L"SEBP_SEARCHEDITBOXTEXT (Windows 7)" )
			->AddState( ID_TAG( SEBTS_FORMATTED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"SPIN" );

		pClass->AddPart( ID_TAG( SPNP_UP ) )
			->AddState( ID_TAG( UPS_NORMAL ) )
			->AddState( ID_TAG( UPS_HOT ) )
			->AddState( ID_TAG( UPS_PRESSED ) )
			->AddState( ID_TAG( UPS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( SPNP_DOWN ) )
			->AddState( ID_TAG( DNS_NORMAL ) )
			->AddState( ID_TAG( DNS_HOT ) )
			->AddState( ID_TAG( DNS_PRESSED ) )
			->AddState( ID_TAG( DNS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( SPNP_UPHORZ ) )
			->AddState( ID_TAG( UPHZS_NORMAL ) )
			->AddState( ID_TAG( UPHZS_HOT ) )
			->AddState( ID_TAG( UPHZS_PRESSED ) )
			->AddState( ID_TAG( UPHZS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( SPNP_DOWNHORZ ) )
			->AddState( ID_TAG( DNHZS_NORMAL ) )
			->AddState( ID_TAG( DNHZS_HOT ) )
			->AddState( ID_TAG( DNHZS_PRESSED ) )
			->AddState( ID_TAG( DNHZS_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"STARTPANEL", MediumRelevance );

		pClass->AddPart( ID_TAG( SPP_USERPANE ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMS ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSARROW ) )
			->AddState( ID_TAG( SPS_NORMAL ) )
			->AddState( ID_TAG( SPS_HOT ) )
			->AddState( ID_TAG( SPS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( SPP_PROGLIST ) );
		pClass->AddPart( ID_TAG( SPP_PROGLISTSEPARATOR ) );
		pClass->AddPart( ID_TAG( SPP_PLACESLIST ) );
		pClass->AddPart( ID_TAG( SPP_PLACESLISTSEPARATOR ) );
		pClass->AddPart( ID_TAG( SPP_LOGOFF ) );
		pClass->AddPart( ID_TAG( SPP_LOGOFFBUTTONS ) )
			->AddState( ID_TAG( SPLS_NORMAL ) )
			->AddState( ID_TAG( SPLS_HOT ) )
			->AddState( ID_TAG( SPLS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( SPP_USERPICTURE ) );
		pClass->AddPart( ID_TAG( SPP_PREVIEW ) );

		pClass->AddPart( ID_TAG( SPP_PREVIEW ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSTAB ) )
			->AddState( ID_TAG( SPSB_NORMAL ) )
			->AddState( ID_TAG( SPSB_HOT ) )
			->AddState( ID_TAG( SPSB_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( SPP_NSCHOST ) );
		pClass->AddPart( ID_TAG( SPP_SOFTWAREEXPLORER ) );
		pClass->AddPart( ID_TAG( SPP_OPENBOX ) );
		pClass->AddPart( ID_TAG( SPP_SEARCHVIEW ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSARROWBACK ) );
		pClass->AddPart( ID_TAG( SPP_TOPMATCH ) );
		pClass->AddPart( ID_TAG( SPP_LOGOFFSPLITBUTTONDROPDOWN ) )
			->AddState( ID_TAG( SPLS_NORMAL ) )
			->AddState( ID_TAG( SPLS_HOT ) )
			->AddState( ID_TAG( SPLS_PRESSED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"STATUS", MediumRelevance );

		pClass->AddPart( ID_TAG( SP_PANE ) );
		pClass->AddPart( ID_TAG( SP_GRIPPERPANE ) );
		pClass->AddPart( ID_TAG( SP_GRIPPER ), MediumRelevance );
	}

	{
		CThemeClass* pClass = AddClass( L"TAB" );

		pClass->AddPart( ID_TAG( TABP_TABITEM ) )
			->AddState( ID_TAG( TIS_NORMAL ) )
			->AddState( ID_TAG( TIS_HOT ) )
			->AddState( ID_TAG( TIS_SELECTED ) )
			->AddState( ID_TAG( TIS_DISABLED ) )
			->AddState( ID_TAG( TIS_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TABITEMLEFTEDGE ) )
			->AddState( ID_TAG( TILES_NORMAL ) )
			->AddState( ID_TAG( TILES_HOT ) )
			->AddState( ID_TAG( TILES_SELECTED ) )
			->AddState( ID_TAG( TILES_DISABLED ) )
			->AddState( ID_TAG( TILES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TABITEMRIGHTEDGE ) )
			->AddState( ID_TAG( TIRES_NORMAL ) )
			->AddState( ID_TAG( TIRES_HOT ) )
			->AddState( ID_TAG( TIRES_SELECTED ) )
			->AddState( ID_TAG( TIRES_DISABLED ) )
			->AddState( ID_TAG( TIRES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TABITEMBOTHEDGE ) )
			->AddState( ID_TAG( TIBES_NORMAL ) )
			->AddState( ID_TAG( TIBES_HOT ) )
			->AddState( ID_TAG( TIBES_SELECTED ) )
			->AddState( ID_TAG( TIBES_DISABLED ) )
			->AddState( ID_TAG( TIBES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TOPTABITEM ) )
			->AddState( ID_TAG( TTIS_NORMAL ) )
			->AddState( ID_TAG( TTIS_HOT ) )
			->AddState( ID_TAG( TTIS_SELECTED ) )
			->AddState( ID_TAG( TTIS_DISABLED ) )
			->AddState( ID_TAG( TTIS_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TOPTABITEMLEFTEDGE ) )
			->AddState( ID_TAG( TTILES_NORMAL ) )
			->AddState( ID_TAG( TTILES_HOT ) )
			->AddState( ID_TAG( TTILES_SELECTED ) )
			->AddState( ID_TAG( TTILES_DISABLED ) )
			->AddState( ID_TAG( TTILES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TOPTABITEMRIGHTEDGE ) )
			->AddState( ID_TAG( TTIRES_NORMAL ) )
			->AddState( ID_TAG( TTIRES_HOT ) )
			->AddState( ID_TAG( TTIRES_SELECTED ) )
			->AddState( ID_TAG( TTIRES_DISABLED ) )
			->AddState( ID_TAG( TTIRES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_TOPTABITEMBOTHEDGE ) )
			->AddState( ID_TAG( TTIBES_NORMAL ) )
			->AddState( ID_TAG( TTIBES_HOT ) )
			->AddState( ID_TAG( TTIBES_SELECTED ) )
			->AddState( ID_TAG( TTIBES_DISABLED ) )
			->AddState( ID_TAG( TTIBES_FOCUSED ) )
		;
		pClass->AddPart( ID_TAG( TABP_PANE ), MediumRelevance );
		pClass->AddPart( ID_TAG( TABP_BODY ), MediumRelevance );
		pClass->AddPart( ID_TAG( TABP_AEROWIZARDBODY ), MediumRelevance );
	}

	{
		CThemeClass* pClass = AddClass( L"TASKBAND", MediumRelevance );

		pClass->AddPart( ID_TAG( TDP_GROUPCOUNT ), MediumRelevance );
		pClass->AddPart( ID_TAG( TDP_FLASHBUTTON ) );
		pClass->AddPart( ID_TAG( TDP_FLASHBUTTONGROUPMENU ) );
	}

	{
		CThemeClass* pClass = AddClass( L"TASKBAR", MediumRelevance );

		pClass->AddPart( ID_TAG( TBP_BACKGROUNDBOTTOM ) );
		pClass->AddPart( ID_TAG( TBP_BACKGROUNDRIGHT ) );
		pClass->AddPart( ID_TAG( TBP_BACKGROUNDTOP ) );
		pClass->AddPart( ID_TAG( TBP_BACKGROUNDLEFT ) );
		pClass->AddPart( ID_TAG( TBP_SIZINGBARBOTTOM ) );
		pClass->AddPart( ID_TAG( TBP_SIZINGBARRIGHT ) );
		pClass->AddPart( ID_TAG( TBP_SIZINGBARTOP ) );
		pClass->AddPart( ID_TAG( TBP_SIZINGBARLEFT ) );
	}

	{
		CThemeClass* pClass = AddClass( L"TASKDIALOG", MediumRelevance );

		pClass->AddPart( ID_TAG( TDLG_PRIMARYPANEL ) );
		pClass->AddPart( ID_TAG( TDLG_MAININSTRUCTIONPANE ) );
		pClass->AddPart( ID_TAG( TDLG_MAINICON ) );
		pClass->AddPart( ID_TAG( TDLG_CONTENTPANE ) )
			->AddState( ID_TAG( TDLGCPS_STANDALONE ) )
		;
		pClass->AddPart( ID_TAG( TDLG_CONTENTICON ) );
		pClass->AddPart( ID_TAG( TDLG_EXPANDEDCONTENT ) );
		pClass->AddPart( ID_TAG( TDLG_COMMANDLINKPANE ) );
		pClass->AddPart( ID_TAG( TDLG_SECONDARYPANEL ) );
		pClass->AddPart( ID_TAG( TDLG_CONTROLPANE ) );
		pClass->AddPart( ID_TAG( TDLG_BUTTONSECTION ) );
		pClass->AddPart( ID_TAG( TDLG_BUTTONWRAPPER ) );
		pClass->AddPart( ID_TAG( TDLG_EXPANDOTEXT ) );
		pClass->AddPart( ID_TAG( TDLG_EXPANDOBUTTON ) )
			->AddState( ID_TAG( TDLGEBS_NORMAL ) )
			->AddState( ID_TAG( TDLGEBS_HOVER ) )
			->AddState( ID_TAG( TDLGEBS_PRESSED ) )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDNORMAL ) )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDHOVER ) )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDPRESSED ) )
			->AddState( ID_TAG( TDLGEBS_NORMALDISABLED ) )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDDISABLED ) )
		;
		pClass->AddPart( ID_TAG( TDLG_VERIFICATIONTEXT ) );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTEPANE ) );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTEAREA ) );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTESEPARATOR ) );
		pClass->AddPart( ID_TAG( TDLG_EXPANDEDFOOTERAREA ) );
		pClass->AddPart( ID_TAG( TDLG_PROGRESSBAR ) );
		pClass->AddPart( ID_TAG( TDLG_IMAGEALIGNMENT ) );
		pClass->AddPart( ID_TAG( TDLG_RADIOBUTTONPANE ) );
	}

	{
		CThemeClass* pClass = AddClass( L"TEXTSTYLE", MediumRelevance );

		pClass->AddPart( ID_TAG( TEXT_MAININSTRUCTION ) );
		pClass->AddPart( ID_TAG( TEXT_INSTRUCTION ) );
		pClass->AddPart( ID_TAG( TEXT_BODYTITLE ) );
		pClass->AddPart( ID_TAG( TEXT_BODYTEXT  ) );
		pClass->AddPart( ID_TAG( TEXT_SECONDARYTEXT ) );
		pClass->AddPart( ID_TAG( TEXT_HYPERLINKTEXT ) )
			->AddState( ID_TAG( TS_HYPERLINK_NORMAL ) )
			->AddState( ID_TAG( TS_HYPERLINK_HOT ) )
			->AddState( ID_TAG( TS_HYPERLINK_PRESSED ) )
			->AddState( ID_TAG( TS_HYPERLINK_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TEXT_EXPANDED ) );
		pClass->AddPart( ID_TAG( TEXT_LABEL ) );
		pClass->AddPart( ID_TAG( TEXT_CONTROLLABEL ) )
			->AddState( ID_TAG( TS_CONTROLLABEL_NORMAL ) )
			->AddState( ID_TAG( TS_CONTROLLABEL_DISABLED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TOOLBAR" );

		pClass->AddPart( ID_TAG( TP_BUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( ID_TAG( TP_DROPDOWNBUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( ID_TAG( TP_SPLITBUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( ID_TAG( TP_SPLITBUTTONDROPDOWN ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( ID_TAG( TP_SEPARATOR ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( ID_TAG( TP_SEPARATORVERT ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pClass->AddPart( TP_DROPDOWNBUTTONGLYPH, L"TP_DROPDOWNBUTTONGLYPH (Windows 7)" )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TOOLTIP" );

		pClass->AddPart( ID_TAG( TTP_STANDARD ) )
			->AddState( ID_TAG( TTSS_NORMAL ) )
			->AddState( ID_TAG( TTSS_LINK ) )
		;
		pClass->AddPart( ID_TAG( TTP_STANDARDTITLE ) )
			->AddState( ID_TAG( TTSS_NORMAL ) )
			->AddState( ID_TAG( TTSS_LINK ) )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOON ) )
			->AddState( ID_TAG( TTBS_NORMAL ) )
			->AddState( ID_TAG( TTBS_LINK ) )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOONTITLE ) );
		pClass->AddPart( ID_TAG( TTP_CLOSE ) )
			->AddState( ID_TAG( TTCS_NORMAL ) )
			->AddState( ID_TAG( TTCS_HOT ) )
			->AddState( ID_TAG( TTCS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOONSTEM ) )
			->AddState( ID_TAG( TTBSS_POINTINGUPLEFTWALL ) )
			->AddState( ID_TAG( TTBSS_POINTINGUPCENTERED ) )
			->AddState( ID_TAG( TTBSS_POINTINGUPRIGHTWALL ) )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNRIGHTWALL ) )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNCENTERED ) )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNLEFTWALL ) )
		;
		pClass->AddPart( TTP_WRENCH, L"TTP_WRENCH (Windows 7)" )
			->AddState( ID_TAG( TTWS_NORMAL ) )
			->AddState( ID_TAG( TTWS_HOT ) )
			->AddState( ID_TAG( TTWS_PRESSED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TRACKBAR" );

		pClass->AddPart( ID_TAG( TKP_TRACK ) )
			->AddState( ID_TAG( TRS_NORMAL ) )
		;
		pClass->AddPart( ID_TAG( TKP_TRACKVERT ) )
			->AddState( ID_TAG( TRVS_NORMAL ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMB ) )
			->AddState( ID_TAG( TUS_NORMAL ) )
			->AddState( ID_TAG( TUS_HOT ) )
			->AddState( ID_TAG( TUS_PRESSED ) )
			->AddState( ID_TAG( TUS_FOCUSED ) )
			->AddState( ID_TAG( TUS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMBBOTTOM ) )
			->AddState( ID_TAG( TUBS_NORMAL ) )
			->AddState( ID_TAG( TUBS_HOT ) )
			->AddState( ID_TAG( TUBS_PRESSED ) )
			->AddState( ID_TAG( TUBS_FOCUSED ) )
			->AddState( ID_TAG( TUBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMBTOP ) )
			->AddState( ID_TAG( TUTS_NORMAL ) )
			->AddState( ID_TAG( TUTS_HOT ) )
			->AddState( ID_TAG( TUTS_PRESSED ) )
			->AddState( ID_TAG( TUTS_FOCUSED ) )
			->AddState( ID_TAG( TUTS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMBVERT ) )
			->AddState( ID_TAG( TUVS_NORMAL ) )
			->AddState( ID_TAG( TUVS_HOT ) )
			->AddState( ID_TAG( TUVS_PRESSED ) )
			->AddState( ID_TAG( TUVS_FOCUSED ) )
			->AddState( ID_TAG( TUVS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMBLEFT ) )
			->AddState( ID_TAG( TUVLS_NORMAL ) )
			->AddState( ID_TAG( TUVLS_HOT ) )
			->AddState( ID_TAG( TUVLS_PRESSED ) )
			->AddState( ID_TAG( TUVLS_FOCUSED ) )
			->AddState( ID_TAG( TUVLS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_THUMBRIGHT ) )
			->AddState( ID_TAG( TUVRS_NORMAL ) )
			->AddState( ID_TAG( TUVRS_HOT ) )
			->AddState( ID_TAG( TUVRS_PRESSED ) )
			->AddState( ID_TAG( TUVRS_FOCUSED ) )
			->AddState( ID_TAG( TUVRS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TKP_TICS ) )
			->AddState( ID_TAG( TSS_NORMAL ) )
		;
		pClass->AddPart( ID_TAG( TKP_TICSVERT ) )
			->AddState( ID_TAG( TSVS_NORMAL ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TRAYNOTIFY" );		// ObscureRelevance

		pClass->AddPart( ID_TAG( TNP_BACKGROUND ) );
		pClass->AddPart( ID_TAG( TNP_ANIMBACKGROUND ) );
	}

	{
		CThemeClass* pClass = AddClass( L"TREEVIEW" );

		pClass->AddPart( ID_TAG( TVP_GLYPH ) )
			->AddState( ID_TAG( GLPS_CLOSED ) )
			->AddState( ID_TAG( GLPS_OPENED ) )
		;
		pClass->AddPart( ID_TAG( TVP_TREEITEM ) )
			->AddState( ID_TAG( TREIS_NORMAL ) )
			->AddState( ID_TAG( TREIS_HOT ) )
			->AddState( ID_TAG( TREIS_SELECTED ) )
			->AddState( ID_TAG( TREIS_DISABLED ) )
			->AddState( ID_TAG( TREIS_SELECTEDNOTFOCUS ) )
			->AddState( ID_TAG( TREIS_HOTSELECTED ) )
		;
		pClass->AddPart( ID_TAG( TVP_BRANCH ) );
		pClass->AddPart( ID_TAG( TVP_HOTGLYPH ) )
			->AddState( ID_TAG( HGLPS_CLOSED ) )
			->AddState( ID_TAG( HGLPS_OPENED ) )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"WINDOW" );

		pClass->AddPart( ID_TAG( WP_CAPTION ) )
			->AddState( ID_TAG( CS_ACTIVE ) )
			->AddState( ID_TAG( CS_INACTIVE ) )
			->AddState( ID_TAG( CS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_SMALLCAPTION ) );
		pClass->AddPart( ID_TAG( WP_MINCAPTION ) )
			->AddState( ID_TAG( MNCS_ACTIVE ) )
			->AddState( ID_TAG( MNCS_INACTIVE ) )
			->AddState( ID_TAG( MNCS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_SMALLMINCAPTION ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_MAXCAPTION ) )
			->AddState( ID_TAG( MXCS_ACTIVE ) )
			->AddState( ID_TAG( MXCS_INACTIVE ) )
			->AddState( ID_TAG( MXCS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_SMALLMAXCAPTION ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_FRAMELEFT ) );
		pClass->AddPart( ID_TAG( WP_FRAMERIGHT ) );
		pClass->AddPart( ID_TAG( WP_FRAMEBOTTOM ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMELEFT ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMERIGHT ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMEBOTTOM ) );
		pClass->AddPart( ID_TAG( WP_SYSBUTTON ) )				// ObscureRelevance
			->AddState( ID_TAG( SBS_NORMAL ) )
			->AddState( ID_TAG( SBS_HOT ) )
			->AddState( ID_TAG( SBS_PUSHED ) )
			->AddState( ID_TAG( SBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_MDISYSBUTTON ) );			// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_MINBUTTON ) )
			->AddState( ID_TAG( MINBS_NORMAL ) )
			->AddState( ID_TAG( MINBS_HOT ) )
			->AddState( ID_TAG( MINBS_PUSHED ) )
			->AddState( ID_TAG( MINBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_MDIMINBUTTON ) );
		pClass->AddPart( ID_TAG( WP_MAXBUTTON ) )
			->AddState( ID_TAG( MAXBS_NORMAL ) )
			->AddState( ID_TAG( MAXBS_HOT ) )
			->AddState( ID_TAG( MAXBS_PUSHED ) )
			->AddState( ID_TAG( MAXBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_CLOSEBUTTON ) )
			->AddState( ID_TAG( CBS_NORMAL ) )
			->AddState( ID_TAG( CBS_HOT ) )
			->AddState( ID_TAG( CBS_PUSHED ) )
			->AddState( ID_TAG( CBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_SMALLCLOSEBUTTON ) );
		pClass->AddPart( ID_TAG( WP_MDICLOSEBUTTON ) );
		pClass->AddPart( ID_TAG( WP_RESTOREBUTTON ) )
			->AddState( ID_TAG( RBS_NORMAL ) )
			->AddState( ID_TAG( RBS_HOT ) )
			->AddState( ID_TAG( RBS_PUSHED ) )
			->AddState( ID_TAG( RBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_MDIRESTOREBUTTON ) );
		pClass->AddPart( ID_TAG( WP_HELPBUTTON ) )
			->AddState( ID_TAG( HBS_NORMAL ) )
			->AddState( ID_TAG( HBS_HOT ) )
			->AddState( ID_TAG( HBS_PUSHED ) )
			->AddState( ID_TAG( HBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_MDIHELPBUTTON ) );			// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_HORZSCROLL ) )
			->AddState( ID_TAG( HSS_NORMAL ) )
			->AddState( ID_TAG( HSS_HOT ) )
			->AddState( ID_TAG( HSS_PUSHED ) )
			->AddState( ID_TAG( HSS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_HORZTHUMB ) )				// ObscureRelevance
			->AddState( ID_TAG( HTS_NORMAL ) )
			->AddState( ID_TAG( HTS_HOT ) )
			->AddState( ID_TAG( HTS_PUSHED ) )
			->AddState( ID_TAG( HTS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_VERTSCROLL ) )				// ObscureRelevance
			->AddState( ID_TAG( VSS_NORMAL ) )
			->AddState( ID_TAG( VSS_HOT ) )
			->AddState( ID_TAG( VSS_PUSHED ) )
			->AddState( ID_TAG( VSS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_VERTTHUMB ) )				// ObscureRelevance
			->AddState( ID_TAG( VTS_NORMAL ) )
			->AddState( ID_TAG( VTS_HOT ) )
			->AddState( ID_TAG( VTS_PUSHED ) )
			->AddState( ID_TAG( VTS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_DIALOG ) );
		pClass->AddPart( ID_TAG( WP_CAPTIONSIZINGTEMPLATE ), MediumRelevance );
		pClass->AddPart( ID_TAG( WP_SMALLCAPTIONSIZINGTEMPLATE ), MediumRelevance );
		pClass->AddPart( ID_TAG( WP_FRAMELEFTSIZINGTEMPLATE ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_SMALLFRAMELEFTSIZINGTEMPLATE ) );	// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_FRAMERIGHTSIZINGTEMPLATE ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_SMALLFRAMERIGHTSIZINGTEMPLATE ) );	// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_FRAMEBOTTOMSIZINGTEMPLATE ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE ) );	// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_FRAME ) )					// ObscureRelevance
			->AddState( ID_TAG( FS_ACTIVE ) )
			->AddState( ID_TAG( FS_INACTIVE ) )
		;
	}
}
