
#include "stdafx.h"
#include "ShellUtilities.h"
#include "ShellTypes.h"
#include "ContainerUtilities.h"
#include "ImagingWic.h"
#include "Registry.h"
#include "StringUtilities.h"
#include "TreeControl.h"
#include <afxole.h>				// COleDataSource, COleDataObject
#include <hash_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	namespace impl
	{
		// Allows creation and modal execution in 2 separate steps, with eventual customization in between.
		// Persists last selected filter index per each used array of filters.
		//
		struct CScopedFileDialog
		{
			CScopedFileDialog( const TCHAR* pFileFilter );
			~CScopedFileDialog();

			void StoreDialog( CFileDialog* pFileDlg );
		public:
			std::tstring m_fileFilter;
			std::auto_ptr< CFileDialog > m_pFileDlg;							// Vista style dialog requires COM initialization (done in CFileDialog)

			static const TCHAR s_allFilesFilter[];
			static stdext::hash_map< std::tstring, int > s_selFilterMap;		// filter text to selected filter index
		};


		// BrowseForFolder callback

		static TCHAR s_initFolderPath[ MAX_PATH ];
		static enum BrowseFolderStatus { Done, Initialized } s_browseFolderStatus = Done;

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data );
	}


	bool BrowseForFolder( std::tstring& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName /*= NULL*/,
						  BrowseFlags flags /*= BF_FileSystem*/, const TCHAR* pTitle /*= NULL*/, bool useNetwork /*= false*/ )
	{
		bool isOk = false;

		str::Copy( impl::s_initFolderPath, rFolderPath );

		TCHAR displayName[ MAX_PATH ] = _T("");
		BROWSEINFO bi;
		utl::ZeroStruct( &bi );

		bi.hwndOwner = pParentWnd->GetSafeHwnd();
		bi.pszDisplayName = displayName;
		bi.lpszTitle = pTitle;
		bi.ulFlags = BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;

		switch( flags )
		{
			case BF_Computers:				bi.ulFlags |= BIF_BROWSEFORCOMPUTER; break;
			case BF_Printers:				bi.ulFlags |= BIF_BROWSEFORPRINTER; break;
			case BF_FileSystem:				bi.ulFlags |= BIF_RETURNONLYFSDIRS; break;
			case BF_FileSystemIncludeFiles:	bi.ulFlags |= ( BIF_RETURNONLYFSDIRS | BIF_BROWSEINCLUDEFILES ); break;
			case BF_AllIncludeFiles:		bi.ulFlags |= BIF_BROWSEINCLUDEFILES; break;
			default: ASSERT( BF_All == flags );
		}

		if ( useNetwork )
			bi.ulFlags |= BIF_DONTGOBELOWDOMAIN;		// could be slow network binding
		bi.lpfn = impl::BrowseFolderCallback;

		CPidl pidlFolder( ::SHBrowseForFolder( &bi ) );

		if ( pDisplayedName != NULL )
			*pDisplayedName = displayName;

		if ( !pidlFolder.IsEmpty() )
		{
			if ( ::SHGetPathFromIDList( pidlFolder.Get(), impl::s_initFolderPath ) )
			{
				rFolderPath = impl::s_initFolderPath;
				isOk = true;
			}
			else if ( flags == BF_Computers || flags == BF_All || flags == BF_AllIncludeFiles )
			{
				rFolderPath = displayName;
				isOk = true;
			}
		}

		return isOk;
	}


	bool BrowseForFile( std::tstring& rFilePath, CWnd* pParentWnd, BrowseMode browseMode /*= FileOpen*/,
						const TCHAR* pFileFilter /*= NULL*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
	{
		impl::CScopedFileDialog scopedDlg( pFileFilter );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, browseMode, scopedDlg.m_fileFilter, flags, pTitle ) );
		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}

	bool PickFolder( std::tstring& rFilePath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
	{
		impl::CScopedFileDialog scopedDlg( NULL );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, FileOpen, std::tstring(), 0, pTitle ) );		// no filter for picking folders

		if ( IFileOpenDialog* pFileOpenDialog = scopedDlg.m_pFileDlg->GetIFileOpenDialog() )
		{
			pFileOpenDialog->SetOptions( FOS_PICKFOLDERS | options );
			pFileOpenDialog->Release();			// ** cannot use CComPtr< IFileOpenDialog > here since CFileDialog::~CFileDialog() fires an assertion anyway, shape and form
		}

		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}


	namespace impl
	{
		// File Dialog

		CFileDialog* MakeFileDialog( const std::tstring& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
		{
			SetFlag( flags, OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );

			if ( FileOpen == browseMode )
				SetFlag( flags, OFN_FILEMUSTEXIST );
			else
				SetFlag( flags, OFN_OVERWRITEPROMPT );

			CFileDialog* pDlg = new CFileDialog( browseMode, NULL, filePath.c_str(), flags, fileFilter.c_str(), pParentWnd );

			if ( pTitle != NULL )
				pDlg->m_ofn.lpstrTitle = pTitle;

			return pDlg;
		}

		bool RunFileDialog( std::tstring& rFilePath, CFileDialog* pFileDialog )
		{
			ASSERT_PTR( pFileDialog );
			if ( !pFileDialog->DoModal() )
				return false;

			rFilePath = pFileDialog->GetPathName().GetString();
			return true;
		}


		// CScopedFileDialog implementation

		const TCHAR CScopedFileDialog::s_allFilesFilter[] = _T("All Files (*.*)|*.*||");
		stdext::hash_map< std::tstring, int > CScopedFileDialog::s_selFilterMap;

		CScopedFileDialog::CScopedFileDialog( const TCHAR* pFileFilter )
			: m_fileFilter( pFileFilter != NULL ? pFileFilter : s_allFilesFilter )
		{
		}

		void CScopedFileDialog::StoreDialog( CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );
			m_pFileDlg.reset( pFileDlg );

			stdext::hash_map< std::tstring, int >::const_iterator itFilterIndex = s_selFilterMap.find( m_fileFilter );
			if ( itFilterIndex != s_selFilterMap.end() )
				m_pFileDlg->m_pOFN->nFilterIndex = itFilterIndex->second;				// use last selected filter
		}

		CScopedFileDialog::~CScopedFileDialog()
		{
			if ( m_pFileDlg.get() != NULL )
				s_selFilterMap[ m_fileFilter ] = m_pFileDlg->m_pOFN->nFilterIndex;		// save selected file filter
		}


		// BrowseForFolder callback

		bool IsTreeCtrl( HWND hCtrl )
		{
			TCHAR className[ 256 ];
			::GetClassName( hCtrl, className, COUNT_OF( className ) );
			return str::Find< str::IgnoreCase >( className, _T("SysTreeView") ) != std::tstring::npos;
		}

		static BOOL CALLBACK FindChildTreeCtrlProc( HWND hWnd, HWND* phCtrl )
		{
			ASSERT_PTR( phCtrl );
			if ( !IsTreeCtrl( hWnd ) )
				return TRUE;			// continue enumeration
			*phCtrl = hWnd;
			return FALSE;				// found, stop enumeration
		}

		CTreeControl* FindBrowseFolderTree( HWND hDlg )
		{
			HWND hCtrl = NULL;
			::EnumChildWindows( hDlg, (WNDENUMPROC)&FindChildTreeCtrlProc, reinterpret_cast< LPARAM >( &hCtrl ) );
			return hCtrl != NULL ? (CTreeControl*)CWnd::FromHandle( hCtrl ) : NULL;
		}

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data )
		{
			lParam, data;
			switch( msg )
			{
				case BFFM_INITIALIZED:
					if ( !str::IsEmpty( s_initFolderPath ) )
						SendMessage( hDlg, BFFM_SETSELECTION, TRUE, (LPARAM)s_initFolderPath );
					s_browseFolderStatus = Initialized;
					break;
				case BFFM_SELCHANGED:
				{
					// it seems no longer necessary
					// set the status window to the currently selected path
					TCHAR dirPath[ MAX_PATH ];
					if ( ::SHGetPathFromIDList( (LPITEMIDLIST)lParam, dirPath ) )
						SendMessage( hDlg, BFFM_SETSTATUSTEXT, 0, (LPARAM)dirPath );

					if ( Initialized == s_browseFolderStatus )
					{
						s_browseFolderStatus = Done;

						// the init cycle is: BFFM_SELCHANGED -> BFFM_SELCHANGED -> BFFM_INITIALIZED -> BFFM_IUNKNOWN -> BFFM_SELCHANGED;
						// only as late as now is possible to ensure the tree item is visible (earlier on BFFM_INITIALIZED it doesn't always work)
						if ( CTreeControl* pTreeCtrl = FindBrowseFolderTree( hDlg ) )
							if ( HTREEITEM hSelItem = pTreeCtrl->GetSelectedItem() )
								pTreeCtrl->EnsureVisible( hSelItem );
					}
					break;
				}
				default:
					break;
			}
			return 0;
		}
	}
}


namespace shell
{
	bool DoFileOperation( UINT shellOp, const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >* pDestPaths,
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
				SetFlag( fileOp.fFlags, FOF_MULTIDESTFILES );								// each SRC file has a DEST file

		return 0 == ::SHFileOperation( &fileOp ) && !fileOp.fAnyOperationsAborted;		// true for success AND not canceled by user
	}

	bool MoveFiles( const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >& destPaths,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		return DoFileOperation( FO_MOVE, srcPaths, &destPaths, pWnd, flags );
	}

	bool MoveFiles( const std::vector< std::tstring >& srcPaths, const std::tstring& destFolderPath,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		std::vector< std::tstring > destPaths( 1, destFolderPath );
		return DoFileOperation( FO_MOVE, srcPaths, &destPaths, pWnd, flags );
	}

	bool CopyFiles( const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >& destPaths,
					CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		return DoFileOperation( FO_COPY, srcPaths, &destPaths, pWnd, flags );
	}

	bool CopyFiles( const std::vector< std::tstring >& srcPaths, const std::tstring& destFolderPath, CWnd* pWnd /*= AfxGetMainWnd()*/,
					FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
	{
		std::vector< std::tstring > destPaths( 1, destFolderPath );
		return DoFileOperation( FO_COPY, srcPaths, &destPaths, pWnd, flags );
	}

	bool DeleteFiles( const std::vector< std::tstring >& srcPaths, CWnd* pWnd /*= AfxGetMainWnd()*/, FILEOP_FLAGS flags /*= FOF_ALLOWUNDO*/ )
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

		std::tstring extKeyName = str::Format( _T("HKEY_CLASSES_ROOT\\%s"), pExt );
		reg::CKey textClassName( extKeyName.c_str(), false );
		if ( !textClassName.IsValid() )
			return std::tstring();

		return textClassName.ReadString( NULL );
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

		HGLOBAL BuildHDrop( const std::vector< std::tstring >& srcFiles )
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

		HGLOBAL BuildFileGroupDescriptor( const std::vector< std::tstring >& srcFiles )
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
							_tcscpy( pFileGroupDescr->fgd[ i ].cFileName, srcFiles[ i ].c_str() );
						}
						::GlobalUnlock( hGlobal );
					}
				}
			}
			return hGlobal;
		}

	} //namespace xfer

} //namespace shell
