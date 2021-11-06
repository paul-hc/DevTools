
#include "stdafx.h"
#include "VisualThemeFallback.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CVisualThemeFallback::CVisualThemeFallback( void )
{
	RegisterButtonClass();
	RegisterMenuClass();
	RegisterComboClass();
	RegisterScrollClass();
	RegisterCaptionClass();
}

CVisualThemeFallback::~CVisualThemeFallback()
{
}

CVisualThemeFallback& CVisualThemeFallback::Instance( void )
{
	static CVisualThemeFallback instance;
	return instance;
}

bool CVisualThemeFallback::DrawBackground( const wchar_t* pClass, int partId, int stateId, HDC hdc, const RECT& rect )
{
	ClassKey key( pClass, partId );
	str::ToUpper( key.first );

	stdext::hash_map< ClassKey, CCtrlStates >::const_iterator itFound = m_classToStateMap.find( key );
	if ( itFound != m_classToStateMap.end() )
	{
		const StateSet& stateSet = itFound->second.m_stateSet;
		StateSet::const_iterator itState = utl::FindPair( stateSet.begin(), stateSet.end(), stateId );
		if ( itState != stateSet.end() )
		{
			CtrlType ctrlType = itFound->second.m_ctrlType;
			State state = itState->second;

			return ::DrawFrameControl( hdc, const_cast<RECT*>( &rect ), ctrlType, state ) != FALSE;
		}
	}

	stdext::hash_map< ClassKey, CustomDrawBkFunc >::const_iterator itCustom = m_classToCustomBkMap.find( key );
	if ( itCustom != m_classToCustomBkMap.end() )
		return itCustom->second( stateId, hdc, rect );

	return false;
}

bool CVisualThemeFallback::DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags )
{
	return ::DrawEdge( hdc, const_cast<RECT*>( &rect ), edge, flags ) != FALSE;
}

CVisualThemeFallback::StateSet* CVisualThemeFallback::AddClassPart( const ClassKey& classKey, CtrlType ctrlType )
{
	CCtrlStates* pNewCtrlStates = &m_classToStateMap[ classKey ];
	pNewCtrlStates->m_ctrlType = ctrlType;
	return &pNewCtrlStates->m_stateSet;
}

bool CVisualThemeFallback::CustomDrawBk_MenuBackground( int stateId, HDC hdc, const RECT& rect )
{
	stateId;
	::FillRect( hdc, &rect, GetSysColorBrush( COLOR_MENU ) );
	return true;
}

bool CVisualThemeFallback::CustomDrawBk_MenuItem( int stateId, HDC hdc, const RECT& rect )
{
	int sysColor = COLOR_MENU;
	switch ( stateId )
	{
		case MPI_HOT:
			sysColor = COLOR_HIGHLIGHT;
			break;
		case MPI_DISABLED:
		case MPI_DISABLEDHOT:
			sysColor = COLOR_HIGHLIGHT;
			break;
	}
	::FillRect( hdc, &rect, GetSysColorBrush( sysColor ) );
	return true;
}

bool CVisualThemeFallback::CustomDrawBk_MenuChecked( int stateId, HDC hdc, const RECT& rect )
{
	return ::DrawEdge( hdc, const_cast<RECT*>( &rect ), stateId != MCB_DISABLED ? BDR_SUNKENOUTER : BDR_SUNKENINNER, BF_RECT | BF_SOFT ) != FALSE;
}

void CVisualThemeFallback::RegisterButtonClass( void )
{
	const wchar_t buttonClass[] = L"BUTTON";
	StateSet* pStateSet;

	pStateSet = AddClassPart( ClassKey( buttonClass, vt::BP_CHECKBOX ), DFC_BUTTON );
	pStateSet->push_back( StatePair( vt::CBS_UNCHECKEDNORMAL, DFCS_BUTTONCHECK ) );
	pStateSet->push_back( StatePair( vt::CBS_UNCHECKEDHOT, DFCS_BUTTONCHECK | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBS_UNCHECKEDPRESSED, DFCS_BUTTONCHECK | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBS_UNCHECKEDDISABLED, DFCS_BUTTONCHECK | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::CBS_CHECKEDNORMAL, DFCS_BUTTONCHECK | DFCS_CHECKED ) );
	pStateSet->push_back( StatePair( vt::CBS_CHECKEDHOT, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBS_CHECKEDPRESSED, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBS_CHECKEDDISABLED, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::CBS_MIXEDNORMAL, DFCS_BUTTON3STATE ) );
	pStateSet->push_back( StatePair( vt::CBS_MIXEDHOT, DFCS_BUTTON3STATE | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBS_MIXEDPRESSED, DFCS_BUTTON3STATE | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBS_MIXEDDISABLED, DFCS_BUTTON3STATE | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( buttonClass, vt::BP_PUSHBUTTON ), DFC_BUTTON );
	pStateSet->push_back( StatePair( vt::PBS_NORMAL, DFCS_BUTTONPUSH ) );
	pStateSet->push_back( StatePair( vt::PBS_HOT, DFCS_BUTTONPUSH | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::PBS_PRESSED, DFCS_BUTTONPUSH | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::PBS_DISABLED, DFCS_BUTTONPUSH | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::PBS_DEFAULTED, DFCS_BUTTONPUSH ) );
	pStateSet->push_back( StatePair( vt::PBS_DEFAULTED_ANIMATING, DFCS_BUTTONPUSH ) );

	pStateSet = AddClassPart( ClassKey( buttonClass, vt::BP_RADIOBUTTON ), DFC_BUTTON );
	pStateSet->push_back( StatePair( vt::RBS_UNCHECKEDNORMAL, DFCS_BUTTONRADIO ) );
	pStateSet->push_back( StatePair( vt::RBS_UNCHECKEDHOT, DFCS_BUTTONRADIO | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::RBS_UNCHECKEDPRESSED, DFCS_BUTTONRADIO | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::RBS_UNCHECKEDDISABLED, DFCS_BUTTONRADIO | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::RBS_CHECKEDNORMAL, DFCS_BUTTONRADIO | DFCS_CHECKED ) );
	pStateSet->push_back( StatePair( vt::RBS_CHECKEDHOT, DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::RBS_CHECKEDPRESSED, DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::RBS_CHECKEDDISABLED, DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_INACTIVE ) );
}

void CVisualThemeFallback::RegisterMenuClass( void )
{
	const wchar_t menuClass[] = L"MENU";
	StateSet* pStateSet;

	pStateSet = AddClassPart( ClassKey( menuClass, vt::MENU_POPUPCHECK ), DFC_POPUPMENU );
	pStateSet->push_back( StatePair( vt::MC_CHECKMARKNORMAL, DFCS_MENUCHECK ) );
	pStateSet->push_back( StatePair( vt::MC_CHECKMARKDISABLED, DFCS_MENUCHECK | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::MC_BULLETNORMAL, DFCS_MENUBULLET ) );
	pStateSet->push_back( StatePair( vt::MC_BULLETDISABLED, DFCS_MENUBULLET | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( menuClass, vt::MENU_POPUPSUBMENU ), DFC_POPUPMENU );
	pStateSet->push_back( StatePair( vt::MSM_NORMAL, DFCS_MENUARROW | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::MSM_DISABLED, DFCS_MENUARROW | DFCS_INACTIVE ) );

	// custom menu backgrounds
	m_classToCustomBkMap[ ClassKey( menuClass, vt::MENU_POPUPBACKGROUND ) ] = &CustomDrawBk_MenuBackground;
	m_classToCustomBkMap[ ClassKey( menuClass, vt::MENU_POPUPITEM ) ] = &CustomDrawBk_MenuItem;
	m_classToCustomBkMap[ ClassKey( menuClass, vt::MENU_POPUPCHECKBACKGROUND ) ] = &CustomDrawBk_MenuChecked;
}

void CVisualThemeFallback::RegisterComboClass( void )
{
	const wchar_t comboClass[] = L"COMBOBOX";
	StateSet* pStateSet;

	pStateSet = AddClassPart( ClassKey( comboClass, vt::CP_DROPDOWNBUTTON ), DFC_SCROLL );
	pStateSet->push_back( StatePair( vt::CBXS_NORMAL, DFCS_SCROLLCOMBOBOX ) );
	pStateSet->push_back( StatePair( vt::CBXS_HOT, DFCS_SCROLLCOMBOBOX | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBXS_PRESSED, DFCS_SCROLLCOMBOBOX | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBXS_DISABLED, DFCS_SCROLLCOMBOBOX | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( comboClass, vt::CP_DROPDOWNBUTTONLEFT ), DFC_SCROLL );
	pStateSet->push_back( StatePair( vt::CBXSL_NORMAL, DFCS_SCROLLCOMBOBOX ) );
	pStateSet->push_back( StatePair( vt::CBXSL_HOT, DFCS_SCROLLCOMBOBOX | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBXSL_PRESSED, DFCS_SCROLLCOMBOBOX | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBXSL_DISABLED, DFCS_SCROLLCOMBOBOX | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( comboClass, vt::CP_DROPDOWNBUTTONRIGHT ), DFC_SCROLL );
	pStateSet->push_back( StatePair( vt::CBXSR_NORMAL, DFCS_SCROLLCOMBOBOX ) );
	pStateSet->push_back( StatePair( vt::CBXSR_HOT, DFCS_SCROLLCOMBOBOX | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBXSR_PRESSED, DFCS_SCROLLCOMBOBOX | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBXSR_DISABLED, DFCS_SCROLLCOMBOBOX | DFCS_INACTIVE ) );
}

void CVisualThemeFallback::RegisterScrollClass( void )
{
	const wchar_t scrollClass[] = L"SCROLLBAR";
	StateSet* pStateSet;

	pStateSet = AddClassPart( ClassKey( scrollClass, vt::SBP_ARROWBTN ), DFC_SCROLL );
	pStateSet->push_back( StatePair( vt::ABS_UPNORMAL, DFCS_SCROLLUP ) );
	pStateSet->push_back( StatePair( vt::ABS_UPHOT, DFCS_SCROLLUP | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_UPPRESSED, DFCS_SCROLLUP | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::ABS_UPDISABLED, DFCS_SCROLLUP | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::ABS_UPHOVER, DFCS_SCROLLUP | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_DOWNNORMAL, DFCS_SCROLLDOWN ) );
	pStateSet->push_back( StatePair( vt::ABS_DOWNHOT, DFCS_SCROLLDOWN | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_DOWNPRESSED, DFCS_SCROLLDOWN | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::ABS_DOWNDISABLED, DFCS_SCROLLDOWN | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::ABS_DOWNHOVER, DFCS_SCROLLDOWN | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_LEFTNORMAL, DFCS_SCROLLLEFT ) );
	pStateSet->push_back( StatePair( vt::ABS_LEFTHOT, DFCS_SCROLLLEFT | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_LEFTPRESSED, DFCS_SCROLLLEFT | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::ABS_LEFTDISABLED, DFCS_SCROLLLEFT | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::ABS_LEFTHOVER, DFCS_SCROLLLEFT | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_RIGHTNORMAL, DFCS_SCROLLRIGHT ) );
	pStateSet->push_back( StatePair( vt::ABS_RIGHTHOT, DFCS_SCROLLRIGHT | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::ABS_RIGHTPRESSED, DFCS_SCROLLRIGHT | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::ABS_RIGHTDISABLED, DFCS_SCROLLRIGHT | DFCS_INACTIVE ) );
	pStateSet->push_back( StatePair( vt::ABS_RIGHTHOVER, DFCS_SCROLLRIGHT | DFCS_HOT ) );

	pStateSet = AddClassPart( ClassKey( scrollClass, vt::SBP_SIZEBOX ), DFC_SCROLL );
	pStateSet->push_back( StatePair( vt::SZB_RIGHTALIGN, DFCS_SCROLLSIZEGRIP ) );
	pStateSet->push_back( StatePair( vt::SZB_LEFTALIGN, DFCS_SCROLLSIZEGRIPRIGHT ) );
	pStateSet->push_back( StatePair( vt::SZB_HALFBOTTOMRIGHTALIGN, DFCS_SCROLLSIZEGRIP ) );
	pStateSet->push_back( StatePair( vt::SZB_HALFBOTTOMLEFTALIGN, DFCS_SCROLLSIZEGRIPRIGHT ) );
}

void CVisualThemeFallback::RegisterCaptionClass( void )
{
	const wchar_t windowClass[] = L"WINDOW";
	StateSet* pStateSet;

	pStateSet = AddClassPart( ClassKey( windowClass, vt::WP_CLOSEBUTTON ), DFC_CAPTION );
	pStateSet->push_back( StatePair( vt::CBS_NORMAL, DFCS_CAPTIONCLOSE ) );
	pStateSet->push_back( StatePair( vt::CBS_HOT, DFCS_CAPTIONCLOSE | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::CBS_PUSHED, DFCS_CAPTIONCLOSE | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::CBS_DISABLED, DFCS_CAPTIONCLOSE | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( windowClass, vt::WP_HELPBUTTON ), DFC_CAPTION );
	pStateSet->push_back( StatePair( vt::HBS_NORMAL, DFCS_CAPTIONHELP ) );
	pStateSet->push_back( StatePair( vt::HBS_HOT, DFCS_CAPTIONHELP | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::HBS_PUSHED, DFCS_CAPTIONHELP | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::HBS_DISABLED, DFCS_CAPTIONHELP | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( windowClass, vt::WP_MAXBUTTON ), DFC_CAPTION );
	pStateSet->push_back( StatePair( vt::MAXBS_NORMAL, DFCS_CAPTIONMAX ) );
	pStateSet->push_back( StatePair( vt::MAXBS_HOT, DFCS_CAPTIONMAX | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::MAXBS_PUSHED, DFCS_CAPTIONMAX | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::MAXBS_DISABLED, DFCS_CAPTIONMAX | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( windowClass, vt::WP_MINBUTTON ), DFC_CAPTION );
	pStateSet->push_back( StatePair( vt::MINBS_NORMAL, DFCS_CAPTIONMIN ) );
	pStateSet->push_back( StatePair( vt::MINBS_HOT, DFCS_CAPTIONMIN | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::MINBS_PUSHED, DFCS_CAPTIONMIN | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::MINBS_DISABLED, DFCS_CAPTIONMIN | DFCS_INACTIVE ) );

	pStateSet = AddClassPart( ClassKey( windowClass, vt::WP_RESTOREBUTTON ), DFC_CAPTION );
	pStateSet->push_back( StatePair( vt::RBS_NORMAL, DFCS_CAPTIONRESTORE ) );
	pStateSet->push_back( StatePair( vt::RBS_HOT, DFCS_CAPTIONRESTORE | DFCS_HOT ) );
	pStateSet->push_back( StatePair( vt::RBS_PUSHED, DFCS_CAPTIONRESTORE | DFCS_PUSHED ) );
	pStateSet->push_back( StatePair( vt::RBS_DISABLED, DFCS_CAPTIONRESTORE | DFCS_INACTIVE ) );
}
