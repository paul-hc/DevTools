
#include "pch.h"
#include "FileChecksumsDialog.h"
#include "utl/Algorithms.h"
#include "utl/IoBin.h"
#include "utl/FileEnumerator.h"
#include "utl/FileStateEnumerator.h"
#include "utl/FileSystem.h"
#include "utl/PathItemBase.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"
#include "resource.h"
#include <fstream>

#define USE_BOOST_CRC
#include "utl/Crc32.h"		// define func::ComputeBoostChecksum

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/FileStateEnumerator.hxx"


namespace hlp
{
	UINT ComputeUtlFileStreamCrc32( const fs::CPath& filePath )
	{
		return io::bin::ReadFileStream_NoThrow( filePath, func::ComputeChecksum<utl::TCrc32Checksum>() ).m_checksum.GetResult();
	}

	UINT ComputeBoostFileStreamCrc32( const fs::CPath& filePath )
	{
		return io::bin::ReadFileStream_NoThrow( filePath, func::ComputeBoostChecksum<>() ).m_checksum.checksum();
	}

	UINT ComputeBoostCFileCrc32( const fs::CPath& filePath )
	{
		return io::bin::ReadCFile_NoThrow( filePath, func::ComputeBoostChecksum<>() ).m_checksum.checksum();
	}
}


struct CChecksumInfo
{
	CChecksumInfo( void  ) : m_crc32( 0 ), m_elapsedSecs( 0.0 ) {}
public:
	UINT m_crc32;
	double m_elapsedSecs;
};


class CFileChecksumItem : public CPathItemBase
{
public:
	CFileChecksumItem( const fs::CFileState& fileState )
		: CPathItemBase( fileState.m_fullPath )
		, m_fileSize( fileState.m_fileSize )
	{
	}

	CFileChecksumItem( const TCHAR tag[] )
		: CPathItemBase( fs::CPath( tag ) )
		, m_fileSize( 0 )
	{
		m_utlIfs.m_crc32 = UINT_MAX;		// mark as proxy item
	}

	bool HasChecksums( void ) const { return m_utl.m_crc32 != 0; }
	bool IsProxy( void ) const { return UINT_MAX == m_utlIfs.m_crc32; }
	void ComputeChecksums( void );

	void ResetProxy( void )
	{
		REQUIRE( IsProxy() );
		m_fileSize = 0;
		m_utl.m_elapsedSecs = m_utlIfs.m_elapsedSecs = m_boostCFile.m_elapsedSecs = m_boostIfs.m_elapsedSecs = 0.0;
	}

	CFileChecksumItem& operator+=( const CFileChecksumItem& right )
	{
		m_fileSize += right.m_fileSize;
		m_utl.m_elapsedSecs += right.m_utl.m_elapsedSecs;
		m_utlIfs.m_elapsedSecs += right.m_utlIfs.m_elapsedSecs;
		m_boostCFile.m_elapsedSecs += right.m_boostCFile.m_elapsedSecs;
		m_boostIfs.m_elapsedSecs += right.m_boostIfs.m_elapsedSecs;

		return *this;
	}
public:
	ULONGLONG m_fileSize;
	CChecksumInfo m_utl;
	CChecksumInfo m_utlIfs;
	CChecksumInfo m_boostCFile;
	CChecksumInfo m_boostIfs;
};


// CFileChecksumItem implementation

void CFileChecksumItem::ComputeChecksums( void )
{
	if ( IsProxy() )
		return;

	CTimer timer;
	{
		m_utl.m_crc32 = crc32::ComputeFileChecksum( GetFilePath() );
		m_utl.m_elapsedSecs = timer.ElapsedSeconds();
	}
	{
		timer.Restart();
		m_utlIfs.m_crc32 = hlp::ComputeUtlFileStreamCrc32( GetFilePath() );
		m_utlIfs.m_elapsedSecs = timer.ElapsedSeconds();
	}
	{
		timer.Restart();
		m_boostCFile.m_crc32 = hlp::ComputeBoostCFileCrc32( GetFilePath() );
		m_boostCFile.m_elapsedSecs = timer.ElapsedSeconds();
	}
	{
		timer.Restart();
		m_boostIfs.m_crc32 = hlp::ComputeBoostFileStreamCrc32( GetFilePath() );
		m_boostIfs.m_elapsedSecs = timer.ElapsedSeconds();
	}
}


namespace func
{
	struct SumElapsed
	{
		SumElapsed( CFileChecksumItem* pTotalItem ) : m_pTotalItem( pTotalItem ) {}

		void operator()( const CFileChecksumItem* pItem )
		{
			if ( !pItem->IsProxy() )
				*m_pTotalItem += *pItem;
		}
	private:
		CFileChecksumItem* m_pTotalItem;
	};
}


namespace reg
{
	static const TCHAR section_dialog[] = _T("FileChecksumsDialog");
	static const TCHAR entry_searchPathHistory[] = _T("SearchPathHistory");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_SEARCH_PATH_COMBO, pctSizeX( 50 ) },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX },
		{ IDC_FIND_FILES_BUTTON, MoveX },
		{ IDC_CALC_CHECKSUMS_BUTTON, MoveX },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },

		{ IDC_FILE_CHECKSUMS_LIST, Size }
	};
}


// CFileChecksumsDialog implementation

CFileChecksumsDialog::CFileChecksumsDialog( CWnd* pParent )
	: CLayoutDialog( IDD_FILE_CHECKSUMS_DIALOG, pParent )
	, m_fileListCtrl( IDC_FILE_CHECKSUMS_LIST, LVS_EX_GRIDLINES | lv::DefaultStyleEx )
	, m_searchPathCombo( ui::MixedPath )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );

	m_searchPathCombo.SetEnsurePathExist();
	//SetFlag( m_fileListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER, false );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.AddRecordCompare( pred::NewComparator( pred::TCompareCode() ) );		// default row item comparator
}

CFileChecksumsDialog::~CFileChecksumsDialog()
{
	utl::ClearOwningContainer( m_fileItems );
}

void CFileChecksumsDialog::SearchForFiles( void )
{
	CWaitCursor wait;
	fs::CFileStateItemEnumerator<CFileChecksumItem> found( fs::EF_Recurse );

	if ( fs::InvalidPattern == fs::SearchEnumFiles( &found, m_searchPath ) )
		ui::MessageBox( std::tstring( _T("Invalid path:\n\n") ) + m_searchPath.Get() );

	found.SortItems();
	m_fileItems.swap( found.m_fileItems );		// (!) enumerator destructor deletes the old swapped items

	SetupFileListView();
	m_fileListCtrl.UpdateWindow();				// display found files on the spot if continuing with CRC32 evaluation
}

void CFileChecksumsDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();

		unsigned int pos = 0;

		for ( ; pos != m_fileItems.size(); ++pos )
		{
			const CFileChecksumItem* pFileItem = m_fileItems[ pos ];

			m_fileListCtrl.InsertObjectItem( pos, pFileItem );		// FileName

			if ( !pFileItem->IsProxy() )
			{
				m_fileListCtrl.SetSubItemText( pos, DirPath, pFileItem->GetFilePath().GetParentPath().Get() );
				m_fileListCtrl.SetSubItemText( pos, UTL_CRC32, str::Format( _T("%08X"), pFileItem->m_utl.m_crc32 ) );
				m_fileListCtrl.SetSubItemText( pos, UTL_ifs_CRC32, str::Format( _T("%08X"), pFileItem->m_utlIfs.m_crc32 ) );
				m_fileListCtrl.SetSubItemText( pos, BoostCFile_CRC32, str::Format( _T("%08X"), pFileItem->m_boostCFile.m_crc32 ) );
				m_fileListCtrl.SetSubItemText( pos, Boost_ifs_CRC32, str::Format( _T("%08X"), pFileItem->m_boostIfs.m_crc32 ) );
			}
			else
				m_fileListCtrl.MarkRowAt( pos, ui::CTextEffect( ui::Bold, color::Blue ) );

			m_fileListCtrl.SetSubItemText( pos, FileSize, num::FormatFileSizeAsPair( pFileItem->m_fileSize ) );
			m_fileListCtrl.SetSubItemText( pos, UTL_Elapsed, CTimer::FormatSeconds( pFileItem->m_utl.m_elapsedSecs ) );
			m_fileListCtrl.SetSubItemText( pos, UTL_ifs_Elapsed, CTimer::FormatSeconds( pFileItem->m_utlIfs.m_elapsedSecs ) );
			m_fileListCtrl.SetSubItemText( pos, BoostCFile_Elapsed, CTimer::FormatSeconds( pFileItem->m_boostCFile.m_elapsedSecs ) );
			m_fileListCtrl.SetSubItemText( pos, Boost_ifs_Elapsed, CTimer::FormatSeconds( pFileItem->m_boostIfs.m_elapsedSecs ) );
		}
	}

	if ( orgSel != -1 )		// restore selection?
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}
}

BOOL CFileChecksumsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CFileChecksumsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_searchPathCombo.m_hWnd;

	DDX_Control( pDX, IDC_SEARCH_PATH_COMBO, m_searchPathCombo );
	DDX_Control( pDX, IDC_FILE_CHECKSUMS_LIST, m_fileListCtrl );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_searchPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
		}

		SetupFileListView();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileChecksumsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_FIND_FILES_BUTTON, OnBnClicked_FindFiles )
	ON_BN_CLICKED( IDC_CALC_CHECKSUMS_BUTTON, OnBnClicked_CalculateChecksums )
END_MESSAGE_MAP()

void CFileChecksumsDialog::OnOK( void )
{
	// Notifications are disabled for the focused control during UpdateData() by MFC.
	// Save history combos just before UpdateData( DialogSaveChanges ), so that HCN_VALIDATEITEMS notifications are received.
	m_searchPathCombo.SaveHistory( reg::section_dialog, reg::entry_searchPathHistory );

	__super::OnOK();
}

void CFileChecksumsDialog::OnBnClicked_FindFiles( void )
{
	CWaitCursor wait;

	m_searchPath = ui::GetComboSelText( m_searchPathCombo );
	SearchForFiles();
}

void CFileChecksumsDialog::OnBnClicked_CalculateChecksums( void )
{
	if ( m_fileItems.empty() )
		OnBnClicked_FindFiles();

	CWaitCursor wait;

	utl::for_each( m_fileItems, std::mem_fun( &CFileChecksumItem::ComputeChecksums ) );

	if ( !m_fileItems.empty() && m_fileItems.front()->HasChecksums() )
	{	// insert/update the TOTALS proxy item
		CFileChecksumItem* pTotalItem;

		if ( m_fileItems.back()->IsProxy() )
		{	// clear existing proxy item
			pTotalItem = m_fileItems.back();
			pTotalItem->ResetProxy();
		}
		else
			m_fileItems.push_back( pTotalItem = new CFileChecksumItem( _T("TOTAL:") ) );

		std::for_each( m_fileItems.begin(), --m_fileItems.end(), func::SumElapsed( pTotalItem ) );
	}

	SetupFileListView();
}
