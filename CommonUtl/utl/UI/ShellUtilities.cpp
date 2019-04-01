
#include "stdafx.h"
#include "ShellUtilities.h"
#include "ImagingWic.h"
#include "Registry.h"
#include "ContainerUtilities.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr< IStream > DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete /*= true*/ )
	{
		CComPtr< IStream > pDestStream;
		ULARGE_INTEGER size;
		if ( HR_OK( ::IStream_Size( pSrcStream, &size ) ) )
		{
			ASSERT( 0 == size.u.HighPart );				// shouldn't be huge

			if ( HR_OK( ::CreateStreamOnHGlobal( NULL, autoDelete, &pDestStream ) ) )
				if ( HR_OK( ::IStream_Copy( pSrcStream, pDestStream, size.u.LowPart ) ) )
					return pDestStream;
		}

		return NULL;
	}
}


namespace shell
{
	bool DoFileOperation( UINT shellOp, const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >* pDestPaths,
						  CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		static const TCHAR* pShellOpTags[] = { _T("Move Files"), _T("Copy Files"), _T("Delete Files"), _T("Rename Files") };	// FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME
		ASSERT( shellOp <= FO_RENAME );

		ASSERT( !srcPaths.empty() );

		std::vector< TCHAR > srcBuffer, destBuffer;
		shell::BuildFileListBuffer( srcBuffer, srcPaths );
		if ( pDestPaths != NULL )
			shell::BuildFileListBuffer( destBuffer, *pDestPaths );

		SHFILEOPSTRUCT fileOp;
		fileOp.hwnd = pWnd->GetSafeHwnd();
		fileOp.wFunc = shellOp;
		fileOp.pFrom = &srcBuffer.front();
		fileOp.pTo = !destBuffer.empty() ? &destBuffer.front() : NULL;
		fileOp.fFlags = flags;
		fileOp.lpszProgressTitle = pShellOpTags[ shellOp ];

		if ( shellOp != FO_DELETE )
			if ( pDestPaths != NULL && pDestPaths->size() == srcPaths.size() )
				SetFlag( fileOp.fFlags, FOF_MULTIDESTFILES );							// each SRC file has a DEST file

		return 0 == ::SHFileOperation( &fileOp ) && !fileOp.fAnyOperationsAborted;		// true for success AND not canceled by user
	}

	bool MoveFiles( const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >& destPaths,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		return DoFileOperation( FO_MOVE, srcPaths, &destPaths, pWnd, flags );
	}

	bool MoveFiles( const std::vector< fs::CPath >& srcPaths, const fs::CPath& destFolderPath,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		std::vector< fs::CPath > destPaths( 1, destFolderPath );
		return DoFileOperation( FO_MOVE, srcPaths, &destPaths, pWnd, flags );
	}

	bool CopyFiles( const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >& destPaths,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		return DoFileOperation( FO_COPY, srcPaths, &destPaths, pWnd, flags );
	}

	bool CopyFiles( const std::vector< fs::CPath >& srcPaths, const fs::CPath& destFolderPath, CWnd* pWnd /*= AfxGetMainWnd()*/,
					FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		std::vector< fs::CPath > destPaths( 1, destFolderPath );
		return DoFileOperation( FO_COPY, srcPaths, &destPaths, pWnd, flags );
	}

	bool DeleteFiles( const std::vector< fs::CPath >& srcPaths, CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		return DoFileOperation( FO_DELETE, srcPaths, NULL, pWnd, flags );
	}


	// if SEE_MASK_FLAG_DDEWAIT mask is specified, the call will be modal for DDE conversations;
	// otherwise (the default), the function returns before the DDE conversation is finished.

	HINSTANCE Execute( CWnd* pParentWnd, const TCHAR* pFilePath, const TCHAR* pParams /*= NULL*/, DWORD mask /*= 0*/, const TCHAR* pVerb /*= NULL*/,
					   const TCHAR* pUseClassName /*= NULL*/, const TCHAR* pUseExtType /*= NULL*/, int cmdShow /*= SW_SHOWNORMAL*/ )
	{
		SHELLEXECUTEINFO shellInfo;

		memset( &shellInfo, 0, sizeof( shellInfo ) );
		shellInfo.cbSize = sizeof( shellInfo );
		shellInfo.fMask = mask;
		shellInfo.hwnd = pParentWnd->GetSafeHwnd();
		shellInfo.lpVerb = pVerb;			// Like "[Open(\"%1\")]"
		shellInfo.lpFile = pFilePath;
		shellInfo.lpParameters = pParams;
		shellInfo.lpDirectory = NULL;
		shellInfo.nShow = cmdShow;

		std::tstring className;

		if ( pUseClassName != NULL  )
			className = pUseClassName;
		else if ( pUseExtType != NULL )
			className = GetClassAssociatedWith( pUseExtType );

		if ( !className.empty() )
		{
			shellInfo.fMask |= SEE_MASK_CLASSNAME;
			shellInfo.lpClass = (TCHAR*)className.c_str();
		}

		::ShellExecuteEx( &shellInfo );
		return shellInfo.hInstApp;
	}

	std::tstring GetClassAssociatedWith( const TCHAR* pExt )
	{
		ASSERT( !str::IsEmpty( pExt ) );

		std::tstring className;
		reg::CKey key;
		if ( key.Open( HKEY_CLASSES_ROOT, pExt, KEY_READ ) )
			className = key.ReadStringValue( NULL );			// default value for the key

		return className;
	}

	const TCHAR* GetExecErrorString( HINSTANCE hInstExec )
	{
		switch ( (DWORD_PTR)hInstExec )
		{
			case SE_ERR_FNF:			return _T("File not found");
			case SE_ERR_PNF:			return _T("Path not found");
			case SE_ERR_ACCESSDENIED:	return _T("Access denied");
			case SE_ERR_OOM:			return _T("Out of memory");
			case SE_ERR_DLLNOTFOUND:	return _T("Dynamic-link library not found");
			case SE_ERR_SHARE:			return _T("Cannot share open file");
			case SE_ERR_ASSOCINCOMPLETE:return _T("File association information not complete");
			case SE_ERR_DDETIMEOUT:		return _T("DDE operation timed out");
			case SE_ERR_DDEFAIL:		return _T("DDE operation failed");
			case SE_ERR_DDEBUSY:		return _T("DDE operation busy");
			case SE_ERR_NOASSOC:		return _T("File association not available");
			default:
				return (DWORD_PTR)hInstExec < HINSTANCE_ERROR ? _T("Unknown error") : NULL;
		}
	}

	std::tstring GetExecErrorMessage( const TCHAR* pExeFullPath, HINSTANCE hInstExec )
	{
		return str::Format( _T("Cannot run specified file !\n\n[%s]\n\nError: %s"), pExeFullPath, GetExecErrorString( hInstExec ) );
	}

	bool ExploreAndSelectFile( const TCHAR* pFullPath )
	{
		std::tstring commandLine = str::Format( _T("Explorer.exe /select, \"%s\""), pFullPath );
		DWORD_PTR hInstExec = WinExec( str::ToUtf8( commandLine.c_str() ).c_str(), SW_SHOWNORMAL );

		if ( hInstExec < HINSTANCE_ERROR )
		{
			AfxMessageBox( GetExecErrorMessage( pFullPath, (HINSTANCE)hInstExec ).c_str() );
			return false;
		}

		return true;
	}

} //namespace shell


namespace shell
{
	namespace xfer
	{
		CLIPFORMAT cfHDROP = CF_HDROP;
		CLIPFORMAT cfFileGroupDescriptor = static_cast< CLIPFORMAT >( RegisterClipboardFormat( CFSTR_FILEDESCRIPTOR ) );


		bool IsValidClipFormat( UINT cfFormat )
		{
			return
				cfHDROP == cfFormat ||
				cfFileGroupDescriptor == cfFormat;
		}

		HGLOBAL BuildHDrop( const std::vector< fs::CPath >& srcFiles )
		{
			if ( srcFiles.empty() )
				return NULL;

			std::vector< TCHAR > srcBuffer;
			shell::BuildFileListBuffer( srcBuffer, srcFiles );

			size_t byteSize = sizeof( TCHAR ) * srcBuffer.size();

			// allocate space for DROPFILE structure plus the number of file and one extra byte for final NULL terminator
			HGLOBAL hGlobal = ::GlobalAlloc( GHND | GMEM_SHARE, ( sizeof( DROPFILES ) + byteSize ) );

			if ( hGlobal != NULL )
				if ( DROPFILES* pDropFiles = (DROPFILES*)::GlobalLock( hGlobal ) )
				{
					pDropFiles->pFiles = sizeof( DROPFILES );					// set the offset where the starting point of the file start
					pDropFiles->fWide = sizeof( TCHAR ) != sizeof( char );		// false for ANSI, true for WIDE

					TCHAR* pDestFileBuffer = reinterpret_cast< TCHAR* >( pDropFiles + 1 );			// p + 1 means past DROPFILES header

					utl::Copy( srcBuffer.begin(), srcBuffer.end(), pDestFileBuffer );

					::GlobalUnlock( hGlobal );
				}

			return hGlobal;
		}

		HGLOBAL BuildFileGroupDescriptor( const std::vector< fs::CPath >& srcFiles )
		{
			size_t fileCount = srcFiles.size();
			HGLOBAL hGlobal = NULL;

			if ( fileCount != 0 )
			{
				hGlobal = ::GlobalAlloc( GMEM_FIXED, sizeof( FILEGROUPDESCRIPTOR ) + ( fileCount - 1 ) * sizeof( FILEDESCRIPTOR ) );
				if ( hGlobal != NULL )
				{
					if ( FILEGROUPDESCRIPTOR* pFileGroupDescr = (FILEGROUPDESCRIPTOR*)::GlobalLock( hGlobal ) )
					{
						pFileGroupDescr->cItems = static_cast< UINT >( fileCount );
						for ( size_t i = 0; i != fileCount; ++i )
						{
							pFileGroupDescr->fgd[ i ].dwFlags = FD_ATTRIBUTES;
							pFileGroupDescr->fgd[ i ].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
							_tcscpy( pFileGroupDescr->fgd[ i ].cFileName, srcFiles[ i ].GetPtr() );
						}
						::GlobalUnlock( hGlobal );
					}
				}
			}
			return hGlobal;
		}

	} //namespace xfer

} //namespace shell
