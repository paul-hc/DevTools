
#include "stdafx.h"
#include "FileChecksumsDialog.h"
#include "utl/AppTools.h"
#include "utl/FileEnumerator.h"
#include "utl/FileSystem.h"
#include "utl/Crc32.h"
#include "utl/PathItemBase.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"
#include "resource.h"
#include <fstream>
#include <numeric>

#pragma warning( push, 3 )			// switch to warning level 3
#pragma warning( disable: 4245 )	// identifier was truncated to 'number' characters in the debug information
#include <boost/crc.hpp>	// for boost::crc_32_type
#pragma warning( pop )				// restore to the initial warning level

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


namespace utl
{
	UINT ComputeUtlFileCrc32( const fs::CPath& filePath ) throws_( std::exception )
	{
		TCrc32Checksum checkSum;

		char buffer[ crc32::FileBlockSize ];

		std::ifstream ifs( filePath.GetPtr(), std::ios_base::binary );

		while ( ifs )
		{
			ifs.read( buffer, COUNT_OF( buffer ) );
			checkSum.ProcessBytes( buffer, ifs.gcount() );
		}

		return checkSum.GetResult();
	}

	UINT ComputeBoostFileCrc32( const fs::CPath& filePath ) throws_( std::exception )
	{
		boost::crc_32_type result;
		char buffer[ crc32::FileBlockSize ];

		std::ifstream ifs( filePath.GetPtr(), std::ios_base::binary );

		while ( ifs )
		{
			ifs.read( buffer, COUNT_OF( buffer ) );
			result.process_bytes( buffer, ifs.gcount() );
		}

		return result.checksum();
	}

	UINT ComputeBoostCFileCrc32( const fs::CPath& filePath ) throws_( CFileException* )
	{
		CFile file( filePath.GetPtr(), CFile::modeRead | CFile::shareDenyWrite );

		boost::crc_32_type result;
		std::vector< BYTE > buffer( crc32::FileBlockSize );
		BYTE* pBuffer = &buffer.front();
		UINT readCount;
		
		do
		{
			readCount = file.Read( pBuffer, crc32::FileBlockSize );
			result.process_bytes( pBuffer, readCount );
		}
		while ( readCount > 0 );

		file.Close();
		return result.checksum();
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
	CFileChecksumItem( const CFileFind& foundFile )
		: CPathItemBase( fs::CPath( foundFile.GetFilePath().GetString() ) )
		, m_fileSize( foundFile.GetLength() )
	{
	}

	CFileChecksumItem( const fs::CPath& filePath )
		: CPathItemBase( filePath )
		, m_fileSize( fs::GetFileSize( filePath.GetPtr() ) )
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

	try
	{
		m_utl.m_crc32 = crc32::ComputeFileChecksum( GetFilePath() );
	}
	catch ( CFileException* pExc )
	{
		app::TraceException( pExc );
		pExc->Delete();
		m_utl.m_crc32 = UINT_MAX;
	}
	m_utl.m_elapsedSecs = timer.ElapsedSeconds();

	timer.Restart();
	m_utlIfs.m_crc32 = utl::ComputeUtlFileCrc32( GetFilePath() );
	m_utlIfs.m_elapsedSecs = timer.ElapsedSeconds();

	timer.Restart();
	try
	{
		m_boostCFile.m_crc32 = utl::ComputeBoostCFileCrc32( GetFilePath() );
	}
	catch ( CFileException* pExc )
	{
		app::TraceException( pExc );
		pExc->Delete();
		m_boostCFile.m_crc32 = UINT_MAX;
	}
	m_boostCFile.m_elapsedSecs = timer.ElapsedSeconds();

	timer.Restart();
	try
	{
		m_boostIfs.m_crc32 = utl::ComputeBoostFileCrc32( GetFilePath() );
	}
	catch ( std::exception exc )
	{
		app::TraceException( exc );
		m_boostIfs.m_crc32 = UINT_MAX;
	}
	m_boostIfs.m_elapsedSecs = timer.ElapsedSeconds();
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


namespace impl
{
	struct CEnumerator : public fs::IEnumerator
	{
		CEnumerator( std::vector< CFileChecksumItem* >* pFileItems ) : m_pFileItems( pFileItems ) { ASSERT_PTR( m_pFileItems ); }

		// IEnumerator interface
		virtual void AddFile( const CFileFind& foundFile )
		{
			m_pFileItems->push_back( new CFileChecksumItem( foundFile ) );
		}

		virtual void AddFoundFile( const TCHAR* pFilePath ) { pFilePath; }
	private:
		std::vector< CFileChecksumItem* >* m_pFileItems;
	public:
		std::vector< fs::CPath > m_filePaths;
		std::vector< fs::CPath > m_subDirPaths;
		size_t m_moreFilesCount;			// incremented when it reaches the limit
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
		{ IDC_SEARCH_PATH_COMBO, SizeX },
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
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_searchPathCombo.SetEnsurePathExist();
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	SetFlag( m_fileListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER, false );
}

CFileChecksumsDialog::~CFileChecksumsDialog()
{
	utl::ClearOwningContainer( m_fileItems );
}

void CFileChecksumsDialog::SearchForFiles( void )
{
	utl::ClearOwningContainer( m_fileItems );

	if ( fs::IsValidFile( m_searchPath.GetPtr() ) )
		m_fileItems.push_back( new CFileChecksumItem( m_searchPath ) );
	else
	{
		fs::CPath dirPath = m_searchPath;
		std::tstring pattern = _T("*");

		if ( path::ContainsWildcards( m_searchPath.GetPtr() ) )
		{
			dirPath = m_searchPath.GetParentPath();
			pattern = m_searchPath.GetFilename();
		}

		if ( fs::IsValidDirectory( dirPath.GetPtr() ) )
		{
			CWaitCursor wait;
			impl::CEnumerator found( &m_fileItems );

			fs::EnumFiles( &found, dirPath, pattern.c_str(), fs::EF_Recurse );

			typedef pred::CompareAdapter< pred::CompareNaturalPath, CPathItemBase::ToFilePath > CompareFileItem;
			typedef pred::LessValue< CompareFileItem > TLess_FileItem;

			std::sort( m_fileItems.begin(), m_fileItems.end(), TLess_FileItem() );			// sort by full key and path
		}
	}

	SetupFileListView();
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
	bool firstInit = NULL == m_searchPathCombo.m_hWnd;

	DDX_Control( pDX, IDC_SEARCH_PATH_COMBO, m_searchPathCombo );
	DDX_Control( pDX, IDC_FILE_CHECKSUMS_LIST, m_fileListCtrl );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			DragAcceptFiles();
			m_searchPathCombo.LoadHistory( reg::section_dialog, reg::entry_searchPathHistory );
		}

		SetupFileListView();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileChecksumsDialog, CLayoutDialog )
	ON_WM_DROPFILES()
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

void CFileChecksumsDialog::OnDropFiles( HDROP hDropInfo )
{
	std::vector< fs::CPath > searchPaths;
	shell::QueryDroppedFiles( searchPaths, hDropInfo );

	if ( !searchPaths.empty() && searchPaths.front().FileExist() )
	{
		m_searchPath = searchPaths.front();
		SearchForFiles();
	}
}

void CFileChecksumsDialog::OnBnClicked_FindFiles( void )
{
	m_searchPath = ui::GetComboSelText( m_searchPathCombo );
	SearchForFiles();
}

void CFileChecksumsDialog::OnBnClicked_CalculateChecksums( void )
{
	if ( m_fileItems.empty() )
		OnBnClicked_FindFiles();

	CWaitCursor wait;

	for ( std::vector< CFileChecksumItem* >::const_iterator itFileItem = m_fileItems.begin(); itFileItem != m_fileItems.end(); ++itFileItem )
		(*itFileItem)->ComputeChecksums();

	if ( !m_fileItems.empty() && m_fileItems.front()->HasChecksums() )
	{	// insert/update the TOTALS proxy item
		CFileChecksumItem* pTotalItem = NULL;

		if ( m_fileItems.back()->IsProxy() )
			pTotalItem = m_fileItems.back();
		else
			m_fileItems.push_back( pTotalItem = new CFileChecksumItem( _T("TOTAL:") ) );

		std::for_each( m_fileItems.begin(), m_fileItems.end(), func::SumElapsed( pTotalItem ) );
	}

	SetupFileListView();
}
