#ifndef ItemContent_h
#define ItemContent_h
#pragma once

#include "utl/Subject.h"
#include "utl/Path.h"


class CEnumTags;


namespace ui
{
	enum ContentType { String, DirPath, FilePath };

	const CEnumTags& GetTags_ContentType( void );


	struct CItemContent
	{
		enum ItemsFlags
		{
			Trim			= BIT_FLAG( 0 ),
			RemoveEmpty		= BIT_FLAG( 1 ),
			EnsureUnique	= BIT_FLAG( 2 ),
			EnsurePathExist	= BIT_FLAG( 3 ),
				All = Trim | RemoveEmpty | EnsureUnique
		};

		CItemContent( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL, int itemsFlags = All )
			: m_type( type ), m_pFileFilter( pFileFilter ), m_itemsFlags( itemsFlags ) {}

		std::tstring EditItem( const TCHAR* pItem, CWnd* pParent ) const;

		void SplitItems( std::vector< std::tstring >& rItems, const std::tstring& source, const TCHAR sep[] ) const;
		void FilterItems( std::vector< std::tstring >& rItems ) const;

		bool AutoBrowsePath( std::tstring& rNewItem, CWnd* pParent ) const;
	public:
		ui::ContentType m_type;
		const TCHAR* m_pFileFilter;
		int m_itemsFlags;
	};


	interface IContentValidator
	{
		virtual void ValidateContent( void ) = 0;
	};


	class CPathItem : public CSubject
	{
	public:
		CPathItem( const fs::CPath& filePath ) : m_filePath( filePath ) {}

		const fs::CPath& GetPath( void ) const { return m_filePath; }

		// utl::ISubject interface
		virtual const std::tstring& GetCode( void ) const;

		template< typename PathContainerT >
		static void MakePathItems( std::vector< CPathItem* >& rPathItems, const PathContainerT& srcPaths )
		{
			utl::ClearOwningContainer( rPathItems );
			rPathItems.reserve( srcPaths.size() );

			for ( typename PathContainerT::const_iterator itSrcPath = srcPaths.begin(); itSrcPath != srcPaths.end(); ++itSrcPath )
				rPathItems.push_back( new CPathItem( *itSrcPath ) );
		}

		template< typename PathContainerT >
		static void QueryItemsPaths( PathContainerT& rPaths, const std::vector< CPathItem* >& srcPathItems )
		{
			rPaths.clear();
			rPaths.reserve( srcPathItems.size() );

			for ( std::vector< CPathItem* >::const_iterator itSrcItem = srcPathItems.begin(); itSrcItem != srcPathItems.end(); ++itSrcItem )
				rPaths.push_back( ( *itSrcItem )->GetPath() );
		}
	private:
		fs::CPath m_filePath;
	};
}


#endif // ItemContent_h
