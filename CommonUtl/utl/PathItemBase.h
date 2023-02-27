#ifndef PathItemBase_h
#define PathItemBase_h
#pragma once

#include "Subject.h"
#include "Path.h"


abstract class CPathItemBase : public TSubject
{
protected:
	CPathItemBase( const fs::CPath& filePath ) { ResetFilePath( filePath ); }
public:
	virtual ~CPathItemBase();

	const fs::CPath& GetFilePath( void ) const { return m_filePath; }
	virtual void SetFilePath( const fs::CPath& filePath );

	void SetDisplayCode( const std::tstring& displayPath ) { m_displayPath = displayPath; }
	void StripDisplayCode( const fs::CPath& commonParentPath );

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	struct ToFilePath
	{
		typedef const fs::CPath& TReturn;

		const fs::CPath& operator()( const fs::CPath& path ) const { return path; }
		const fs::CPath& operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath(); }
	};

	struct ToParentFolderPath
	{
		fs::CPath operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath().GetParentPath(); }
	};

	struct ToNameExt
	{
		const TCHAR* operator()( const CPathItemBase* pItem ) const { return pItem->GetFilePath().GetFilenamePtr(); }
	};
protected:
	void ResetFilePath( const fs::CPath& filePath );
	void Stream( CArchive& archive );
private:
	persist fs::CPath m_filePath;
	std::tstring m_displayPath;
};


namespace utl
{
	// forward declarations - required for C++ 14+ compilation
	template< typename PtrContainerT >
	void ClearOwningContainer( PtrContainerT& rContainer );
}


class CPathItem : public CPathItemBase
{
public:
	CPathItem( const fs::CPath& filePath ) : CPathItemBase( filePath ) {}

	template< typename PathContainerT >
	static void MakePathItems( std::vector<CPathItem*>& rPathItems, const PathContainerT& filePaths )
	{
		utl::ClearOwningContainer( rPathItems );
		rPathItems.reserve( filePaths.size() );

		for ( typename PathContainerT::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
			rPathItems.push_back( new CPathItem( *itFilePath ) );
	}
};


namespace pred
{
	typedef pred::CompareAdapter<pred::CompareNaturalPath, CPathItemBase::ToFilePath> TComparePathItem;
	typedef pred::LessValue<TComparePathItem> TLess_PathItem;
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
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( ( *itItem )->GetFilePath() == filePath )
				return *itItem;

		return ContainerT::value_type( nullptr );
	}


	template< typename CompareItemT, typename ContainerT >
	inline void SortPathItems( ContainerT& rPathItems, bool ascending = true )
	{
		std::sort( rPathItems.begin(), rPathItems.end(), pred::OrderByValue<CompareItemT>( ascending ) );
	}

	template< typename ContainerT >
	inline void SortPathItems( ContainerT& rPathItems, bool ascending = true )
	{
		SortPathItems<pred::TComparePathItem>( rPathItems, ascending );
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


namespace cvt
{
	struct CQueryPaths
	{
		template< typename ItemContainerT >
		CQueryPaths( const ItemContainerT& srcPathItems )
		{
			func::QueryItemsPaths( m_paths, srcPathItems );
		}
	public:
		std::vector<fs::CPath> m_paths;
	};
}


#endif // PathItemBase_h
