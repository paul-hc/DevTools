
#include "stdafx.h"
#include "FileOperation.h"
#include "ICatalogStorage.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/Logger.h"
#include "utl/PathUniqueMaker.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/TaskDialog.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace svc
{
	size_t CopyFiles( const std::vector<fs::CFlexPath>& srcFilePaths, const std::vector<fs::CPath>& destFilePaths, RecursionDepth destDepth /*= Shallow*/ )
	{
		REQUIRE( srcFilePaths.size() == destFilePaths.size() );

		CFileOperation fileOp;
		size_t count = 0;

		for ( size_t i = 0; i != srcFilePaths.size(); ++i )
			if ( Shallow == destDepth || fs::CreateDirPath( destFilePaths[ i ].GetParentPath().GetPtr() ) )
				if ( fileOp.Copy( srcFilePaths[ i ], fs::CastFlexPath( destFilePaths[ i ] ) ) )
					++count;

		return count;
	}

	size_t RelocateFiles( const std::vector<fs::CFlexPath>& srcFilePaths, const std::vector<fs::CPath>& destFilePaths, RecursionDepth destDepth /*= Shallow*/ )
	{
		REQUIRE( srcFilePaths.size() == destFilePaths.size() );

		std::vector<fs::CPath> physicalSrc, physicalDest;

		std::vector<fs::CFlexPath> complexSrc;
		std::vector<fs::CPath> complexDest;

		for ( size_t i = 0; i != srcFilePaths.size(); ++i )
			if ( srcFilePaths[ i ].IsComplexPath() )
			{
				complexSrc.push_back( srcFilePaths[ i ] );
				complexDest.push_back( destFilePaths[ i ] );
			}
			else
			{
				physicalSrc.push_back( srcFilePaths[ i ] );
				physicalDest.push_back( destFilePaths[ i ] );
			}

		size_t count = 0;

		if ( !physicalSrc.empty() )
			if ( shell::MoveFiles( physicalSrc, physicalDest ) )
				count += physicalSrc.size();

		if ( !complexSrc.empty() )
			count += svc::CopyFiles( complexSrc, complexDest, destDepth );

		return count;
	}


	bool PickDestImagePaths( std::vector<fs::CPath>& rDestFilePaths, const std::vector<fs::CFlexPath>& srcFilePaths )
	{
		rDestFilePaths.clear();

		if ( 1 == srcFilePaths.size() )		// single file: pick destination file
		{
			fs::CFlexPath srcFilePath = srcFilePaths.front();
			fs::CPath destFilePath = srcFilePath;

			if ( destFilePath.IsComplexPath() )
				destFilePath = destFilePath.GetFilename();

			if ( !shell::BrowseImageFile( destFilePath, shell::FileSaveAs ) )
				return false;

			rDestFilePaths.push_back( destFilePath );
		}
		else
		{
			fs::TDirPath destFolderPath;		// leave empty to pick the previous selected folder
			if ( !shell::PickFolder( destFolderPath, nullptr ) )		// multiple files: pick destination folder
				return false;

			MakeDestFilePaths( rDestFilePaths, srcFilePaths, destFolderPath, Shallow );

			if ( !CheckOverrideExistingFiles( rDestFilePaths ) )
				return false;
		}

		return true;
	}

	bool CheckOverrideExistingFiles( const std::vector<fs::CPath> destFilePaths, const TCHAR* pTitle /*= nullptr*/ )
	{
		std::vector<fs::CPath> existingPaths;
		utl::QueryThat( existingPaths, destFilePaths, pred::FileExist() );

		if ( !existingPaths.empty() )
		{
			if ( str::IsEmpty( pTitle ) )
				pTitle = _T("Confirm Save As");

			ui::CTaskDialog dlg( pTitle, _T("Override existing files?"), str::ToNonBreakingSpace( str::Join( existingPaths, _T("\n") ) ),
								 TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TDF_SIZE_TO_CONTENT );

			dlg.SetMainIcon( TD_WARNING_ICON );
			if ( dlg.DoModal( nullptr ) != IDYES )
				return false;			// cancelled by user
		}

		return true;
	}


	void MakeDestFilePaths( std::vector<fs::CPath>& rDestFilePaths, const std::vector<fs::CFlexPath>& srcFilePaths, const fs::CPath& destFolderPath, RecursionDepth destDepth /*= Shallow*/ )
	{
		utl::Assign( rDestFilePaths, srcFilePaths, func::tor::StringOf() );

		if ( Shallow == destDepth )
		{
			utl::for_each( rDestFilePaths, func::StripToFilename() );

			CPathUniqueMaker uniqueMaker;
			uniqueMaker.UniquifyPaths( rDestFilePaths );
		}
		else
		{
			fs::TDirPath commonDirPath = path::ExtractCommonParentPath( rDestFilePaths );
			if ( !commonDirPath.IsEmpty() )
				path::StripDirPrefixes( rDestFilePaths, commonDirPath.GetPtr() );
			else
				path::StripRootPrefixes( rDestFilePaths );		// ignore drive letter
		}

		utl::for_each( rDestFilePaths, func::PrefixPath( destFolderPath ) );

		ENSURE( srcFilePaths.size() == rDestFilePaths.size() );
	}
}


// CFileOperation implementation

bool CFileOperation::Copy( const fs::CFlexPath& srcFilePath, const fs::CFlexPath& destFilePath ) throws_( CException* )
{
	AddLogMessage( CopyFile, srcFilePath, &destFilePath );

	if ( srcFilePath.FileExist() )
		if ( !srcFilePath.IsComplexPath() && !destFilePath.IsComplexPath() )
		{
			if ( !::CopyFile( srcFilePath.GetPtr(), destFilePath.GetPtr(), FALSE ) )
				return HandleLastError();
		}
		else
		{	// source file is an embedded file in a compound file
			try
			{
				std::auto_ptr<CFile> pSrcFile = CCatalogStorageFactory::Instance()->OpenFlexImageFile( srcFilePath, CFile::modeRead );
				std::auto_ptr<CFile> pDestFile = CCatalogStorageFactory::Instance()->OpenFlexImageFile( destFilePath, CFile::modeCreate | CFile::modeWrite );

				if ( pSrcFile.get() != nullptr && pDestFile.get() != nullptr )
					fs::BufferedCopy( *pDestFile, *pSrcFile );
				else
					return HandleError( str::Format( _T("Cannot find embedded file: %s"), srcFilePath.GetPtr() ) );
			}
			catch ( CException* pExc )
			{
				return HandleError( pExc );
			}
		}
	else
		return HandleError( str::Format( _T("Cannot find source file '%s'"), srcFilePath.GetPtr() ) );

	return true;
}

bool CFileOperation::Move( const fs::CFlexPath& srcFilePath, const fs::CFlexPath& destFilePath ) throws_( CException* )
{
	AddLogMessage( MoveFile, srcFilePath, &destFilePath );

	if ( srcFilePath.FileExist() )
		if ( !srcFilePath.IsComplexPath() )
		{
			if ( !::MoveFile( srcFilePath.GetPtr(), destFilePath.GetPtr() ) )
				return HandleLastError();
		}
		else
			return Copy( srcFilePath, destFilePath );		// source file is an embedded file -> just do a copy defensively
	else
		return HandleError( str::Format( _T("Cannot find source file '%s'"), srcFilePath.GetPtr() ) );

	return true;
}

bool CFileOperation::Delete( const fs::CFlexPath& filePath ) throws_( CException* )
{
	AddLogMessage( DeleteFile, filePath );

	if ( filePath.FileExist() )
		if ( filePath.IsComplexPath() )
		{
			CComPtr<ICatalogStorage> pCatalogStorage = CCatalogStorageFactory::Instance()->AcquireStorage( filePath.GetPhysicalPath(), STGM_READWRITE );

			if ( nullptr == pCatalogStorage || !pCatalogStorage->GetDocStorage()->DeleteStream( filePath.GetEmbeddedPathPtr() ) )
				return HandleError( str::Format( _T("Cannot delete the embedded file '%s'"), filePath.GetPtr() ) );
		}
		else
		{
			if ( !::DeleteFile( filePath.GetPtr() ) )
				return HandleLastError();
		}
	else
		return HandleError( str::Format( _T("Cannot find source file '%s'"), filePath.GetPtr() ) );

	return true;
}

const CEnumTags& CFileOperation::GetTags_Operation( void )
{
	static const CEnumTags tags( _T("COPY|MOVE|DELETE") );
	return tags;
}

void CFileOperation::AddLogMessage( Operation operation, const fs::CPath& srcFilePath, const fs::CPath* pDestFilePath /*= nullptr*/ )
{
	std::tostringstream oss;
	oss << GetTags_Operation().GetUiTags()[ operation ] << _T(": ") << srcFilePath.Get();
	if ( pDestFilePath != nullptr )
		oss << _T(" -> ") << pDestFilePath->Get();

	m_logLines.push_back( oss.str() );
}

void CFileOperation::AugmentLogError( const std::tstring& errorMessage )
{
	ASSERT( !m_logLines.empty() );
	std::tstring& rLine = m_logLines.back();
	rLine += _T(" * FAILED ");

	if ( !IsThrowMode() )
		rLine += _T("\n\t") + errorMessage;

	// else:in throw mode the error message is logged by the catch block

	TRACE( _T("* %s\n"), rLine.c_str() );
}

void CFileOperation::Log( CLogger& rLogger )
{
	for ( std::vector<std::tstring>::const_iterator itLogLine = m_logLines.begin(); itLogLine != m_logLines.end(); ++itLogLine )
		rLogger.Log( itLogLine->c_str() );

	ClearLog();
}

bool CFileOperation::HandleError( CException* pExc )
{
	AugmentLogError( mfc::CRuntimeException::MessageOf( *pExc ) );
	if ( !IsThrowMode() )
	{
		pExc->Delete();
		return false;
	}
	throw pExc;
}

bool CFileOperation::HandleError( const std::tstring& errorMessage )
{
	AugmentLogError( errorMessage );
	if ( !IsThrowMode() )
		return false;

	throw new mfc::CRuntimeException( errorMessage );
}

bool CFileOperation::HandleLastError( void )
{
	std::vector<TCHAR> errorMessage( 512 );

	::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		GetLastError(),
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		&errorMessage.front(),
		0,
		nullptr );

	AugmentLogError( &errorMessage.front() );
	return HandleError( &errorMessage.front() );
}
