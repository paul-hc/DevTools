
#include "stdafx.h"
#include "BuddyControlsDialog.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileStateEnumerator.h"
#include "utl/FileSystem.h"
#include "utl/FlagTags.h"
#include "utl/FileStateItem.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/TimeUtils.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/FileStateEnumerator.hxx"
#include "utl/UI/TandemControls.hxx"


class CFileStateTimedItem : public CFileStateItem
{
public:
	CFileStateTimedItem( const fs::CFileState& fileState )
		: CFileStateItem( fileState )
		, m_checksumElapsed( 0.0 )
	{
	}

	CFileStateTimedItem( const TCHAR tag[] )
		: CFileStateItem( fs::CFileState() )
		, m_checksumElapsed( 0.0 )
	{
		SetFilePath( fs::CPath( tag ) );		// set the proxy item tag
	}

	bool IsProxy( void ) const { return !GetState().IsValid(); }
	double GetChecksumElapsed( void ) const { return m_checksumElapsed; }

	void ComputeChecksum( void )
	{
		if ( !IsProxy() )
		{
			CTimer timer;
			RefState().ComputeCrc32( fs::CFileState::CacheCompute );
			m_checksumElapsed = timer.ElapsedSeconds();
		}
	}

	void ResetProxy( void )
	{
		REQUIRE( IsProxy() );
		RefState().m_fileSize = 0;
		m_checksumElapsed = 0.0;
	}

	CFileStateTimedItem& operator+=( const CFileStateTimedItem& right )
	{
		RefState().m_fileSize += right.GetState().m_fileSize;
		m_checksumElapsed += right.m_checksumElapsed;

		return *this;
	}
private:
	double m_checksumElapsed;
};


namespace func
{
	struct SumElapsed
	{
		SumElapsed( CFileStateTimedItem* pTotalProxy ) : m_pTotalProxy( pTotalProxy ) {}

		void operator()( const CFileStateTimedItem* pItem )
		{
			if ( !pItem->IsProxy() )
				*m_pTotalProxy += *pItem;
		}
	private:
		CFileStateTimedItem* m_pTotalProxy;
	};
}


namespace reg
{
	static const TCHAR section_dialog[] = _T("BuddyControlsDialog");
	static const TCHAR entry_searchPathHistory[] = _T("SearchPathHistory");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_SEARCH_PATH_COMBO, pctSizeX( 50 ) },
		{ IDC_FOLDER_PATH_COMBO, pctSizeX( 50 ) },
		{ IDC_FIND_FILES_BUTTON, MoveX },
		{ IDC_CALC_CHECKSUMS_BUTTON, MoveX },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },

		{ IDC_FILE_STATE_EX_LIST, Size }
	};
}


// CBuddyControlsDialog implementation

CBuddyControlsDialog::CBuddyControlsDialog( CWnd* pParent )
	: CLayoutDialog( IDD_BUDDY_CONTROLS_DIALOG, pParent )
	, m_searchPathCombo( ui::MixedPath )
	, m_folderPathCombo( ui::DirPath )
	, m_fileListCtrl( ui::ListHost_TileMateOnTopRight /*IDC_FILE_STATE_EX_LIST, LVS_EX_GRIDLINES | lv::DefaultStyleEx*/ )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_searchPathCombo.SetEnsurePathExist();

	m_folderPathCombo.SetEnsurePathExist();
	m_folderPathCombo.RefTandemLayout().SetTandemAlign( H_AlignLeft | V_AlignCenter | ui::H_ShrinkHost );

	//SetFlag( m_fileListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER, false );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetLayoutInfo( IDC_FILE_STATE_EX_LIST );
	m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_GRIDLINES );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetSubjectAdapter( ui::GetFullPathAdapter() );		// display full paths

	m_fileListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
	m_fileListCtrl.AddColumnCompare( ModifyTime, pred::NewPropertyComparator<CFileStateTimedItem>( func::AsModifyTime() ), false );

	m_fileListCtrl.GetMateToolbar()->GetStrip()
		.AddButton( ID_LIST_VIEW_REPORT )
		.AddButton( ID_LIST_VIEW_TILE )
		.AddSeparator()
		.AddButton( ID_LIST_VIEW_ICON_LARGE )
		.AddButton( ID_LIST_VIEW_ICON_SMALL )
		.AddButton( ID_LIST_VIEW_LIST );
}

CBuddyControlsDialog::~CBuddyControlsDialog()
{
	utl::ClearOwningContainer( m_fileItems );
}

void CBuddyControlsDialog::SearchForFiles( void )
{
	CWaitCursor wait;
	fs::CFileStateItemEnumerator<CFileStateTimedItem> found( fs::EF_Recurse );

	if ( fs::InvalidPattern == fs::SearchEnumFiles( &found, m_searchPath ) )
		ui::MessageBox( std::tstring( _T("Invalid path:\n\n") ) + m_searchPath.Get() );

	found.SortItems();
	m_fileItems.swap( found.m_fileItems );		// (!) enumerator destructor deletes the old swapped items

	SetupFileListView();
	m_fileListCtrl.UpdateWindow();				// display found files on the spot if continuing with CRC32 evaluation
}

void CBuddyControlsDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();

		for ( unsigned int pos = 0; pos != m_fileItems.size(); ++pos )
		{
			const CFileStateTimedItem* pFileItem = m_fileItems[ pos ];
			const fs::CFileState& fileState = pFileItem->GetState();

			m_fileListCtrl.InsertObjectItem( pos, pFileItem );		// FilePath

			if ( !pFileItem->IsProxy() )
			{
				m_fileListCtrl.SetSubItemText( pos, Attributes, fs::CFileState::GetTags_FileAttributes().FormatKey( fileState.m_attributes, _T("") ) );
				m_fileListCtrl.SetSubItemText( pos, CRC32, str::Format( _T("%08X"), fileState.GetCrc32( fs::CFileState::AsIs ) ) );
				m_fileListCtrl.SetSubItemText( pos, ModifyTime, time_utl::FormatTimestamp( fileState.m_modifTime ) );
			}
			else
				m_fileListCtrl.MarkRowAt( pos, ui::CTextEffect( ui::Bold, color::Red ) );

			m_fileListCtrl.SetSubItemText( pos, FileSize, num::FormatFileSizeAsPair( fileState.m_fileSize ) );
			m_fileListCtrl.SetSubItemText( pos, ChecksumElapsed, CTimer::FormatSeconds( pFileItem->GetChecksumElapsed() ) );
		}
	}

	if ( orgSel != -1 )		// restore selection?
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}
}

BOOL CBuddyControlsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) /*||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo )*/;
}

void CBuddyControlsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_searchPathCombo.m_hWnd;

	DDX_Control( pDX, IDC_SEARCH_PATH_COMBO, m_searchPathCombo );
	DDX_Control( pDX, IDC_FOLDER_PATH_COMBO, m_folderPathCombo );
	DDX_Control( pDX, IDC_FILE_STATE_EX_LIST, m_fileListCtrl );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_searchPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
			m_folderPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
		}

		SetupFileListView();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CBuddyControlsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_FIND_FILES_BUTTON, OnBnClicked_FindFiles )
	ON_BN_CLICKED( IDC_CALC_CHECKSUMS_BUTTON, OnBnClicked_CalculateChecksums )
END_MESSAGE_MAP()

void CBuddyControlsDialog::OnOK( void )
{
	// Notifications are disabled for the focused control during UpdateData() by MFC.
	// Save history combos just before UpdateData( DialogSaveChanges ), so that HCN_VALIDATEITEMS notifications are received.
	m_searchPathCombo.SaveHistory( reg::section_dialog, reg::entry_searchPathHistory );

	__super::OnOK();
}

void CBuddyControlsDialog::OnBnClicked_FindFiles( void )
{
	CWaitCursor wait;

	m_searchPath = ui::GetComboSelText( m_searchPathCombo );
	SearchForFiles();
}

void CBuddyControlsDialog::OnBnClicked_CalculateChecksums( void )
{
	if ( m_fileItems.empty() )
		OnBnClicked_FindFiles();

	CWaitCursor wait;

	utl::for_each( m_fileItems, std::mem_fun( &CFileStateTimedItem::ComputeChecksum ) );

	if ( !m_fileItems.empty() )
	{	// insert/update the TOTALS proxy item
		CFileStateTimedItem* pTotalItem;

		if ( m_fileItems.back()->IsProxy() )
		{	// clear existing proxy item
			pTotalItem = m_fileItems.back();
			pTotalItem->ResetProxy();
		}
		else
			m_fileItems.push_back( pTotalItem = new CFileStateTimedItem( _T("TOTAL:") ) );

		std::for_each( m_fileItems.begin(), --m_fileItems.end(), func::SumElapsed( pTotalItem ) );
	}

	SetupFileListView();
}
