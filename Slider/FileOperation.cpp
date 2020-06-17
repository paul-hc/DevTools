
#include "stdafx.h"
#include "FileOperation.h"
#include "ImageArchiveStg.h"
#include "utl/EnumTags.h"
#include "utl/Logger.h"
#include "utl/RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CFileOperation::Copy( const fs::CFlexPath& srcFilePath, const fs::CFlexPath& destFilePath ) throws_( CException* )
{
	AddLogMessage( CopyFile, srcFilePath, &destFilePath );

	if ( srcFilePath.FileExist() )
		if ( !srcFilePath.IsComplexPath() && !destFilePath.IsComplexPath() )
		{
			ReleaseStgs( srcFilePath, &destFilePath );
			if ( !::CopyFile( srcFilePath.GetPtr(), destFilePath.GetPtr(), FALSE ) )
				return HandleLastError();
		}
		else
		{	// source file is an embedded file in a compound file
			try
			{
				std::auto_ptr< CFile > pSrcFile = CImageArchiveStg::Factory()->OpenFlexImageFile( srcFilePath, CFile::modeRead );
				std::auto_ptr< CFile > pDestFile = CImageArchiveStg::Factory()->OpenFlexImageFile( destFilePath, CFile::modeCreate | CFile::modeWrite );

				if ( pSrcFile.get() != NULL && pDestFile.get() != NULL )
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
			ReleaseStgs( srcFilePath, &destFilePath );
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
			IImageArchiveStg* pImageStg = CImageArchiveStg::Factory()->AcquireStorage( filePath.GetPhysicalPath(), STGM_WRITE );
			if ( NULL == pImageStg || !pImageStg->GetDocStorage()->DeleteStream( filePath.GetEmbeddedPathPtr() ) )
				return HandleError( str::Format( _T("Cannot delete the embedded file '%s'"), filePath.GetPtr() ) );
		}
		else
		{
			ReleaseStgs( filePath );
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

void CFileOperation::ReleaseStgs( const fs::CPath& srcFilePath, const fs::CPath* pDestFilePath /*= NULL*/ )
{
	CImageArchiveStg::Factory()->ReleaseStorage( srcFilePath );

	if ( pDestFilePath != NULL )
		CImageArchiveStg::Factory()->ReleaseStorage( *pDestFilePath );
}

void CFileOperation::AddLogMessage( Operation operation, const fs::CPath& srcFilePath, const fs::CPath* pDestFilePath /*= NULL*/ )
{
	std::tostringstream oss;
	oss << GetTags_Operation().GetUiTags()[ operation ] << _T(": ") << srcFilePath.Get();
	if ( pDestFilePath != NULL )
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
	for ( std::vector< std::tstring >::const_iterator itLogLine = m_logLines.begin(); itLogLine != m_logLines.end(); ++itLogLine )
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
	std::vector< TCHAR > errorMessage( 512 );
	::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		&errorMessage.front(),
		0,
		NULL );

	AugmentLogError( &errorMessage.front() );
	return HandleError( &errorMessage.front() );
}
