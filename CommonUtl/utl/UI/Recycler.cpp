
#include "stdafx.h"
#include "Recycler.h"
#include "ShellContextMenuHost.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include <ntquery.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	struct ReleaseComFirst
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			rPair.first->Release();
		}
	};
}


namespace shell
{
	// CRecycler implementation

	const PROPERTYKEY CRecycler::PK_Name = { PSGUID_STORAGE, PID_STG_NAME };
	const PROPERTYKEY CRecycler::PK_Type = { PSGUID_STORAGE, PID_STG_STORAGETYPE };
	const PROPERTYKEY CRecycler::PK_Size = { PSGUID_STORAGE, PID_STG_SIZE };
	const PROPERTYKEY CRecycler::PK_WriteTime = { PSGUID_STORAGE, PID_STG_WRITETIME };
	const PROPERTYKEY CRecycler::PK_CreateTime = { PSGUID_STORAGE, PID_STG_CREATETIME };
	const PROPERTYKEY CRecycler::PK_AccessTime = { PSGUID_STORAGE, PID_STG_ACCESSTIME };
	const PROPERTYKEY CRecycler::PK_Attributes = { PSGUID_STORAGE, PID_STG_ATTRIBUTES };

	const PROPERTYKEY CRecycler::PK_OriginalLocation = { PSGUID_DISPLACED, PID_DISPLACED_FROM };
	const PROPERTYKEY CRecycler::PK_DateDeleted = { PSGUID_DISPLACED, PID_DISPLACED_DATE };

	CComPtr<IShellItem2> CRecycler::GetRecycleBinShellItem( void )
	{
		CComPtr<IShellItem2> pRecycleBinItem;
		if ( HR_OK( ::SHGetKnownFolderItem( FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, nullptr, IID_PPV_ARGS( &pRecycleBinItem ) ) ) )
			return pRecycleBinItem;

		return nullptr;
	}

	CComPtr<IShellFolder2> CRecycler::GetRecycleBinFolder( void )
	{
		CComPtr<IShellFolder2> pRecycleBinFolder;
		if ( CComPtr<IShellFolder> pDesktop = GetDesktopFolder() )
		{
			CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlRecycleBin;
			if ( HR_OK( ::SHGetSpecialFolderLocation( nullptr /*m_hWnd*/, CSIDL_BITBUCKET, &pidlRecycleBin ) ) )
				if ( HR_OK( pDesktop->BindToObject( pidlRecycleBin, nullptr, IID_PPV_ARGS( &pRecycleBinFolder ) ) ) )
					return pRecycleBinFolder;
		}

		return nullptr;
	}

	CComPtr<IEnumShellItems> CRecycler::GetEnumItems( void ) const
	{
		CComPtr<IEnumShellItems> pEnumItems;
		if ( HR_OK( m_pRecyclerItem->BindToHandler( nullptr, BHID_EnumItems, IID_PPV_ARGS( &pEnumItems ) ) ) )
			return pEnumItems;

		return nullptr;
	}

	void CRecycler::QueryRecycledItems( std::vector< IShellItem2* >& rRecycledItems, const TCHAR* pOrigPrefixOrSpec, path::SpecMatch minMatch /*= path::Match_Any*/ ) const
	{
		// pOrigPrefixOrSpec could be:
		//	"" or NULL - no filter, return all recycled items
		//	"C:\dev\Samples\xRecycled\RaymondChen.pch" - original file path
		//	"C:\dev\Samples/xrecycled" - ancestor the original directory path
		//	"C:\dev\Samples\xrecycled\*.txt" - original directory path with wildcard spec
		//
		std::vector< std::pair<IShellItem2*, CTime> > items;

		if ( CComPtr<IEnumShellItems> pEnumItems = GetEnumItems() )
			for ( CComPtr<IShellItem> pItem; S_OK == pEnumItems->Next( 1, &pItem, nullptr ); pItem = nullptr )
				if ( CComQIPtr<IShellItem2> pItem2 = pItem.p )
				{
					path::SpecMatch foundMatch = OriginalPathMatchesPrefix( pItem2, pOrigPrefixOrSpec );
					if ( foundMatch >= minMatch )
					{
						pItem2.p->AddRef();		// will be released by the caller
						items.push_back( std::make_pair( pItem2.p, GetDateDeleted( pItem2 ) ) );
					}
				}

		std::sort( items.begin(), items.end(), pred::OrderByValue< pred::CompareSecond< pred::CompareValue > >( false ) );		// sort by Deleted Time descending: most recently deleted first

		for ( std::vector< std::pair<IShellItem2*, CTime> >::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			rRecycledItems.push_back( itItem->first );
	}

	void CRecycler::QueryMultiRecycledItems( std::vector< IShellItem2* >& rRecycledItems, const std::vector< fs::CPath >& delFilePaths ) const
	{
		REQUIRE( rRecycledItems.empty() );						// any previous items must have been released by caller
		rRecycledItems.resize( delFilePaths.size() );			// reset to NULL all recycled items (corresponding to each file in delFilePaths)

		typedef std::vector< std::pair<IShellItem2*, CTime> > TFileRecycledItems;
		std::vector< TFileRecycledItems > fileRecycledItems;		// indexed in sync with delFilePaths

		fileRecycledItems.resize( delFilePaths.size() );

		if ( CComPtr<IEnumShellItems> pEnumItems = GetEnumItems() )
			for ( CComPtr<IShellItem> pItem; S_OK == pEnumItems->Next( 1, &pItem, nullptr ); pItem = nullptr )
				if ( CComQIPtr<IShellItem2> pRecycledItem = pItem.p )
				{
					fs::CPath origFilePath = GetOriginalFilePath( pRecycledItem );
					size_t fileEntryPos = utl::FindPos( delFilePaths, origFilePath );

					if ( fileEntryPos != utl::npos )
					{
						pRecycledItem.p->AddRef();		// will be released by the caller

						TFileRecycledItems& rMultiRecycledItems = fileRecycledItems[ fileEntryPos ];
						rMultiRecycledItems.push_back( std::make_pair( pRecycledItem.p, GetDateDeleted( pRecycledItem ) ) );
					}
				}

		// sort each sequence by Deleted Time descending: most recently deleted comes first
		for ( size_t entryPos = 0; entryPos != fileRecycledItems.size(); ++entryPos )
		{
			TFileRecycledItems& rMultiRecycledItems = fileRecycledItems[ entryPos ];

			if ( !rMultiRecycledItems.empty() )
			{
				// a file with same path can be recycled multiple times; sorting by Deleted Time descending puts first the most recently deleted (up for undeletion)
				std::sort( rMultiRecycledItems.begin(), rMultiRecycledItems.end(), pred::OrderByValue< pred::CompareSecond< pred::CompareValue > >( false ) );

				std::for_each( rMultiRecycledItems.begin() + 1, rMultiRecycledItems.end(), func::ReleaseComFirst() );		// release the unused, previously AddRef-ed interfaces
				rRecycledItems[ entryPos ] = rMultiRecycledItems.front().first;												// store returned recycled item interface (latest deleted)
			}
		}
	}

	IShellItem2* CRecycler::FindRecycledItem( const fs::CPath& delFilePath ) const
	{
		std::vector< IShellItem2* > recycledItems;
		QueryMultiRecycledItems( recycledItems, std::vector< fs::CPath >( 1, delFilePath ) );
		ASSERT( 1 == recycledItems.size() );

		return recycledItems.front();
	}

	bool CRecycler::UndeleteFile( const fs::CPath& delFilePath, CWnd* pWndOwner )
	{
		bool succeeded = false;
		if ( IShellItem2* pRecycledItem = FindRecycledItem( delFilePath ) )
		{
			succeeded = UndeleteItem( pRecycledItem, pWndOwner );
			pRecycledItem->Release();		// release previously AddRef-ed item interface
		}
		return succeeded;
	}

	size_t CRecycler::UndeleteMultiFiles( const std::vector< fs::CPath >& delFilePaths, CWnd* pWndOwner, std::vector< fs::CPath >* pErrorFilePaths /*= nullptr*/ )
	{
		std::vector< IShellItem2* > recycledItems;
		QueryMultiRecycledItems( recycledItems, delFilePaths );

		std::vector< fs::CPath > errorFilePaths;

		for ( size_t entryPos = 0; entryPos != recycledItems.size(); )
		{
			IShellItem2* pRecycledItem = recycledItems[ entryPos ];

			if ( nullptr == pRecycledItem )
			{
				errorFilePaths.push_back( delFilePaths[ entryPos ] );
				recycledItems.erase( recycledItems.begin() + entryPos );		// remove NULL items
			}
			else
				++entryPos;
		}

		size_t undeletedCount = 0;

		if ( !recycledItems.empty() )
			if ( CComPtr<IContextMenu> pContextMenu = shell::MakeItemsContextMenu( recycledItems, pWndOwner->GetSafeHwnd() ) )
				if ( Undelete( pContextMenu, pWndOwner ) )
					undeletedCount = recycledItems.size();

		std::for_each( recycledItems.begin() + 1, recycledItems.end(), func::ReleaseCom() );		// release the unused, previously AddRef-ed interfaces

		if ( pErrorFilePaths != nullptr )
			pErrorFilePaths->swap( errorFilePaths );
		return undeletedCount;
	}

	size_t CRecycler::UndeleteMultiFiles2( const std::vector< fs::CPath >& delFilePaths, CWnd* pWndOwner, std::vector< fs::CPath >* pErrorFilePaths /*= nullptr*/ )
	{
		std::vector< IShellItem2* > recycledItems;
		QueryMultiRecycledItems( recycledItems, delFilePaths );

		size_t undeletedCount = 0;
		std::vector< fs::CPath > errorFilePaths;

		for ( size_t entryPos = 0; entryPos != recycledItems.size(); ++entryPos )
		{
			IShellItem2* pRecycledItem = recycledItems[ entryPos ];
			const fs::CPath& delFilePath = delFilePaths[ entryPos ];

			if ( pRecycledItem != nullptr )
			{
				if ( UndeleteItem( pRecycledItem, pWndOwner ) )
					++undeletedCount;
				else
					errorFilePaths.push_back( delFilePath );

				pRecycledItem->Release();		// release previously AddRef-ed item interface
			}
			else
				errorFilePaths.push_back( delFilePath );
		}

		if ( pErrorFilePaths != nullptr )
			pErrorFilePaths->swap( errorFilePaths );
		return undeletedCount;
	}

	bool CRecycler::Undelete( IContextMenu* pContextMenu, CWnd* pWndOwner )
	{
		ASSERT_PTR( pContextMenu );
		CShellContextMenuHost shellMenuHost( pWndOwner, pContextMenu );
		return shellMenuHost.InvokeVerb( "undelete" );
	}

	bool CRecycler::UndeleteItem( IShellItem2* pRecycledItem, CWnd* pWndOwner )
	{
		ASSERT_PTR( pRecycledItem );
		if ( CComPtr<IContextMenu> pContextMenu = shell::MakeItemContextMenu( pRecycledItem, pWndOwner->GetSafeHwnd() ) )
			return Undelete( pContextMenu, pWndOwner );
		return false;
	}


	path::SpecMatch CRecycler::OriginalPathMatchesPrefix( IShellItem* pRecycledItem, const TCHAR* pOrigPrefixOrSpec )
	{
		ASSERT_PTR( pRecycledItem );

		if ( str::IsEmpty( pOrigPrefixOrSpec ) )
			return path::Match_Any;		// no common prefix/spec filter

		return path::MatchesPrefix( GetOriginalFilePath( pRecycledItem ).GetPtr(), pOrigPrefixOrSpec );
	}

	void CRecycler::QueryAvailableDrives( std::vector< std::tstring >& rDriveRootPaths )
	{
		std::vector< TCHAR > drivesBuffer( ::GetLogicalDriveStrings( 0, nullptr ) + 1 );
		if ( TCHAR* pDrivesList = &drivesBuffer.front() )
		{
			::GetLogicalDriveStrings( static_cast<DWORD>( drivesBuffer.size() ), pDrivesList );

			for ( const TCHAR* pDrive = pDrivesList; *pDrive != _T('\0'); pDrive += str::GetLength( pDrive ) + 1 )
				rDriveRootPaths.push_back( pDrive );
		}
	}

	void CRecycler::QueryDrivesWithRecycledItems( std::vector< std::tstring >& rDriveRootPaths )
	{
		QueryAvailableDrives( rDriveRootPaths );

		for ( std::vector< std::tstring >::iterator itRootPath = rDriveRootPaths.begin(); itRootPath != rDriveRootPaths.end(); )
			if ( FindRecycledItemCount( itRootPath->c_str() ) != 0 )		// has deleted files in Recycle Bin?
				++itRootPath;
			else
				itRootPath = rDriveRootPaths.erase( itRootPath );
	}

	bool CRecycler::QueryRecycleBin( const TCHAR* pRootPath, size_t& rItemCount, ULONGLONG* pTotalSize /*= nullptr*/ )
	{
		SHQUERYRBINFO rbInfo;
		::ZeroMemory( &rbInfo, sizeof( rbInfo ) );
		rbInfo.cbSize = sizeof( SHQUERYRBINFO );

		bool succeeded = HR_OK( ::SHQueryRecycleBin( pRootPath, &rbInfo ) );

		rItemCount = static_cast<size_t>( rbInfo.i64NumItems );
		if ( pTotalSize != nullptr )
			*pTotalSize = static_cast<ULONGLONG>( rbInfo.i64Size );
		return succeeded;
	}

	size_t CRecycler::FindRecycledItemCount( const TCHAR* pRootPath )
	{
		size_t itemCount;
		QueryRecycleBin( pRootPath, itemCount );
		return itemCount;		// non-zero if it has deleted files in Recycle Bin?
	}
}
