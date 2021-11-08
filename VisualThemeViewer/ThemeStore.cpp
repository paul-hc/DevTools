
#include "stdafx.h"
#include "ThemeStore.h"
#include "utl/ScopedValue.h"
#include "utl/UI/VisualTheme.h"
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

CThemeItemNode CThemeState::MakeThemeItem( void ) const
{
	const CThemePart* pPart = GetParentAs< CThemePart >();

	return CThemeItemNode( pPart->GetParentAs< CThemeClass >()->m_className.c_str(), pPart->m_partId, m_stateId, this );
}


// CThemePart class

CThemePart* CThemePart::AddState( int stateId, const std::wstring& stateName, int flags /*= 0*/, Relevance relevance /*= HighRelevance*/ )
{
	CThemeState* pNewState = new CThemeState( stateId, stateName, relevance, flags );
	pNewState->SetParentNode( this );

	if ( HasFlag( flags, PreviewFlag | PreviewShallowFlag ) )
		SetPreviewState( pNewState, HasFlag( GetFlags(), PreviewShallowFlag ) ? Shallow : Deep );

	m_states.push_back( pNewState );
	return this;
}

void CThemePart::SetDeepFlags( unsigned int addFlags )
{
	ModifyFlags( 0, addFlags );

	for ( CThemeState* pState : m_states )
		pState->ModifyFlags( 0, addFlags );
}

CThemeItemNode CThemePart::MakeThemeItem( void ) const
{
	if ( const CThemeState* pPreviewState = GetPreviewState() )
		return pPreviewState->MakeThemeItem();

	return CThemeItemNode( GetParentAs< CThemeClass >()->m_className.c_str(), m_partId, 0, this );
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

const CThemeState* CThemePart::GetPreviewState( void ) const
{
	if ( NULL == m_pPreviewState && !m_states.empty() )
		m_pPreviewState = *std::min_element( m_states.begin(), m_states.end(), pred::LessValue< pred::TCompareRelevance >() );

	return m_pPreviewState;
}

void CThemePart::SetPreviewState( const CThemeState* pPreviewState, RecursionDepth depth )
{
	ASSERT_PTR( pPreviewState );
	m_pPreviewState = pPreviewState;

	if ( Deep == depth )
		GetParentAs< CThemeClass >()->SetPreviewPart( this );
}


// CThemeClass class

IThemeNode* CThemeClass::FindNode( const std::wstring& code ) const
{
	for ( CThemePart* pPart : m_parts )
		if ( pPart->m_partName == code )
			return pPart;
		else
			for ( CThemeState* pState : pPart->m_states )
				if ( pState->m_stateName == code )
					return pState;

	return NULL;
}

CThemePart* CThemeClass::AddPart( int partId, const std::wstring& partName, int flags /*= 0*/, Relevance relevance /*= HighRelevance*/ )
{
	CThemePart* pNewPart = new CThemePart( partId, partName, relevance, flags );
	pNewPart->SetParentNode( this );
	if ( HasFlag( flags, PreviewFlag ) )
		SetPreviewPart( pNewPart );

	m_parts.push_back( pNewPart );
	return pNewPart;
}

void CThemeClass::SetDeepFlags( unsigned int addFlags )
{
	ModifyFlags( 0, addFlags );

	for ( CThemePart* pPart : m_parts )
		pPart->SetDeepFlags( addFlags );
}

CThemeItemNode CThemeClass::MakeThemeItem( void ) const
{
	if ( const CThemePart* pPreviewPart = GetPreviewPart() )
		return pPreviewPart->MakeThemeItem();

	return CThemeItemNode( m_className.c_str(), 0, 0, this );
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

const CThemePart* CThemeClass::GetPreviewPart( void ) const
{
	if ( NULL == m_pPreviewPart && !m_parts.empty() )
		m_pPreviewPart = *std::min_element( m_parts.begin(), m_parts.end(), pred::LessValue< pred::TCompareRelevance >() );

	return m_pPreviewPart;
}


// CThemeStore class

CThemeClass* CThemeStore::FindClass( const std::wstring& className ) const
{
	for ( CThemeClass* pClass : m_classes )
		if ( pClass->m_className == className )
			return pClass;

	return NULL;
}

IThemeNode* CThemeStore::FindNode( const std::wstring& code ) const
{
	for ( CThemeClass* pClass : m_classes )
		if ( pClass->m_className == code )
			return pClass;
		else
			if ( IThemeNode* pFoundNode = pClass->FindNode( code ) )
				return pFoundNode;

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

size_t CThemeStore::GetTotalCount( void ) const
{
	size_t totalCount = 0;

	for ( CThemeClass* pClass : m_classes )
	{
		++totalCount;

		for ( CThemePart* pPart : pClass->m_parts )
			totalCount += 1 + pPart->m_states.size();
	}
	return totalCount;
}

void CThemeStore::RegisterStandardClasses( void )
{
	{
		CThemeClass* pClass = AddClass( L"BUTTON" );

		pClass->AddPart( ID_TAG( BP_PUSHBUTTON ), TextFlag )
			->AddState( ID_TAG( PBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( PBS_HOT ), TextFlag )
			->AddState( ID_TAG( PBS_PRESSED ), TextFlag )
			->AddState( ID_TAG( PBS_DISABLED ), TextFlag )
			->AddState( ID_TAG( PBS_DEFAULTED ), TextFlag )
			->AddState( ID_TAG( PBS_DEFAULTED_ANIMATING ), TextFlag )
		;
		pClass->AddPart( ID_TAG( BP_RADIOBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( RBS_UNCHECKEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( RBS_UNCHECKEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( RBS_UNCHECKEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( RBS_UNCHECKEDDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( RBS_CHECKEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( RBS_CHECKEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( RBS_CHECKEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( RBS_CHECKEDDISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( BP_CHECKBOX ), SquareContentFlag )
			->AddState( ID_TAG( CBS_UNCHECKEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBS_UNCHECKEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( CBS_UNCHECKEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_UNCHECKEDDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_CHECKEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBS_CHECKEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( CBS_CHECKEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_CHECKEDDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_MIXEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBS_MIXEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( CBS_MIXEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_MIXEDDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_IMPLICITNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBS_IMPLICITHOT ), SquareContentFlag )
			->AddState( ID_TAG( CBS_IMPLICITPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_IMPLICITDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_EXCLUDEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBS_EXCLUDEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( CBS_EXCLUDEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBS_EXCLUDEDDISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( BP_GROUPBOX ) )
			->AddState( ID_TAG( GBS_NORMAL ) )
			->AddState( ID_TAG( GBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( BP_USERBUTTON ), TextFlag );
		pClass->AddPart( ID_TAG( BP_COMMANDLINK ), TextFlag )
			->AddState( ID_TAG( CMDLS_NORMAL ), TextFlag )
			->AddState( ID_TAG( CMDLS_HOT ), TextFlag )
			->AddState( ID_TAG( CMDLS_PRESSED ), TextFlag )
			->AddState( ID_TAG( CMDLS_DISABLED ), TextFlag )
			->AddState( ID_TAG( CMDLS_DEFAULTED ), TextFlag )
			->AddState( ID_TAG( CMDLS_DEFAULTED_ANIMATING ), TextFlag )
		;
		pClass->AddPart( ID_TAG( BP_COMMANDLINKGLYPH ), SquareContentFlag )
			->AddState( ID_TAG( CMDLGS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CMDLGS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CMDLGS_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CMDLGS_DISABLED ), SquareContentFlag )
			->AddState( ID_TAG( CMDLGS_DEFAULTED ), SquareContentFlag )
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

		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( CBXS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBXS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CBXS_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBXS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( CP_BACKGROUND ), TextFlag );
		pClass->AddPart( ID_TAG( CP_TRANSPARENTBACKGROUND ), TextFlag )
			->AddState( ID_TAG( CBTBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( CBTBS_HOT ), TextFlag )
			->AddState( ID_TAG( CBTBS_FOCUSED ), TextFlag )
			->AddState( ID_TAG( CBTBS_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( CP_BORDER ) )
			->AddState( ID_TAG( CBB_NORMAL ) )
			->AddState( ID_TAG( CBB_HOT ) )
			->AddState( ID_TAG( CBB_FOCUSED ) )
			->AddState( ID_TAG( CBB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( CP_READONLY ), TextFlag )
			->AddState( ID_TAG( CBRO_NORMAL ), TextFlag )
			->AddState( ID_TAG( CBRO_HOT ), TextFlag )
			->AddState( ID_TAG( CBRO_PRESSED ), TextFlag )
			->AddState( ID_TAG( CBRO_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTONRIGHT ), SquareContentFlag )
			->AddState( ID_TAG( CBXSR_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBXSR_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CBXSR_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBXSR_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( CP_DROPDOWNBUTTONLEFT ), SquareContentFlag )
			->AddState( ID_TAG( CBXSL_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CBXSL_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CBXSL_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( CBXSL_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( CP_CUEBANNER ), TextFlag )
			->AddState( ID_TAG( CBCB_NORMAL ), TextFlag )
			->AddState( ID_TAG( CBCB_HOT ), TextFlag )
			->AddState( ID_TAG( CBCB_PRESSED ), TextFlag )
			->AddState( ID_TAG( CBCB_DISABLED ), TextFlag )
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

		pClass->SetDeepFlags( TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"DATEPICKER", MediumRelevance );

		pClass->AddPart( ID_TAG( DP_DATETEXT ), TextFlag )
			->AddState( ID_TAG( DPDT_NORMAL ), TextFlag )
			->AddState( ID_TAG( DPDT_SELECTED ), TextFlag )
			->AddState( ID_TAG( DPDT_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( DP_DATEBORDER ) )
			->AddState( ID_TAG( DPDB_NORMAL ) )
			->AddState( ID_TAG( DPDB_HOT ) )
			->AddState( ID_TAG( DPDB_FOCUSED ) )
			->AddState( ID_TAG( DPDB_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( DP_SHOWCALENDARBUTTONRIGHT ) )
			->AddState( ID_TAG( DPSCBR_NORMAL ) )
			->AddState( ID_TAG( DPSCBR_HOT ), PreviewFlag )
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

		pClass->SetDeepFlags( SquareContentFlag );

		pClass->AddPart( ID_TAG( DD_IMAGEBG ), TextFlag );
		pClass->AddPart( ID_TAG( DD_TEXTBG ), TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"EDIT" );

		pClass->AddPart( ID_TAG( EP_EDITTEXT ), TextFlag )
			->AddState( ID_TAG( ETS_NORMAL ), TextFlag )
			->AddState( ID_TAG( ETS_HOT ), TextFlag )
			->AddState( ID_TAG( ETS_SELECTED ), TextFlag )
			->AddState( ID_TAG( ETS_DISABLED ), TextFlag )
			->AddState( ID_TAG( ETS_FOCUSED ), TextFlag )
			->AddState( ID_TAG( ETS_READONLY ), TextFlag )
			->AddState( ID_TAG( ETS_ASSIST ), TextFlag )
			->AddState( ID_TAG( ETS_CUEBANNER ), TextFlag )
		;
		pClass->AddPart( ID_TAG( EP_CARET ), TextFlag );
		pClass->AddPart( ID_TAG( EP_BACKGROUND ), TextFlag )
			->AddState( ID_TAG( EBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( EBS_HOT ), TextFlag )
			->AddState( ID_TAG( EBS_DISABLED ), TextFlag )
			->AddState( ID_TAG( EBS_FOCUSED ), TextFlag )
			->AddState( ID_TAG( EBS_READONLY ), TextFlag )
			->AddState( ID_TAG( EBS_ASSIST ), TextFlag )
		;
		pClass->AddPart( ID_TAG( EP_PASSWORD ) );
		pClass->AddPart( ID_TAG( EP_BACKGROUNDWITHBORDER ), TextFlag )
			->AddState( ID_TAG( EBWBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( EBWBS_HOT ), TextFlag )
			->AddState( ID_TAG( EBWBS_DISABLED ), TextFlag )
			->AddState( ID_TAG( EBWBS_FOCUSED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_NOSCROLL ), TextFlag )
			->AddState( ID_TAG( EPSN_NORMAL ), TextFlag )
			->AddState( ID_TAG( EPSN_HOT ), PreviewFlag | TextFlag )
			->AddState( ID_TAG( EPSN_FOCUSED ), TextFlag )
			->AddState( ID_TAG( EPSN_DISABLED ), TextFlag )
		;

		pClass->AddPart( ID_TAG( EP_EDITBORDER_HSCROLL ), TextFlag )
			->AddState( ID_TAG( EPSH_NORMAL ), TextFlag )
			->AddState( ID_TAG( EPSH_HOT ), TextFlag )
			->AddState( ID_TAG( EPSH_FOCUSED ), TextFlag )
			->AddState( ID_TAG( EPSH_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_VSCROLL ), TextFlag )
			->AddState( ID_TAG( EPSV_NORMAL ), TextFlag )
			->AddState( ID_TAG( EPSV_HOT ), TextFlag )
			->AddState( ID_TAG( EPSV_FOCUSED ), TextFlag )
			->AddState( ID_TAG( EPSV_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( EP_EDITBORDER_HVSCROLL ), TextFlag )
			->AddState( ID_TAG( EPSHV_NORMAL ), TextFlag )
			->AddState( ID_TAG( EPSHV_HOT ), TextFlag )
			->AddState( ID_TAG( EPSHV_FOCUSED ), TextFlag )
			->AddState( ID_TAG( EPSHV_DISABLED ), TextFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"EXPLORERBAR", MediumRelevance );

		pClass->AddPart( ID_TAG( EBP_HEADERBACKGROUND ), TextFlag );
		pClass->AddPart( ID_TAG( EBP_HEADERCLOSE ), SquareContentFlag )
			->AddState( ID_TAG( EBHC_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBHC_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBHC_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_HEADERPIN ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_SELECTEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_SELECTEDHOT ), SquareContentFlag )
			->AddState( ID_TAG( EBHP_SELECTEDPRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_IEBARMENU ), SquareContentFlag )
			->AddState( ID_TAG( EBM_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBM_HOT ), PreviewFlag | SquareContentFlag )
			->AddState( ID_TAG( EBM_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPBACKGROUND ), TextFlag );
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPCOLLAPSE ), SquareContentFlag )
			->AddState( ID_TAG( EBNGC_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBNGC_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBNGC_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPEXPAND ), SquareContentFlag )
			->AddState( ID_TAG( EBNGE_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBNGE_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBNGE_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_NORMALGROUPHEAD ), TextFlag );
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPBACKGROUND ), TextFlag );
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPCOLLAPSE ), SquareContentFlag )
			->AddState( ID_TAG( EBSGC_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBSGC_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBSGC_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPEXPAND ), SquareContentFlag )
			->AddState( ID_TAG( EBSGE_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( EBSGE_HOT ), SquareContentFlag )
			->AddState( ID_TAG( EBSGE_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( EBP_SPECIALGROUPHEAD ), TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"FLYOUT", MediumRelevance );

		pClass->AddPart( ID_TAG( FLYOUT_HEADER ), TextFlag );
		pClass->AddPart( ID_TAG( FLYOUT_BODY ), TextFlag )
			->AddState( ID_TAG( FBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( FBS_EMPHASIZED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( FLYOUT_LABEL ), TextFlag )
			->AddState( ID_TAG( FLS_NORMAL ), TextFlag )
			->AddState( ID_TAG( FLS_SELECTED ), TextFlag )
			->AddState( ID_TAG( FLS_EMPHASIZED ), TextFlag )
			->AddState( ID_TAG( FLS_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( FLYOUT_LINK ), TextFlag )
			->AddState( ID_TAG( FLYOUTLINK_NORMAL ), TextFlag )
			->AddState( ID_TAG( FLYOUTLINK_HOVER ), PreviewFlag | TextFlag )
		;
		pClass->AddPart( ID_TAG( FLYOUT_DIVIDER ) );
		pClass->AddPart( ID_TAG( FLYOUT_WINDOW ), TextFlag );
		pClass->AddPart( ID_TAG( FLYOUT_LINKAREA ), TextFlag );
		pClass->AddPart( ID_TAG( FLYOUT_LINKHEADER ), TextFlag )
			->AddState( ID_TAG( FLH_NORMAL ), TextFlag )
			->AddState( ID_TAG( FLH_HOVER ), TextFlag )
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

		pClass->SetDeepFlags( TextFlag );
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
			->AddState( ID_TAG( HILS_NORMAL ), PreviewFlag )
			->AddState( ID_TAG( HILS_HOT ) )
			->AddState( ID_TAG( HILS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( HP_HEADERITEMRIGHT ) )
			->AddState( ID_TAG( HIRS_NORMAL ) )
			->AddState( ID_TAG( HIRS_HOT ) )
			->AddState( ID_TAG( HIRS_PRESSED ) )
		;

		pClass->SetDeepFlags( TextFlag );

		pClass->AddPart( ID_TAG( HP_HEADERSORTARROW ), SquareContentFlag )
			->AddState( ID_TAG( HSAS_SORTEDUP ), SquareContentFlag )
			->AddState( ID_TAG( HSAS_SORTEDDOWN ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( HP_HEADERDROPDOWN ), SquareContentFlag )
			->AddState( ID_TAG( HDDS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HDDS_SOFTHOT ), SquareContentFlag )
			->AddState( ID_TAG( HDDS_HOT ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( HP_HEADERDROPDOWNFILTER ), SquareContentFlag )
			->AddState( ID_TAG( HDDFS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HDDFS_SOFTHOT ), SquareContentFlag )
			->AddState( ID_TAG( HDDFS_HOT ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( HP_HEADEROVERFLOW ), SquareContentFlag )
			->AddState( ID_TAG( HOFS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HOFS_HOT ), SquareContentFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"LISTBOX", MediumRelevance );

		pClass->AddPart( ID_TAG( LBCP_BORDER_HSCROLL ), 0, MediumRelevance )
			->AddState( ID_TAG( LBPSH_NORMAL ) )
			->AddState( ID_TAG( LBPSH_FOCUSED ) )
			->AddState( ID_TAG( LBPSH_HOT ) )
			->AddState( ID_TAG( LBPSH_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_HVSCROLL ), 0, MediumRelevance )
			->AddState( ID_TAG( LBPSHV_NORMAL ) )
			->AddState( ID_TAG( LBPSHV_FOCUSED ) )
			->AddState( ID_TAG( LBPSHV_HOT ) )
			->AddState( ID_TAG( LBPSHV_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_NOSCROLL ), 0, MediumRelevance )
			->AddState( ID_TAG( LBPSN_NORMAL ) )
			->AddState( ID_TAG( LBPSN_FOCUSED ) )
			->AddState( ID_TAG( LBPSN_HOT ) )
			->AddState( ID_TAG( LBPSN_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_BORDER_VSCROLL ), 0, MediumRelevance )
			->AddState( ID_TAG( LBPSV_NORMAL ) )
			->AddState( ID_TAG( LBPSV_FOCUSED ) )
			->AddState( ID_TAG( LBPSV_HOT ) )
			->AddState( ID_TAG( LBPSV_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( LBCP_ITEM ) )
			->AddState( ID_TAG( LBPSI_HOT ), PreviewFlag )
			->AddState( ID_TAG( LBPSI_HOTSELECTED ) )
			->AddState( ID_TAG( LBPSI_SELECTED ) )
			->AddState( ID_TAG( LBPSI_SELECTEDNOTFOCUS ) )
		;

		pClass->SetDeepFlags( TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"LISTVIEW" );

		pClass->AddPart( ID_TAG( LVP_LISTITEM ), TextFlag )
			->AddState( ID_TAG( LISS_NORMAL ), TextFlag )
			->AddState( ID_TAG( LISS_HOT ), TextFlag )
			->AddState( ID_TAG( LISS_SELECTED ), TextFlag )
			->AddState( ID_TAG( LISS_DISABLED ), TextFlag )
			->AddState( ID_TAG( LISS_SELECTEDNOTFOCUS ), TextFlag )
			->AddState( ID_TAG( LISS_HOTSELECTED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( LVP_LISTGROUP ), TextFlag, MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_LISTDETAIL ), TextFlag, MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_LISTSORTEDDETAIL ), TextFlag, MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_EMPTYTEXT ), TextFlag, MediumRelevance );
		pClass->AddPart( ID_TAG( LVP_GROUPHEADER ), TextFlag )
			->AddState( ID_TAG( LVGH_OPEN ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENHOT ), PreviewFlag | TextFlag )
			->AddState( ID_TAG( LVGH_OPENSELECTED ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENSELECTEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENSELECTEDNOTFOCUSED ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENSELECTEDNOTFOCUSEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENMIXEDSELECTION ), TextFlag )
			->AddState( ID_TAG( LVGH_OPENMIXEDSELECTIONHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSE ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSEHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSESELECTED ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDNOTFOCUSED ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSESELECTEDNOTFOCUSEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSEMIXEDSELECTION ), TextFlag )
			->AddState( ID_TAG( LVGH_CLOSEMIXEDSELECTIONHOT ), TextFlag )
		;
		pClass->AddPart( ID_TAG( LVP_GROUPHEADERLINE ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPEN ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENSELECTED ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDNOTFOCUSED ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENSELECTEDNOTFOCUSEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENMIXEDSELECTION ), TextFlag )
			->AddState( ID_TAG( LVGHL_OPENMIXEDSELECTIONHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSE ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSEHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSESELECTED ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDNOTFOCUSED ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSESELECTEDNOTFOCUSEDHOT ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSEMIXEDSELECTION ), TextFlag )
			->AddState( ID_TAG( LVGHL_CLOSEMIXEDSELECTIONHOT ), TextFlag )
		;
		pClass->AddPart( ID_TAG( LVP_EXPANDBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( LVEB_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( LVEB_HOVER ), SquareContentFlag )
			->AddState( ID_TAG( LVEB_PUSHED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( LVP_COLLAPSEBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( LVCB_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( LVCB_HOVER ), SquareContentFlag )
			->AddState( ID_TAG( LVCB_PUSHED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( LVP_COLUMNDETAIL ) );
	}

	{
		CThemeClass* pClass = AddClass( L"MENU" );

		pClass->AddPart( ID_TAG( MENU_MENUITEM_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUDROPDOWN_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUBARITEM_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_MENUBARDROPDOWN_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_CHEVRON_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_SEPARATOR_TMSCHEMA ), TextFlag, ObscureRelevance );
		pClass->AddPart( ID_TAG( MENU_BARBACKGROUND ), TextFlag )
			->AddState( ID_TAG( MB_ACTIVE ), PreviewFlag | TextFlag )
			->AddState( ID_TAG( MB_INACTIVE ), TextFlag )
		;
		pClass->AddPart( ID_TAG( MENU_BARITEM ), TextFlag )
			->AddState( ID_TAG( MBI_NORMAL ), TextFlag )
			->AddState( ID_TAG( MBI_HOT ), TextFlag )
			->AddState( ID_TAG( MBI_PUSHED ), TextFlag )
			->AddState( ID_TAG( MBI_DISABLED ), TextFlag )
			->AddState( ID_TAG( MBI_DISABLEDHOT ), TextFlag )
			->AddState( ID_TAG( MBI_DISABLEDPUSHED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPBACKGROUND ) );
		pClass->AddPart( ID_TAG( MENU_POPUPBORDERS ) );

		pClass->AddPart( ID_TAG( MENU_POPUPCHECK ), SquareContentFlag )
			->AddState( ID_TAG( MC_CHECKMARKNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MC_CHECKMARKDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( MC_BULLETNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MC_BULLETDISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPCHECKBACKGROUND ), SquareContentFlag )
			->AddState( ID_TAG( MCB_DISABLED ), SquareContentFlag )
			->AddState( ID_TAG( MCB_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MCB_BITMAP ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPGUTTER ) );
		pClass->AddPart( ID_TAG( MENU_POPUPITEM ), TextFlag )
			->AddState( ID_TAG( MPI_NORMAL ), TextFlag )
			->AddState( ID_TAG( MPI_HOT ), TextFlag )
			->AddState( ID_TAG( MPI_DISABLED ), TextFlag )
			->AddState( ID_TAG( MPI_DISABLEDHOT ), TextFlag )
		;
		pClass->AddPart( ID_TAG( MENU_POPUPSEPARATOR ) );

		pClass->AddPart( ID_TAG( MENU_POPUPSUBMENU ), SquareContentFlag )
			->AddState( ID_TAG( MSM_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MSM_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMCLOSE ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( MSYSC_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MSYSC_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMMAXIMIZE ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( MSYSMX_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MSYSMX_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMMINIMIZE ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( MSYSMN_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MSYSMN_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( MENU_SYSTEMRESTORE ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( MSYSR_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MSYSR_DISABLED ), SquareContentFlag )
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
			->AddState( ID_TAG( PBFS_NORMAL ), PreviewFlag )
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
		pClass->AddPart( ID_TAG( PP_PULSEOVERLAY ), PreviewFillBkFlag );
		pClass->AddPart( ID_TAG( PP_MOVEOVERLAY ) );
		pClass->AddPart( ID_TAG( PP_PULSEOVERLAYVERT ), PreviewFillBkFlag );
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
		pClass->AddPart( ID_TAG( RP_BAND ), TextFlag );
		pClass->AddPart( ID_TAG( RP_CHEVRON ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( CHEVS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CHEVS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CHEVS_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( RP_CHEVRONVERT ), SquareContentFlag, MediumRelevance )
			->AddState( ID_TAG( CHEVSV_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( CHEVSV_HOT ), SquareContentFlag )
			->AddState( ID_TAG( CHEVSV_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( RP_BACKGROUND ), PreviewFlag | TextFlag );
		pClass->AddPart( ID_TAG( RP_SPLITTER ), TextFlag )
			->AddState( ID_TAG( SPLITS_NORMAL ), TextFlag )
			->AddState( ID_TAG( SPLITS_HOT ), TextFlag )
			->AddState( ID_TAG( SPLITS_PRESSED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( RP_SPLITTERVERT ), TextFlag )
			->AddState( ID_TAG( SPLITSV_NORMAL ), TextFlag )
			->AddState( ID_TAG( SPLITSV_HOT ), TextFlag )
			->AddState( ID_TAG( SPLITSV_PRESSED ), TextFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"SCROLLBAR" );

		pClass->AddPart( ID_TAG( SBP_ARROWBTN ) )
			->AddState( ID_TAG( ABS_UPNORMAL ) )
			->AddState( ID_TAG( ABS_UPHOT ), PreviewFlag )
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

		pClass->SetDeepFlags( SquareContentFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"SEARCHEDITBOX", MediumRelevance );

		pClass->AddPart( SEBP_SEARCHEDITBOXTEXT, L"SEBP_SEARCHEDITBOXTEXT (Windows 7)", TextFlag )
			->AddState( ID_TAG( SEBTS_FORMATTED ), TextFlag )
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

		pClass->SetDeepFlags( SquareContentFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"STARTPANEL", MediumRelevance );

		pClass->AddPart( ID_TAG( SPP_USERPANE ), PreviewFlag | TextFlag );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMS ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSARROW ), SquareContentFlag )
			->AddState( ID_TAG( SPS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( SPS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( SPS_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( SPP_PROGLIST ) );
		pClass->AddPart( ID_TAG( SPP_PROGLISTSEPARATOR ) );
		pClass->AddPart( ID_TAG( SPP_PLACESLIST ), TextFlag );
		pClass->AddPart( ID_TAG( SPP_PLACESLISTSEPARATOR ) );
		pClass->AddPart( ID_TAG( SPP_LOGOFF ), TextFlag );
		pClass->AddPart( ID_TAG( SPP_LOGOFFBUTTONS ) )
			->AddState( ID_TAG( SPLS_NORMAL ) )
			->AddState( ID_TAG( SPLS_HOT ) )
			->AddState( ID_TAG( SPLS_PRESSED ) )
		;
		pClass->AddPart( ID_TAG( SPP_USERPICTURE ) );
		pClass->AddPart( ID_TAG( SPP_PREVIEW ) );

		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSTAB ), TextFlag )
			->AddState( ID_TAG( SPSB_NORMAL ), TextFlag )
			->AddState( ID_TAG( SPSB_HOT ), TextFlag )
			->AddState( ID_TAG( SPSB_PRESSED ), PreviewShallowFlag | TextFlag )
		;
		pClass->AddPart( ID_TAG( SPP_NSCHOST ) );
		pClass->AddPart( ID_TAG( SPP_SOFTWAREEXPLORER ), TextFlag );
		pClass->AddPart( ID_TAG( SPP_OPENBOX ) );
		pClass->AddPart( ID_TAG( SPP_SEARCHVIEW ) );
		pClass->AddPart( ID_TAG( SPP_MOREPROGRAMSARROWBACK ), SquareContentFlag );
		pClass->AddPart( ID_TAG( SPP_TOPMATCH ) );
		pClass->AddPart( ID_TAG( SPP_LOGOFFSPLITBUTTONDROPDOWN ), SquareContentFlag )
			->AddState( ID_TAG( SPLS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( SPLS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( SPLS_PRESSED ), SquareContentFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"STATUS", MediumRelevance );

		pClass->AddPart( ID_TAG( SP_PANE ), TextFlag );
		pClass->AddPart( ID_TAG( SP_GRIPPERPANE ), TextFlag );
		pClass->AddPart( ID_TAG( SP_GRIPPER ), 0, MediumRelevance );
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
		pClass->AddPart( ID_TAG( TABP_PANE ), 0, MediumRelevance );
		pClass->AddPart( ID_TAG( TABP_BODY ), 0, MediumRelevance );
		pClass->AddPart( ID_TAG( TABP_AEROWIZARDBODY ), 0, MediumRelevance );

		pClass->SetDeepFlags( TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"TASKBAND", MediumRelevance );

		pClass->AddPart( ID_TAG( TDP_GROUPCOUNT ), TextFlag );
		pClass->AddPart( ID_TAG( TDP_FLASHBUTTON ), TextFlag );
		pClass->AddPart( ID_TAG( TDP_FLASHBUTTONGROUPMENU ), 0, MediumRelevance );
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

		pClass->AddPart( ID_TAG( TDLG_PRIMARYPANEL ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_MAININSTRUCTIONPANE ), PreviewFlag | TextFlag );
		pClass->AddPart( ID_TAG( TDLG_MAINICON ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_CONTENTPANE ), TextFlag )
			->AddState( ID_TAG( TDLGCPS_STANDALONE ), TextFlag )
		;
		pClass->AddPart( ID_TAG( TDLG_CONTENTICON ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_EXPANDEDCONTENT ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_COMMANDLINKPANE ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_SECONDARYPANEL ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_CONTROLPANE ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_BUTTONSECTION ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_BUTTONWRAPPER ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_EXPANDOTEXT ), TextFlag );

		pClass->AddPart( ID_TAG( TDLG_EXPANDOBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_HOVER ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDNORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDHOVER ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDPRESSED ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_NORMALDISABLED ), SquareContentFlag )
			->AddState( ID_TAG( TDLGEBS_EXPANDEDDISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TDLG_VERIFICATIONTEXT ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTEPANE ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTEAREA ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_FOOTNOTESEPARATOR ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_EXPANDEDFOOTERAREA ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_PROGRESSBAR ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_IMAGEALIGNMENT ), TextFlag );
		pClass->AddPart( ID_TAG( TDLG_RADIOBUTTONPANE ), TextFlag );
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
			->AddState( ID_TAG( TS_HYPERLINK_PRESSED ), PreviewFlag )
			->AddState( ID_TAG( TS_HYPERLINK_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( TEXT_EXPANDED ) );
		pClass->AddPart( ID_TAG( TEXT_LABEL ) );
		pClass->AddPart( ID_TAG( TEXT_CONTROLLABEL ) )
			->AddState( ID_TAG( TS_CONTROLLABEL_NORMAL ) )
			->AddState( ID_TAG( TS_CONTROLLABEL_DISABLED ) )
		;

		pClass->SetDeepFlags( TextFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"TOOLBAR" );
		CThemePart* pPart;

		pPart = pClass->AddPart( ID_TAG( TP_BUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ), PreviewFlag )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pPart->SetDeepFlags( TextFlag );

		pPart = pClass->AddPart( ID_TAG( TP_DROPDOWNBUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pPart->SetDeepFlags( TextFlag );

		pPart = pClass->AddPart( ID_TAG( TP_SPLITBUTTON ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pPart->SetDeepFlags( TextFlag );

		pPart = pClass->AddPart( ID_TAG( TP_SPLITBUTTONDROPDOWN ) )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pPart->SetDeepFlags( SquareContentFlag );

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
		pPart = pClass->AddPart( TP_DROPDOWNBUTTONGLYPH, L"TP_DROPDOWNBUTTONGLYPH (Windows 7)" )
			->AddState( ID_TAG( TS_NORMAL ) )
			->AddState( ID_TAG( TS_HOT ) )
			->AddState( ID_TAG( TS_PRESSED ) )
			->AddState( ID_TAG( TS_DISABLED ) )
			->AddState( ID_TAG( TS_CHECKED ) )
			->AddState( ID_TAG( TS_HOTCHECKED ) )
			->AddState( ID_TAG( TS_NEARHOT ) )
			->AddState( ID_TAG( TS_OTHERSIDEHOT ) )
		;
		pPart->SetDeepFlags( SquareContentFlag );
	}

	{
		CThemeClass* pClass = AddClass( L"TOOLTIP" );

		pClass->AddPart( ID_TAG( TTP_STANDARD ), TextFlag )
			->AddState( ID_TAG( TTSS_NORMAL ), TextFlag )
			->AddState( ID_TAG( TTSS_LINK ), TextFlag )
		;
		pClass->AddPart( ID_TAG( TTP_STANDARDTITLE ), TextFlag )
			->AddState( ID_TAG( TTSS_NORMAL ), TextFlag )
			->AddState( ID_TAG( TTSS_LINK ), TextFlag )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOON ), TextFlag )
			->AddState( ID_TAG( TTBS_NORMAL ), TextFlag )
			->AddState( ID_TAG( TTBS_LINK ), TextFlag )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOONTITLE ), TextFlag );

		pClass->AddPart( ID_TAG( TTP_CLOSE ), SquareContentFlag )
			->AddState( ID_TAG( TTCS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TTCS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( TTCS_PRESSED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TTP_BALLOONSTEM ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGUPLEFTWALL ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGUPCENTERED ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGUPRIGHTWALL ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNRIGHTWALL ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNCENTERED ), SquareContentFlag )
			->AddState( ID_TAG( TTBSS_POINTINGDOWNLEFTWALL ), SquareContentFlag )
		;
		pClass->AddPart( TTP_WRENCH, L"TTP_WRENCH (Windows 7)", SquareContentFlag )
			->AddState( ID_TAG( TTWS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TTWS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( TTWS_PRESSED ), SquareContentFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TRACKBAR" );

		pClass->AddPart( ID_TAG( TKP_TRACK ), SquareContentFlag )
			->AddState( ID_TAG( TRS_NORMAL ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TKP_TRACKVERT ), SquareContentFlag )
			->AddState( ID_TAG( TRVS_NORMAL ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TKP_THUMB ), SquareContentFlag )
			->AddState( ID_TAG( TUS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TUS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( TUS_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( TUS_FOCUSED ), SquareContentFlag )
			->AddState( ID_TAG( TUS_DISABLED ), SquareContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_THUMBBOTTOM ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUBS_NORMAL ), PreviewFlag | ShrinkFitContentFlag )
			->AddState( ID_TAG( TUBS_HOT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUBS_PRESSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUBS_FOCUSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUBS_DISABLED ), ShrinkFitContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_THUMBTOP ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUTS_NORMAL ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUTS_HOT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUTS_PRESSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUTS_FOCUSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUTS_DISABLED ), ShrinkFitContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_THUMBVERT ), SquareContentFlag )
			->AddState( ID_TAG( TUVS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( TUVS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( TUVS_PRESSED ), SquareContentFlag )
			->AddState( ID_TAG( TUVS_FOCUSED ), SquareContentFlag )
			->AddState( ID_TAG( TUVS_DISABLED ), SquareContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_THUMBLEFT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVLS_NORMAL ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVLS_HOT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVLS_PRESSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVLS_FOCUSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVLS_DISABLED ), ShrinkFitContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_THUMBRIGHT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVRS_NORMAL ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVRS_HOT ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVRS_PRESSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVRS_FOCUSED ), ShrinkFitContentFlag )
			->AddState( ID_TAG( TUVRS_DISABLED ), ShrinkFitContentFlag )
		;

		pClass->AddPart( ID_TAG( TKP_TICS ), SquareContentFlag )
			->AddState( ID_TAG( TSS_NORMAL ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TKP_TICSVERT ), SquareContentFlag )
			->AddState( ID_TAG( TSVS_NORMAL ), SquareContentFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"TRAYNOTIFY" );		// ObscureRelevance

		pClass->AddPart( ID_TAG( TNP_BACKGROUND ) );
		pClass->AddPart( ID_TAG( TNP_ANIMBACKGROUND ) );
	}

	{
		CThemeClass* pClass = AddClass( L"TREEVIEW" );

		pClass->AddPart( ID_TAG( TVP_GLYPH ), SquareContentFlag )
			->AddState( ID_TAG( GLPS_CLOSED ), SquareContentFlag )
			->AddState( ID_TAG( GLPS_OPENED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( TVP_TREEITEM ), TextFlag )
			->AddState( ID_TAG( TREIS_NORMAL ), TextFlag )
			->AddState( ID_TAG( TREIS_HOT ), TextFlag )
			->AddState( ID_TAG( TREIS_SELECTED ), TextFlag )
			->AddState( ID_TAG( TREIS_DISABLED ), TextFlag )
			->AddState( ID_TAG( TREIS_SELECTEDNOTFOCUS ), TextFlag )
			->AddState( ID_TAG( TREIS_HOTSELECTED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( TVP_BRANCH ), TextFlag );
		pClass->AddPart( ID_TAG( TVP_HOTGLYPH ), SquareContentFlag )
			->AddState( ID_TAG( HGLPS_CLOSED ), SquareContentFlag )
			->AddState( ID_TAG( HGLPS_OPENED ), SquareContentFlag )
		;
	}

	{
		CThemeClass* pClass = AddClass( L"WINDOW" );

		pClass->AddPart( ID_TAG( WP_CAPTION ), TextFlag )
			->AddState( ID_TAG( CS_ACTIVE ), TextFlag )
			->AddState( ID_TAG( CS_INACTIVE ), TextFlag )
			->AddState( ID_TAG( CS_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( WP_SMALLCAPTION ), TextFlag );
		pClass->AddPart( ID_TAG( WP_MINCAPTION ), TextFlag )
			->AddState( ID_TAG( MNCS_ACTIVE ), TextFlag )
			->AddState( ID_TAG( MNCS_INACTIVE ), TextFlag )
			->AddState( ID_TAG( MNCS_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( WP_SMALLMINCAPTION ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_MAXCAPTION ), TextFlag )
			->AddState( ID_TAG( MXCS_ACTIVE ), TextFlag )
			->AddState( ID_TAG( MXCS_INACTIVE ), TextFlag )
			->AddState( ID_TAG( MXCS_DISABLED ), TextFlag )
		;
		pClass->AddPart( ID_TAG( WP_SMALLMAXCAPTION ) );		// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_FRAMELEFT ), ShrinkFitContentFlag );
		pClass->AddPart( ID_TAG( WP_FRAMERIGHT ), ShrinkFitContentFlag );
		pClass->AddPart( ID_TAG( WP_FRAMEBOTTOM ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMELEFT ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMERIGHT ) );
		pClass->AddPart( ID_TAG( WP_SMALLFRAMEBOTTOM ) );
		pClass->AddPart( ID_TAG( WP_SYSBUTTON ), SquareContentFlag )				// ObscureRelevance
			->AddState( ID_TAG( SBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( SBS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( SBS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( SBS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_MDISYSBUTTON ), SquareContentFlag );			// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_MINBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( MINBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MINBS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( MINBS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( MINBS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_MDIMINBUTTON ), SquareContentFlag );
		pClass->AddPart( ID_TAG( WP_MAXBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( MAXBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( MAXBS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( MAXBS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( MAXBS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_CLOSEBUTTON ) )
			->AddState( ID_TAG( CBS_NORMAL ) )
			->AddState( ID_TAG( CBS_HOT ) )
			->AddState( ID_TAG( CBS_PUSHED ) )
			->AddState( ID_TAG( CBS_DISABLED ) )
		;
		pClass->AddPart( ID_TAG( WP_SMALLCLOSEBUTTON ) );
		pClass->AddPart( ID_TAG( WP_MDICLOSEBUTTON ) );
		pClass->AddPart( ID_TAG( WP_RESTOREBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( RBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( RBS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( RBS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( RBS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_MDIRESTOREBUTTON ), SquareContentFlag );
		pClass->AddPart( ID_TAG( WP_HELPBUTTON ), SquareContentFlag )
			->AddState( ID_TAG( HBS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HBS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( HBS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( HBS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_MDIHELPBUTTON ), SquareContentFlag );			// ObscureRelevance
		pClass->AddPart( ID_TAG( WP_HORZSCROLL ), SquareContentFlag )
			->AddState( ID_TAG( HSS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HSS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( HSS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( HSS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_HORZTHUMB ), SquareContentFlag )				// ObscureRelevance
			->AddState( ID_TAG( HTS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( HTS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( HTS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( HTS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_VERTSCROLL ), SquareContentFlag )				// ObscureRelevance
			->AddState( ID_TAG( VSS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( VSS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( VSS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( VSS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_VERTTHUMB ), SquareContentFlag )				// ObscureRelevance
			->AddState( ID_TAG( VTS_NORMAL ), SquareContentFlag )
			->AddState( ID_TAG( VTS_HOT ), SquareContentFlag )
			->AddState( ID_TAG( VTS_PUSHED ), SquareContentFlag )
			->AddState( ID_TAG( VTS_DISABLED ), SquareContentFlag )
		;
		pClass->AddPart( ID_TAG( WP_DIALOG ), TextFlag );
		pClass->AddPart( ID_TAG( WP_CAPTIONSIZINGTEMPLATE ), 0, MediumRelevance );
		pClass->AddPart( ID_TAG( WP_SMALLCAPTIONSIZINGTEMPLATE ), 0, MediumRelevance );
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
