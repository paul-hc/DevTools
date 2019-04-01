
#include "stdafx.h"
#include "DropFilesModel.h"
#include "utl/AppTools.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/Logger.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/ShellTypes.h"
#include "utl/UI/ShellUtilities.h"
#include <deque>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDropFilesModel::CDropFilesModel( const fs::CPath& destDirPath )
	: m_destDirPath( destDirPath )
	, m_dropEffect( DROPEFFECT_NONE )
{
	ASSERT( fs::IsValidDirectory( m_destDirPath.GetPtr() ) );
}

CDropFilesModel::~CDropFilesModel()
{
}

void CDropFilesModel::Clear( void )
{
	m_dropPaths.clear();
	m_dropEffect = DROPEFFECT_NONE;

	m_srcCommonFolderPath.Clear();
	m_srcFolderPaths.clear();
	m_srcDeepFolderPaths.clear();
	m_relFolderPaths.clear();
	m_pImageStore.reset();
}

void CDropFilesModel::BuildFromClipboard( void )
{
	Clear();

	std::vector< fs::CPath > dropPaths;
	DROPEFFECT dropEffect = CClipboard::QueryDropFilePaths( dropPaths );

	if ( !dropPaths.empty() )
		Init( dropPaths, dropEffect );
}

void CDropFilesModel::Init( const std::vector< fs::CPath >& dropPaths, DROPEFFECT dropEffect )
{
	m_dropPaths = dropPaths;
	m_dropEffect = dropEffect;

	m_srcCommonFolderPath = path::ExtractCommonParentPath( m_dropPaths );

	m_pImageStore.reset( new CImageStore );

	// folders of drop source files
	for ( std::vector< fs::CPath >::const_iterator itDropPath = m_dropPaths.begin(); itDropPath != m_dropPaths.end(); ++itDropPath )
		if ( fs::IsValidDirectory( itDropPath->GetPtr() ) )
			utl::AddUnique( m_srcFolderPaths, *itDropPath );
		else
			utl::AddUnique( m_srcFolderPaths, itDropPath->GetParentPath() );

	fs::SortPaths( m_srcFolderPaths );

	for ( std::vector< fs::CPath >::const_iterator itSrcFolderPath = m_srcFolderPaths.begin(); itSrcFolderPath != m_srcFolderPaths.end(); ++itSrcFolderPath )
	{
		m_srcDeepFolderPaths.push_back( *itSrcFolderPath );

		std::vector< fs::CPath > subDirPaths;
		fs::EnumSubDirs( subDirPaths, *itSrcFolderPath, _T("*"), Deep );

		utl::JoinUnique( m_srcDeepFolderPaths, subDirPaths.begin(), subDirPaths.end() );
	}
	fs::SortPaths( m_srcDeepFolderPaths );

	// deep paste folders
	m_relFolderPaths.push_back( std::tstring() );		// the "." entry (shallow paste)
	RegisterFolderImage( m_destDirPath );

	for ( fs::CPath relFolderPath = m_srcCommonFolderPath.GetFilename(), parentPath = m_srcCommonFolderPath;
		  !parentPath.IsEmpty() && !path::IsRoot( parentPath.GetPtr() );
		  relFolderPath = path::Combine( parentPath.GetNameExt(), relFolderPath.GetPtr() ) )
	{
		m_relFolderPaths.push_back( relFolderPath );
		RegisterFolderImage( parentPath );

		parentPath = parentPath.GetParentPath();
	}
}

void CDropFilesModel::RegisterFolderImage( const fs::CPath& folderPath )
{
	if ( HICON hFolderIcon = shell::GetFileSysIcon( folderPath.GetPtr(), SHGFI_SMALLICON ) )
		m_pImageStore->RegisterIcon( BaseImageId + static_cast< UINT >( m_relFolderPaths.size() - 1 ), CIcon::NewIcon( hFolderIcon ) );		// match the folder index
}

std::tstring CDropFilesModel::FormatDropCounts( void ) const
{
	size_t folderCount = 0, fileCount = 0;

	for ( std::vector< fs::CPath >::const_iterator itDropPath = m_dropPaths.begin(); itDropPath != m_dropPaths.end(); ++itDropPath )
		if ( fs::IsValidDirectory( itDropPath->GetPtr() ) )
			++folderCount;
		else if ( fs::IsValidFile( itDropPath->GetPtr() ) )
			++fileCount;

	std::vector< std::tstring > dropItems;

	if ( folderCount != 0 )
		dropItems.push_back( str::Format( _T("%d folders"), folderCount ) );

	if ( fileCount != 0 )
		dropItems.push_back( str::Format( _T("%d files"), fileCount ) );

	return str::Join( dropItems, _T(", ") );
}

CBitmap* CDropFilesModel::GetItemInfo( std::tstring& rItemText, size_t fldPos ) const
{
	rItemText = m_relFolderPaths[ fldPos ].Get();

	if ( !rItemText.empty() )
	{
		str::Replace( rItemText, _T("\\"), _T("/") );
		str::Replace( rItemText, _T("&"), _T("&&") );
		rItemText += _T("/");
	}

	rItemText += _T("*");

	return m_pImageStore->RetrieveBitmap( BaseImageId + static_cast< UINT >( fldPos ), ::GetSysColor( COLOR_MENU ) );
}

bool CDropFilesModel::CreateFolders( const std::vector< fs::CPath >& srcFolderPaths )
{
	std::vector< fs::CPath > destPaths; destPaths.reserve( srcFolderPaths.size() );

	size_t createdCount = 0;
	std::deque< std::tstring > errorMsgs;

	for ( std::vector< fs::CPath >::const_iterator itSrcFolderPath = srcFolderPaths.begin(); itSrcFolderPath != srcFolderPaths.end(); ++itSrcFolderPath )
	{
		fs::CPath targetFolderPath = MakeDeepTargetFilePath( *itSrcFolderPath, fs::CPath() );

		if ( !fs::IsValidDirectory( targetFolderPath.GetPtr() ) )
			try
			{
				fs::thr::CreateDirPath( targetFolderPath.GetPtr() );
				++createdCount;
			}
			catch ( CRuntimeException& exc )
			{
				errorMsgs.push_back( exc.GetMessage() );
			}
	}

	if ( !errorMsgs.empty() )
	{
		errorMsgs.push_front( _T("Error creating folder structure:\r\n") );
		return app::ReportError( str::Join( errorMsgs, _T("\r\n") ) );
	}
	else if ( createdCount != srcFolderPaths.size() )	// created fewer directories?
		app::ReportError( str::Format( _T("Created %d new folders out of %d total folders on clipboard."), createdCount, srcFolderPaths.size() ), app::Info );

	return true;
}

bool CDropFilesModel::PasteDeep( const fs::CPath& relFolderPath, CWnd* pParentOwner )
{
	std::vector< fs::CPath > destPaths; destPaths.reserve( m_dropPaths.size() );

	for ( std::vector< fs::CPath >::const_iterator itDropPath = m_dropPaths.begin(); itDropPath != m_dropPaths.end(); ++itDropPath )
		destPaths.push_back( MakeDeepTargetFilePath( *itDropPath, relFolderPath ) );

	PasteOperation pasteOperation = GetPasteOperation();
	CLogger* pLogger = app::GetLogger();
	if ( pLogger != NULL )
	{
		std::tstring message = str::Format( _T("%s: %d files to folder %s:\n"),
			GetTags_PasteOperation().FormatUi( pasteOperation ).c_str(),
			m_dropPaths.size(),
			( m_destDirPath / relFolderPath ).GetPtr(),
			m_destDirPath );

		message += str::Join( m_dropPaths, _T("\n") );
		pLogger->LogString( message );
	}

	bool succeeded = false;

	switch ( pasteOperation )
	{
		case PasteCopyFiles:	succeeded = shell::CopyFiles( m_dropPaths, destPaths, pParentOwner ); break;
		case PasteMoveFiles:	succeeded = shell::MoveFiles( m_dropPaths, destPaths, pParentOwner ); break;
		default:
			return false;
	}

	if ( !succeeded && pLogger != NULL )
		pLogger->LogLine( _T(" * ERROR"), false );

	CClipboard::CopyToLines( destPaths, pParentOwner );		// clear clipboard after Paste, and add the destination paths as text
	return succeeded;
}

fs::CPath CDropFilesModel::MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& relFolderPath ) const
{
	fs::CPath srcParentDirPath = srcFilePath.GetParentPath();
	std::tstring targetRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), m_srcCommonFolderPath.GetPtr() );

	fs::CPath targetFullPath = m_destDirPath / relFolderPath / targetRelPath / srcFilePath.GetFilename();
	return targetFullPath;
}

CDropFilesModel::PasteOperation CDropFilesModel::GetPasteOperation( void ) const
{
	if ( HasFlag( m_dropEffect, DROPEFFECT_MOVE ) )
		return PasteMoveFiles;
	else if ( HasFlag( m_dropEffect, DROPEFFECT_COPY ) )
		return PasteCopyFiles;

	return PasteNone;
}

const CEnumTags& CDropFilesModel::GetTags_PasteOperation( void )
{
	static const CEnumTags tags( _T("n/a|DEEP PASTE - COPY|DEEP PASTE - CUT") );
	return tags;
}
