
#include "stdafx.h"
#include "DropFilesModel.h"
#include "utl/AppTools.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/Logger.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/ShellTypes.h"
#include "utl/UI/ShellUtilities.h"

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
	m_srcPaths.clear();
	m_dropEffect = DROPEFFECT_NONE;

	m_srcParentPath.Clear();
	m_relFolderPaths.clear();
	m_pImageStore.reset();
}

void CDropFilesModel::Init( const std::vector< fs::CPath >& srcPaths, DROPEFFECT dropEffect )
{
	m_srcPaths = srcPaths;
	m_dropEffect = dropEffect;

	m_srcParentPath = path::ExtractCommonParentPath( m_srcPaths );

	m_pImageStore.reset( new CImageStore );

	m_relFolderPaths.push_back( std::tstring() );		// the "." entry (shallow paste)
	RegisterFolderImage( m_destDirPath );

	for ( fs::CPath relFolderPath = m_srcParentPath.GetFilename(), parentPath = m_srcParentPath;
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

void CDropFilesModel::BuildFromClipboard( void )
{
	Clear();

	std::vector< fs::CPath > srcPaths;
	DROPEFFECT dropEffect = CClipboard::QueryDropFilePaths( srcPaths );

	if ( !srcPaths.empty() )
		Init( srcPaths, dropEffect );
}

bool CDropFilesModel::PasteDeep( const fs::CPath& relFolderPath, CWnd* pParentOwner )
{
	std::vector< std::tstring > srcPaths, destPaths;
	srcPaths.reserve( m_srcPaths.size() );
	destPaths.reserve( m_srcPaths.size() );

	for ( std::vector< fs::CPath >::const_iterator itSrcPath = m_srcPaths.begin(); itSrcPath != m_srcPaths.end(); ++itSrcPath )
	{
		srcPaths.push_back( itSrcPath->Get() );
		destPaths.push_back( MakeDeepTargetFilePath( *itSrcPath, relFolderPath ).Get() );
	}

	PasteOperation pasteOperation = GetPasteOperation();
	CLogger* pLogger = app::GetLogger();
	if ( pLogger != NULL )
	{
		std::tstring message = str::Format( _T("%s: %d files to folder %s:\n"),
			GetTags_PasteOperation().FormatUi( pasteOperation ).c_str(),
			srcPaths.size(),
			( m_destDirPath / relFolderPath ).GetPtr(),
			m_destDirPath );

		message += str::JoinLines( srcPaths, _T("\n") );
		pLogger->LogString( message );
	}

	bool succeeded = false;

	switch ( pasteOperation )
	{
		case PasteCopyFiles:	succeeded = shell::CopyFiles( srcPaths, destPaths, pParentOwner ); break;
		case PasteMoveFiles:	succeeded = shell::MoveFiles( srcPaths, destPaths, pParentOwner ); break;
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
	std::tstring targetRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), m_srcParentPath.GetPtr() );

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
