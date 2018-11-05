#ifndef ShellUtilities_h
#define ShellUtilities_h
#pragma once

#include "Shell_fwd.h"


namespace shell
{
	bool BrowseForFolder( std::tstring& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = NULL,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = NULL, bool useNetwork = false );

	bool BrowseForFile( std::tstring& rFilePath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = NULL, DWORD flags = 0, const TCHAR* pTitle = NULL );

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
				_tcscpy( &rFileListBuffer[ pos ], str::traits::GetStr( *itPath ) );
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
