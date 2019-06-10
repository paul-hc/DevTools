
#include "stdafx.h"
#include "DropFilesModel.h"
#include "FileGroupCommands.h"
#include "AppCmdService.h"
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
	m_relFolderPathSeq.clear();
	m_pImageStore.reset();
}

const CEnumTags& CDropFilesModel::GetTags_PasteOperation( void )
{
	static const CEnumTags tags( _T("n/a|DEEP PASTE - COPY|DEEP PASTE - CUT") );
	return tags;
}

CDropFilesModel::PasteOperation CDropFilesModel::GetPasteOperation( void ) const
{
	if ( HasFlag( m_dropEffect, DROPEFFECT_MOVE ) )
		return PasteMoveFiles;
	else if ( HasFlag( m_dropEffect, DROPEFFECT_COPY ) )
		return PasteCopyFiles;

	return PasteNone;
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

	InitSrcFolders();
	InitDeepPasteFolders();
}

void CDropFilesModel::InitSrcFolders( void )
{
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
}

void CDropFilesModel::InitDeepPasteFolders( void )
{
	m_pImageStore.reset( new CImageStore );

	m_relFolderPathSeq.push_back( std::tstring() );		// the "." entry (shallow paste)
	RegisterFolderImage( m_destDirPath );

	for ( fs::CPath relFolderPath = m_srcCommonFolderPath.GetFilename(), parentPath = m_srcCommonFolderPath;
		  !parentPath.IsEmpty() && !path::IsRoot( parentPath.GetPtr() );
		  relFolderPath = path::Combine( parentPath.GetNameExt(), relFolderPath.GetPtr() ) )
	{
		m_relFolderPathSeq.push_back( relFolderPath );
		RegisterFolderImage( parentPath );

		parentPath = parentPath.GetParentPath();
	}
}

void CDropFilesModel::RegisterFolderImage( const fs::CPath& folderPath )
{
	if ( HICON hFolderIcon = shell::GetFileSysIcon( folderPath.GetPtr(), SHGFI_SMALLICON ) )
		m_pImageStore->RegisterIcon( BaseImageId + static_cast< UINT >( m_relFolderPathSeq.size() - 1 ), CIcon::NewIcon( hFolderIcon ) );		// match the folder index
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

CBitmap* CDropFilesModel::GetRelFolderItemInfo( std::tstring& rItemText, size_t fldSeqPos ) const
{
	rItemText = m_relFolderPathSeq[ fldSeqPos ].Get();

	if ( !rItemText.empty() )
	{
		str::Replace( rItemText, _T("\\"), _T("/") );
		str::Replace( rItemText, _T("&"), _T("&&") );
		rItemText += _T("/");
	}

	rItemText += _T("*");

	return m_pImageStore->RetrieveBitmap( BaseImageId + static_cast< UINT >( fldSeqPos ), ::GetSysColor( COLOR_MENU ) );
}

fs::CPath CDropFilesModel::MakeDestFilePath( const fs::CPath& srcFilePath, const fs::CPath& destDirPath ) const
{
	fs::CPath srcParentDirPath = srcFilePath.GetParentPath();
	std::tstring targetRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), m_srcCommonFolderPath.GetPtr() );

	fs::CPath targetFullPath = destDirPath / targetRelPath / srcFilePath.GetFilename();
	return targetFullPath;
}

bool CDropFilesModel::CreateFolders( const std::vector< fs::CPath >& srcFolderPaths, RecursionDepth depth )
{
	return app::GetCmdSvc()->SafeExecuteCmd( new CCreateFoldersCmd( srcFolderPaths, m_destDirPath, Shallow == depth ? CCreateFoldersCmd::PasteDirs : CCreateFoldersCmd::PasteDeepStruct ) );
}

bool CDropFilesModel::PasteDeep( const fs::CPath& relFolderPath, CWnd* pParentOwner )
{
	cmd::CBaseDeepTransferFilesCmd* pCmd = NULL;

	switch ( GetPasteOperation() )
	{
		case PasteCopyFiles:	pCmd = new CCopyFilesCmd( m_dropPaths, m_destDirPath, true ); break;
		case PasteMoveFiles:	pCmd = new CMoveFilesCmd( m_dropPaths, m_destDirPath, true ); break;
		default:
			return false;
	}

	pCmd->SetDeepRelDirPath( relFolderPath );
	pCmd->SetParentOwner( pParentOwner );

	fs::CPath destDeepDirPath = pCmd->GetDestDirPath();

	if ( !app::GetCmdSvc()->SafeExecuteCmd( pCmd ) )
		return false;

	std::vector< fs::CPath > destPaths; destPaths.reserve( m_dropPaths.size() );
	for ( std::vector< fs::CPath >::const_iterator itDropPath = m_dropPaths.begin(); itDropPath != m_dropPaths.end(); ++itDropPath )
		destPaths.push_back( MakeDestFilePath( *itDropPath, destDeepDirPath ) );

	std::vector< fs::CPath > newDropPaths;
	DROPEFFECT newDropEffect = CClipboard::QueryDropFilePaths( newDropPaths );
	if ( newDropEffect == m_dropEffect && newDropPaths == m_dropPaths )				// this was a long process: user did not do a new other copy & paste in the meantime?
		CClipboard::CopyToLines( destPaths, pParentOwner );							// clear clipboard after Paste, and add the destination paths as text

	return true;
}
