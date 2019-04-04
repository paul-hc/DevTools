#ifndef PathItemBase_h
#define PathItemBase_h
#pragma once

#include "Subject.h"
#include "Path.h"


abstract class CPathItemBase : public CSubject
{
protected:
	CPathItemBase( const fs::CPath& filePath );
public:
	virtual ~CPathItemBase();

	const fs::CPath& GetFilePath( void ) const { return m_filePath; }

	void SetDisplayCode( const std::tstring& displayPath ) { m_displayPath = displayPath; }
	void StripDisplayCode( const fs::CPath& commonParentPath );

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	struct ToFilePath
	{
		const fs::CPath& operator()( const fs::CPath& path ) const { return path; }
		const fs::CPath& operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath(); }
	};

	struct ToParentFolderPath
	{
		fs::CPath operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath().GetParentPath(); }
	};

	struct ToNameExt
	{
		const TCHAR* operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath().GetNameExt(); }
	};
private:
	fs::CPath m_filePath;
	std::tstring m_displayPath;
};


class CPathItem : public CPathItemBase
{
public:
	CPathItem( const fs::CPath& filePath ) : CPathItemBase( filePath ) {}

	template< typename PathContainerT >
	static void MakePathItems( std::vector< CPathItem* >& rPathItems, const PathContainerT& filePaths )
	{
		utl::ClearOwningContainer( rPathItems );
		rPathItems.reserve( filePaths.size() );

		for ( typename PathContainerT::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
			rPathItems.push_back( new CPathItem( *itFilePath ) );
	}
};


namespace pred
{
	struct CompareItemPath
	{
		CompareResult operator()( const CPathItemBase* pLeftItem, const CPathItemBase* pRightItem ) const
		{
			return CompareNaturalPath()( pLeftItem->GetFilePath(), pRightItem->GetFilePath() );
		}
	};
}


namespace func
{
	template< typename PathContainerT, typename ItemContainerT >
	void QueryItemsPaths( PathContainerT& rFilePaths, const ItemContainerT& srcPathItems )
	{
		rFilePaths.clear();
		rFilePaths.reserve( srcPathItems.size() );

		for ( typename ItemContainerT::const_iterator itSrcItem = srcPathItems.begin(); itSrcItem != srcPathItems.end(); ++itSrcItem )
			rFilePaths.push_back( ( *itSrcItem )->GetFilePath().Get() );
	}

	template< typename ContainerT >
	typename ContainerT::value_type FindItemWithPath( const ContainerT& items, const fs::CPath& filePath )
	{
		for ( ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( ( *itItem )->GetFilePath() == filePath )
				return *itItem;

		return ContainerT::value_type( NULL );
	}


	struct StripDisplayCode
	{
		StripDisplayCode( const fs::CPath& commonParentPath ) : m_commonParentPath( commonParentPath ) {}

		void operator()( CPathItemBase* pItem ) const
		{
			pItem->StripDisplayCode( m_commonParentPath );
		}
	private:
		const fs::CPath& m_commonParentPath;			// to be stripped
	};
}


#endif // PathItemBase_h
