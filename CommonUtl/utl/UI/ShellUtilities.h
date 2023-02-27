#ifndef ShellUtilities_h
#define ShellUtilities_h
#pragma once

#include "FileSystem_fwd.h"
#include <afxole.h>


namespace utl
{
	// forward declarations - required for C++ 14+ compilation
	template< typename DestContainerT, typename SrcContainerT, typename ConvertUnaryFunc >
	void Assign( DestContainerT& rDestItems, const SrcContainerT& srcItems, ConvertUnaryFunc cvtFunc );
}


namespace shell
{
	template< typename PathType >
	void QueryDroppedFiles( std::vector< PathType >& rFilePaths, HDROP hDropInfo, SortType sortType = NoSort );		// works with std::tstring, fs::CPath, fs::CFlexPath

	CComPtr<IStream> DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete = true );
}


namespace shell
{
	// file operations

	bool MoveFiles( const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >& destPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool MoveFiles( const std::vector< fs::CPath >& srcPaths, const fs::CPath& destFolderPath, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool CopyFiles( const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >& destPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool CopyFiles( const std::vector< fs::CPath >& srcPaths, const fs::CPath& destFolderPath, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool DeleteFiles( const std::vector< fs::CPath >& srcPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool DeleteFile( const fs::CPath& filePath, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool DoFileOperation( UINT shellOp, const std::vector< fs::CPath >& srcPaths, const std::vector< fs::CPath >* pDestPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool AnyOperationAborted( void );		// aborted by user?

	// Recycle Bin operations
	bool UndeleteFile( const fs::CPath& delFilePath, CWnd* pWnd = AfxGetMainWnd() );
	size_t UndeleteFiles( const std::vector< fs::CPath >& delFilePaths, CWnd* pWnd = AfxGetMainWnd(), std::vector< fs::CPath >* pErrorFilePaths = nullptr );


	// empty sub-folders cleanup after move/delete files
	size_t DeleteEmptySubdirs( const fs::CPath& topDirPath, const fs::CPath& subFolderPath, std::vector< fs::CPath >* pDelFolderPaths = nullptr );
	size_t DeleteEmptyMultiSubdirs( const fs::CPath& topDirPath, std::vector< fs::CPath > subFolderPaths, std::vector< fs::CPath >* pDelFolderPaths = nullptr );


	HINSTANCE Execute( CWnd* pParentWnd, const TCHAR* pFilePath, const TCHAR* pParams = nullptr, DWORD mask = 0, const TCHAR* pVerb = nullptr,
					   const TCHAR* pUseClassName = nullptr, const TCHAR* pUseExtType = nullptr, int cmdShow = SW_SHOWNORMAL );

	std::tstring GetClassAssociatedWith( const TCHAR* pExt );
	const TCHAR* GetExecErrorString( HINSTANCE hInstExec );
	std::tstring GetExecErrorMessage( const TCHAR* pExeFullPath, HINSTANCE hInstExec );

	bool ExploreAndSelectFile( const TCHAR* pFullPath );
}


namespace shell
{
	template< typename PathType >
	void QueryDroppedFiles( std::vector< PathType >& rFilePaths, HDROP hDropInfo, SortType sortType /*= NoSort*/ )		// works with std::tstring, fs::CPath, fs::CFlexPath
	{
		ASSERT_PTR( hDropInfo );
		UINT fileCount = ::DragQueryFile( hDropInfo, (UINT)-1, nullptr, 0 );
		rFilePaths.reserve( fileCount );

		for ( UINT i = 0; i != fileCount; ++i )
		{
			TCHAR filePath[ MAX_PATH ];
			::DragQueryFile( hDropInfo, i, filePath, MAX_PATH );

			rFilePaths.push_back( std::tstring( filePath ) );
		}

		::DragFinish( hDropInfo );

		if ( sortType != NoSort )
			fs::SortPathsDirsFirst( rFilePaths, SortAscending == sortType );
	}
}


#include "StringCvt.h"


class COleDataSource;


namespace shell
{
	// returns the size of TCHARs for the buffer
	//
	template< typename ContainerType >
	size_t ComputeFileListBufferSize( const ContainerType& fullPaths )
	{
		size_t bufferSize = 1;			// reserve space for buffer terminating EOS
		if ( fullPaths.empty() )
			++bufferSize;				// empty buffer must contain zero + terminating_zero
		else
			for ( typename ContainerType::const_iterator itPath = fullPaths.begin(); itPath != fullPaths.end(); ++itPath )
				bufferSize += str::traits::GetLength( *itPath ) + 1;

		return bufferSize;
	}


	// builds the file list buffer with the following layout:
	//	  fp1 + zero + ... + fpN + zero + terminating_zero
	//
	// works with container of std::tstring or CString, using str::traits conversion functions
	//
	template< typename ContainerType >
	void BuildFileListBuffer( std::vector< TCHAR >& rFileListBuffer, const ContainerType& fullPaths )
	{
		size_t bufferSize = ComputeFileListBufferSize( fullPaths );
		rFileListBuffer.resize( bufferSize );

		UINT pos = 0;

		if ( fullPaths.empty() )
			rFileListBuffer[ pos++ ] = _T('\0');		// empty buffer must EOS + terminating EOS
		else
			// setup the file list buffer content
			for ( typename ContainerType::const_iterator itPath = fullPaths.begin(); itPath != fullPaths.end(); ++itPath )
			{
				_tcscpy( &rFileListBuffer[ pos ], str::traits::GetCharPtr( *itPath ) );
				pos += (UINT)str::traits::GetLength( *itPath ) + 1;
			}

		rFileListBuffer[ pos ] = _T('\0');				// add the extra EOS
	}


	namespace xfer
	{
		// files clipboard and drag&drop

		extern CLIPFORMAT cfHDROP;
		extern CLIPFORMAT cfFileGroupDescriptor;


		bool IsValidClipFormat( UINT cfFormat );


		// data transfer build helpers
		HGLOBAL BuildHDrop( const std::vector< fs::CPath >& srcFiles );						// works well for drag&drop; minimum neccessary for drag&drop files
		HGLOBAL BuildFileGroupDescriptor( const std::vector< fs::CPath >& srcFiles );

		// template version for compatibility with fs::CPath, fs::CFlexPath, CString
		template< typename SrcContainerT >
		void CacheDragDropSrcFiles( COleDataSource& rDataSource, const SrcContainerT& srcFiles )
		{
			std::vector< fs::CPath > filePaths;
			utl::Assign( filePaths, srcFiles, func::tor::StringOf() );

			if ( HGLOBAL hDrop = BuildHDrop( filePaths ) )		// minimum neccessary for drag&drop files
				rDataSource.CacheGlobalData( cfHDROP, hDrop );

			if ( HGLOBAL hFileGroupDescr = BuildFileGroupDescriptor( filePaths ) )
				rDataSource.CacheGlobalData( cfFileGroupDescriptor, hFileGroupDescr );
		}
	}
}


#endif // ShellUtilities_h
