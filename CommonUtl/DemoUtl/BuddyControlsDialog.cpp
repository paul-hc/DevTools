
#include "stdafx.h"
#include "BuddyControlsDialog.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileEnumerator.h"
#include "utl/FileSystem.h"
#include "utl/FlagTags.h"
#include "utl/PathItemBase.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CFileStateExItem : public CFileStateItem
{
public:
	CFileStateExItem( const CFileFind& foundFile )
		: CFileStateItem( fs::CFileState( foundFile ) )
		, m_checksumElapsed( 0.0 )
	{
	}

	CFileStateExItem( const TCHAR tag[] )
		: CFileStateItem( fs::CFileState() )
		, m_checksumElapsed( 0.0 )
	{
		m_fileState.m_fullPath.Set( tag );		// set the proxy item tag
	}

	bool IsProxy( void ) const { return !GetState().IsValid(); }
	double GetChecksumElapsed( void ) const { return m_checksumElapsed; }

	void ComputeChecksum( void )
	{
		if ( !IsProxy() )
		{
			CTimer timer;
			m_fileState.ComputeCrc32( fs::CFileState::CacheCompute );
			m_checksumElapsed = timer.ElapsedSeconds();
		}
	}

	void ResetProxy( void )
	{
		REQUIRE( IsProxy() );
		m_fileState.m_fileSize = 0;
		m_checksumElapsed = 0.0;
	}

	CFileStateExItem& operator+=( const CFileStateExItem& right )
	{
		m_fileState.m_fileSize += right.GetState().m_fileSize;
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
		SumElapsed( CFileStateExItem* pTotalProxy ) : m_pTotalProxy( pTotalProxy ) {}

		void operator()( const CFileStateExItem* pItem )
		{
			if ( !pItem->IsProxy() )
				*m_pTotalProxy += *pItem;
		}
	private:
		CFileStateExItem* m_pTotalProxy;
	};
}


namespace impl
{
	struct CEnumerator : public fs::IEnumerator
	{
		CEnumerator( std::vector< CFileStateExItem* >* pFileItems ) : m_pFileItems( pFileItems ) { ASSERT_PTR( m_pFileItems ); }

		// IEnumerator interface
		virtual void AddFile( const CFileFind& foundFile )
		{
			m_pFileItems->push_back( new CFileStateExItem( foundFile ) );
		}

		virtual void AddFoundFile( const TCHAR* pFilePath ) { pFilePath; }
	private:
		std::vector< CFileStateExItem* >* m_pFileItems;
	public:
		std::vector< fs::CPath > m_subDirPaths;
		size_t m_moreFilesCount;			// incremented when it reaches the limit
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
	, m_fileListCtrl( IDC_FILE_STATE_EX_LIST, LVS_EX_GRIDLINES | lv::DefaultStyleEx )
	, m_searchPathCombo( ui::MixedPath )
	, m_folderPathCombo( ui::DirPath )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_searchPathCombo.SetEnsurePathExist();

	m_folderPathCombo.SetEnsurePathExist();
	m_folderPathCombo.RefTandemLayout().m_alignment = H_AlignLeft | V_AlignCenter;

	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetSubjectAdapter( ui::GetFullPathAdapter() );		// display full paths
}

CBuddyControlsDialog::~CBuddyControlsDialog()
{
	utl::ClearOwningContainer( m_fileItems );
}

void CBuddyControlsDialog::SearchForFiles( void )
{
	utl::ClearOwningContainer( m_fileItems );

	if ( fs::IsValidFile( m_searchPath.GetPtr() ) )
	{
		CFileFind foundFile;

		if ( m_searchPath.LocateFile( foundFile ) )
			m_fileItems.push_back( new CFileStateExItem( foundFile ) );
	}
	else
	{
		fs::CPath dirPath;
		std::tstring wildSpec = _T("*");

		if ( fs::IsValidDirectoryPattern( m_searchPath, &dirPath, &wildSpec ) )
		{
			CWaitCursor wait;
			impl::CEnumerator found( &m_fileItems );

			fs::EnumFiles( &found, dirPath, wildSpec.c_str(), fs::EF_Recurse );

			typedef pred::CompareAdapter< pred::CompareNaturalPath, CPathItemBase::ToFilePath > CompareFileItem;
			typedef pred::LessValue< CompareFileItem > TLess_FileItem;

			std::sort( m_fileItems.begin(), m_fileItems.end(), TLess_FileItem() );			// sort by full key and path
		}
	}

	SetupFileListView();
	m_fileListCtrl.UpdateWindow();		// display found files on the spot if continuing with CRC32 evaluation
}

void CBuddyControlsDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();

		unsigned int pos = 0;

		for ( ; pos != m_fileItems.size(); ++pos )
		{
			const CFileStateExItem* pFileItem = m_fileItems[ pos ];
			const fs::CFileState& fileState = pFileItem->GetState();

			m_fileListCtrl.InsertObjectItem( pos, pFileItem );		// FileName

			if ( !pFileItem->IsProxy() )
			{
				m_fileListCtrl.SetSubItemText( pos, Attributes, fs::CFileState::GetTags_FileAttributes().FormatKey( fileState.m_attributes, _T("") ) );
				m_fileListCtrl.SetSubItemText( pos, CRC32, str::Format( _T("%08X"), fileState.GetCrc32( fs::CFileState::AsIs ) ) );
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
			DragAcceptFiles();
			m_searchPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
			m_folderPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
		}

		SetupFileListView();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CBuddyControlsDialog, CLayoutDialog )
	ON_WM_DROPFILES()
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

void CBuddyControlsDialog::OnDropFiles( HDROP hDropInfo )
{
	std::vector< fs::CPath > searchPaths;
	shell::QueryDroppedFiles( searchPaths, hDropInfo );

	if ( !searchPaths.empty() && searchPaths.front().FileExist() )
	{
		m_searchPath = searchPaths.front();
		SearchForFiles();
	}
}

void CBuddyControlsDialog::OnBnClicked_FindFiles( void )
{
	m_searchPath = ui::GetComboSelText( m_searchPathCombo );
	SearchForFiles();
}

void CBuddyControlsDialog::OnBnClicked_CalculateChecksums( void )
{
	if ( m_fileItems.empty() )
		OnBnClicked_FindFiles();

	CWaitCursor wait;

	utl::for_each( m_fileItems, std::mem_fun( &CFileStateExItem::ComputeChecksum ) );

	if ( !m_fileItems.empty() )
	{	// insert/update the TOTALS proxy item
		CFileStateExItem* pTotalItem;

		if ( m_fileItems.back()->IsProxy() )
		{	// clear existing proxy item
			pTotalItem = m_fileItems.back();
			pTotalItem->ResetProxy();
		}
		else
			m_fileItems.push_back( pTotalItem = new CFileStateExItem( _T("TOTAL:") ) );

		std::for_each( m_fileItems.begin(), --m_fileItems.end(), func::SumElapsed( pTotalItem ) );
	}

	SetupFileListView();
}
