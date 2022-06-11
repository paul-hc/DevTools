
#include "stdafx.h"
#include "FileListDialog.h"
#include "FlagStrip.h"
#include "TestDialog.h"
#include "utl/ContainerOwnership.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/UI/Color.h"
#include "utl/UI/ReportListCustomDraw.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


namespace hlp
{
	struct ResetDest
	{
		void operator()( std::pair<const fs::CFileState, fs::CFileState>& rFileStatePair )
		{
			rFileStatePair.second = rFileStatePair.first;
		}
	};


	fs::CFileState MakeFileState( const std::tstring& nameExt, const std::tstring& attrsText, const std::tstring& creationDateText )
	{
		static const fs::CPath s_dirPath( _T("C:\\my\\data") );

		fs::CFileState fileState;
		fileState.m_fullPath = s_dirPath / nameExt;
		fileState.m_attributes = static_cast<BYTE>( fmt::ParseFileAttributes( attrsText, false ) );
		fileState.m_creationTime = time_utl::ParseTimestamp( creationDateText );
		return fileState;
	}

	void RegisterNoDiffs( fs::TFileStatePairMap& rStatePairs, const fs::CFileState& fileState )
	{
		rStatePairs[ fileState ] = fileState;
	}

	const fs::TFileStatePairMap& GetStatePairs( bool useDiffsMode )
	{
		static fs::TFileStatePairMap s_statePairs, s_diffStatePairs;
		if ( s_diffStatePairs.empty() )
		{
			RegisterNoDiffs( s_diffStatePairs, MakeFileState( _T("A Rain Song.gp"), _T("A"), _T("09-12-2012 14:17:13") ) );
			RegisterNoDiffs( s_diffStatePairs, MakeFileState( _T("All Along The Watchtower.gp"), _T("A"), _T("23-01-2009 04:35:30") ) );

			s_diffStatePairs[ MakeFileState( _T("Back in Black.gp"), _T("A"), _T("17-10-2005 22:33:38") ) ] =
				MakeFileState( _T("Back with BLACK.gp"), _T("A"), _T("17-10-2005 23:17:38") );

			s_diffStatePairs[ MakeFileState( _T("BaseMainDialog.cpp"), _T("RA"), _T("03-10-2008 14:59:48") ) ] =
				MakeFileState( _T("BasicallyMainMonolog.cpp"), _T("RA"), _T("03-10-2019 14:59:48") );

			s_diffStatePairs[ MakeFileState( _T("BaseMainDialog.h"), _T("RHSDA"), _T("06-03-2010 23:07:46") ) ] =
				MakeFileState( _T("BasicallyMainMonolog.h"), _T("RHSVA"), _T("10-12-2010 23:07:46") );

			s_diffStatePairs[ MakeFileState( _T("Canon Rock (band).gp"), _T("SDA"), _T("23-01-2009 04:59:26") ) ] =
				MakeFileState( _T("Canon Rock (group).gp"), _T("SDA"), _T("23-01-2009 05:59:00") );

			s_diffStatePairs[ MakeFileState( _T("Deep Purple - Lazy.gp"), _T("HSDA"), _T("10-06-2013 11:53:54") ) ] =
				MakeFileState( _T("Deep Red - Lazy.gp"), _T("HA"), _T("10-11-2013 11:53:00") );

			s_diffStatePairs[ MakeFileState( _T("Focus - Hocus Pocus.gp"), _T("RA"), _T("27-08-2017 11:43:09") ) ] =
				MakeFileState( _T("Focus - HOCUS POCUS.gp"), _T("HSA"), _T("27-08-2017 11:43:09") );

			s_diffStatePairs[ MakeFileState( _T("Little Wing.gp"), _T("RHSDA"), _T("25-07-2009 06:06:36") ) ] =
				MakeFileState( _T("Little More Wing.gp"), _T("HSA"), _T("25-07-1989 06:23:36") );

			s_statePairs = s_diffStatePairs;
			std::for_each( s_statePairs.begin(), s_statePairs.end(), ResetDest() );
		}

		return useDiffsMode ? s_diffStatePairs : s_statePairs;
	}

	const std::tstring& GetNotesAt( size_t pos )
	{
		static std::vector< std::tstring > s_noteItems;
		if ( s_noteItems.empty() )
		{
			s_noteItems.push_back( _T("Empire (2003)") );
			s_noteItems.push_back( _T("American Colossus (2004)") );
			s_noteItems.push_back( _T("The War of the World (2006)") );
			s_noteItems.push_back( _T("The Ascent of Money (2008)") );
			s_noteItems.push_back( _T("Civilization: Is the West History? (2011)") );
			s_noteItems.push_back( _T("China: Triumph and Turmoil (2012)") );
			s_noteItems.push_back( _T("The Pity of War (2014)") );
			s_noteItems.push_back( _T("The Square and the Tower") );
		}

		return s_noteItems[ pos % s_noteItems.size() ];
	}
}


namespace reg
{
	static const TCHAR section_dialog[] = _T("FileListDialog");
	static const TCHAR entry_useDiffsMode[] = _T("UseDiffsMode");
	static const TCHAR entry_useAlternateRows[] = _T("UseAlternateRows");
	static const TCHAR entry_useTextEffects[] = _T("UseTextEffects");
	static const TCHAR entry_useDoubleBuffer[] = _T("UseDoubleBuffer");
	static const TCHAR entry_useExplorerTheme[] = _T("UseExplorerTheme");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FILE_STATE_LIST, Size },
		{ IDC_OPEN_DIALOG_BUTTON, MoveX },

		{ IDC_USE_DEFAULT_DRAW_CHECK, MoveY },
		{ IDC_USE_DBG_GUIDES_CHECK, MoveY },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}

CFileListDialog::CFileListDialog( CWnd* pParent )
	: CLayoutDialog( IDD_FILE_LIST_DIALOG, pParent )
	, m_fileListCtrl( IDC_FILE_STATE_LIST, LVS_EX_GRIDLINES | lv::DefaultStyleEx )
	, m_useDiffsMode( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_useDiffsMode, true ) != FALSE )
	, m_useAlternateRows( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_useAlternateRows, true ) != FALSE )
	, m_useTextEffects( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_useTextEffects, false ) != FALSE )
	, m_useDoubleBuffer( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_useDoubleBuffer, true ) != FALSE )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );

	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetTextEffectCallback( this );

	m_fileListCtrl.SetUseAlternateRowColoring( m_useAlternateRows );
	m_fileListCtrl.SetUseExplorerTheme( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_useExplorerTheme, true ) != FALSE );
	m_fileListCtrl.m_ctrlTextEffect.m_textColor = m_useTextEffects ? color::Violet : CLR_NONE;	// list global text effects
	SetFlag( m_fileListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER, m_useDoubleBuffer );

	VERIFY( _FlagStripCount == res::LoadImageListDIB( m_imageList, IDR_FLAG_STRIP_PNG ) );
	m_fileListCtrl.StoreImageLists( &m_imageList );
}

CFileListDialog::~CFileListDialog()
{
	utl::ClearOwningContainer( m_displayItems );
}

void CFileListDialog::InitDisplayItems( void )
{
	const fs::TFileStatePairMap& rStatePairs = hlp::GetStatePairs( m_useDiffsMode );

	utl::ClearOwningContainer( m_displayItems );
	m_displayItems.reserve( rStatePairs.size() );

	for ( fs::TFileStatePairMap::const_iterator itPair = rStatePairs.begin(); itPair != rStatePairs.end(); ++itPair )
		m_displayItems.push_back( new CDisplayObject( &*itPair ) );
}

void CFileListDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();
		InitDisplayItems();

		for ( unsigned int pos = 0; pos != m_displayItems.size(); ++pos )
		{
			const CDisplayObject* pObject = m_displayItems[ pos ];

			m_fileListCtrl.InsertObjectItem( pos, pObject, pos % _FlagStripCount );		// SrcFileName
			m_fileListCtrl.SetSubItemText( pos, DestFileName, pObject->GetDestState().m_fullPath.GetFilename() );

			m_fileListCtrl.SetSubItemText( pos, SrcAttributes, fmt::FormatFileAttributes( pObject->GetSrcState().m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( pObject->GetDestState().m_attributes ) );

			m_fileListCtrl.SetSubItemText( pos, SrcCreationDate, time_utl::FormatTimestamp( pObject->GetSrcState().m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestCreationDate, time_utl::FormatTimestamp( pObject->GetDestState().m_creationTime ) );

			m_fileListCtrl.SetSubItemText( pos, Notes, hlp::GetNotesAt( pos ) );
		}

		m_fileListCtrl.SetupDiffColumnPair( SrcFileName, DestFileName, path::TGetMatch() );
		m_fileListCtrl.SetupDiffColumnPair( SrcAttributes, DestAttributes, str::TGetMatch() );
		m_fileListCtrl.SetupDiffColumnPair( SrcCreationDate, DestCreationDate, str::TGetMatch() );
	}

	if ( orgSel != -1 )		// restore selection?
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}
}

void CFileListDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	pCtrl;
	if ( !m_useTextEffects )
		return;

	static const ui::CTextEffect s_modFileName( ui::Bold );
	static const ui::CTextEffect s_modDest( ui::Regular, CReportListControl::s_mismatchDestTextColor );
	static const ui::CTextEffect s_modSrc( ui::Regular, CReportListControl::s_deleteSrcTextColor );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, ColorErrorBk );

	const CDisplayObject* pObject = CReportListControl::AsPtr<CDisplayObject>( rowKey );
	const ui::CTextEffect* pTextEffect = NULL;
	bool isModified = false, isSrc = false;

	switch ( subItem )
	{
		case SrcFileName:
			isModified = pObject->IsModified();
			if ( isModified )
				pTextEffect = &s_modFileName;
			break;
		case SrcAttributes:
			isSrc = true;		// fall-through
		case DestAttributes:
			isModified = pObject->GetDestState().m_attributes != pObject->GetSrcState().m_attributes;
			break;
		case SrcCreationDate:
			isSrc = true;		// fall-through
		case DestCreationDate:
			isModified = pObject->GetDestState().m_creationTime != pObject->GetSrcState().m_creationTime;
			break;
	}

	if ( pObject->GetDisplayCode() == _T("Focus - Hocus Pocus.gp") )
		rTextEffect |= s_errorBk;

	if ( pTextEffect != NULL )
		rTextEffect |= *pTextEffect;
	else if ( isModified )
		rTextEffect |= isSrc ? s_modSrc : s_modDest;
}

void CFileListDialog::ModifyDiffTextEffectAt( std::vector< ui::CTextEffect >& rMatchEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const
{
	rowKey, pCtrl;
	switch ( subItem )
	{
		case SrcCreationDate:
		case DestCreationDate:
			ClearFlag( rMatchEffects[ str::MatchNotEqual ].m_fontEffect, ui::Bold );		// line-up date columns nicely
			break;
	}
}

BOOL CFileListDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		CLayoutDialog::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CFileListDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_fileListCtrl.m_hWnd;
	DDX_Control( pDX, IDC_FILE_STATE_LIST, m_fileListCtrl );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			CheckDlgButton( IDC_USE_DIFFS_CHECK, m_useDiffsMode );
			CheckDlgButton( IDC_USE_ALTERNATE_ROWS_CHECK, m_useAlternateRows );
			CheckDlgButton( IDC_USE_TEXT_EFFECTS_CHECK, m_useTextEffects );
			CheckDlgButton( IDC_USE_DOUBLEBUFFER_CHECK, m_useDoubleBuffer );
			CheckDlgButton( IDC_USE_EXPLORER_THEME_CHECK, m_fileListCtrl.GetUseExplorerTheme() );
			CheckDlgButton( IDC_USE_DEFAULT_DRAW_CHECK, CReportListCustomDraw::s_useDefaultDraw );
			CheckDlgButton( IDC_USE_DBG_GUIDES_CHECK, CReportListCustomDraw::s_dbgGuides );

			SetupFileListView();
		}
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileListDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED( IDC_OPEN_DIALOG_BUTTON, OnBnClicked_OpenDialog )
	ON_BN_CLICKED( IDC_USE_DIFFS_CHECK, OnToggle_UseDiffsCheck )
	ON_BN_CLICKED( IDC_USE_ALTERNATE_ROWS_CHECK, OnToggle_UseAlternateRows )
	ON_BN_CLICKED( IDC_USE_TEXT_EFFECTS_CHECK, OnToggle_UseTextEffects )
	ON_BN_CLICKED( IDC_USE_DOUBLEBUFFER_CHECK, OnToggle_UseDoubleBuffer )
	ON_BN_CLICKED( IDC_USE_EXPLORER_THEME_CHECK, OnToggle_UseExplorerTheme )
	ON_BN_CLICKED( IDC_USE_DEFAULT_DRAW_CHECK, OnToggle_UseDefaultDraw )
	ON_BN_CLICKED( IDC_USE_DBG_GUIDES_CHECK, OnToggle_UseDbgGuides )
END_MESSAGE_MAP()

void CFileListDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_useDiffsMode, m_useDiffsMode );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_useAlternateRows, m_useAlternateRows );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_useTextEffects, m_useTextEffects );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_useDoubleBuffer, m_useDoubleBuffer );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_useExplorerTheme, m_fileListCtrl.GetUseExplorerTheme() );

	__super::OnDestroy();
}

void CFileListDialog::OnBnClicked_OpenDialog( void )
{
	CTestDialog dialog( this );
	dialog.DoModal();
}

void CFileListDialog::OnToggle_UseDiffsCheck( void )
{
	m_useDiffsMode = IsDlgButtonChecked( IDC_USE_DIFFS_CHECK ) != FALSE;

	SetupFileListView();
}

void CFileListDialog::OnToggle_UseAlternateRows( void )
{
	m_useAlternateRows = IsDlgButtonChecked( IDC_USE_ALTERNATE_ROWS_CHECK ) != FALSE;

	m_fileListCtrl.SetUseAlternateRowColoring( m_useAlternateRows );
	m_fileListCtrl.Invalidate();
}

void CFileListDialog::OnToggle_UseTextEffects( void )
{
	m_useTextEffects = IsDlgButtonChecked( IDC_USE_TEXT_EFFECTS_CHECK ) != FALSE;

	m_fileListCtrl.m_ctrlTextEffect.m_textColor = m_useTextEffects ? color::Violet : CLR_NONE;
	m_fileListCtrl.Invalidate();
}

void CFileListDialog::OnToggle_UseDoubleBuffer( void )
{
	m_useDoubleBuffer = IsDlgButtonChecked( IDC_USE_DOUBLEBUFFER_CHECK ) != FALSE;

	if ( m_useDoubleBuffer )
		m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_DOUBLEBUFFER );
	else
		m_fileListCtrl.ModifyListStyleEx( LVS_EX_DOUBLEBUFFER, 0 );

	m_fileListCtrl.Invalidate();
}

void CFileListDialog::OnToggle_UseExplorerTheme( void )
{
	m_fileListCtrl.SetUseExplorerTheme( IsDlgButtonChecked( IDC_USE_EXPLORER_THEME_CHECK ) != FALSE );
}

void CFileListDialog::OnToggle_UseDefaultDraw( void )
{
	CReportListCustomDraw::s_useDefaultDraw = IsDlgButtonChecked( IDC_USE_DEFAULT_DRAW_CHECK ) != FALSE;
	m_fileListCtrl.Invalidate();
}

void CFileListDialog::OnToggle_UseDbgGuides( void )
{
	CReportListCustomDraw::s_dbgGuides = IsDlgButtonChecked( IDC_USE_DBG_GUIDES_CHECK ) != FALSE;
	m_fileListCtrl.Invalidate();
}


// CDisplayObject implementation

const std::tstring& CDisplayObject::GetCode( void ) const
{
	return m_pStatePair->first.m_fullPath.Get();
}

std::tstring CDisplayObject::GetDisplayCode( void ) const
{
	return m_displayPath;
}
