#ifndef ShellUtilities_h
#define ShellUtilities_h
#pragma once


namespace shell
{
	template< typename PathType >
	void QueryDroppedFiles( std::vector< PathType >& rFilePaths, HDROP hDropInfo, SortType sortType = NoSort );		// works with std::tstring, fs::CPath, fs::CFlexPath

	CComPtr< IStream > DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete = true );
}


namespace shell
{
	// file operations

	bool MoveFiles( const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >& destPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool MoveFiles( const std::vector< std::tstring >& srcPaths, const std::tstring& destFolderPath, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool CopyFiles( const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >& destPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );
	bool CopyFiles( const std::vector< std::tstring >& srcPaths, const std::tstring& destFolderPath, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool DeleteFiles( const std::vector< std::tstring >& srcPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );

	bool DoFileOperation( UINT shellOp, const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >* pDestPaths, CWnd* pWnd = AfxGetMainWnd(), FILEOP_FLAGS flags = FOF_ALLOWUNDO );


	HINSTANCE Execute( CWnd* pParentWnd, const TCHAR* pFilePath, const TCHAR* pParams = NULL, DWORD mask = 0, const TCHAR* pVerb = NULL,
					   const TCHAR* pUseClassName = NULL, const TCHAR* pUseExtType = NULL, int cmdShow = SW_SHOWNORMAL );

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
		UINT fileCount = ::DragQueryFile( hDropInfo, (UINT)-1, NULL, 0 );
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
		HGLOBAL BuildHDrop( const std::vector< std::tstring >& srcFiles );						// works well for drag&drop; minimum neccessary for drag&drop files
		HGLOBAL BuildFileGroupDescriptor( const std::vector< std::tstring >& srcFiles );

		// template version for compatibility with fs::CPath, fs::CFlexPath, CString
		template< typename SrcContainerT >
		void CacheDragDropSrcFiles( COleDataSource& rDataSource, const SrcContainerT& srcFiles )
		{
			std::vector< std::tstring > strSrcFiles;
			str::cvt::MakeItemsAs( strSrcFiles, srcFiles );

			if ( HGLOBAL hDrop = BuildHDrop( strSrcFiles ) )		// minimum neccessary for drag&drop files
				rDataSource.CacheGlobalData( cfHDROP, hDrop );

			if ( HGLOBAL hFileGroupDescr = BuildFileGroupDescriptor( strSrcFiles ) )
				rDataSource.CacheGlobalData( cfFileGroupDescriptor, hFileGroupDescr );
		}
	}
}


#endif // ShellUtilities_h