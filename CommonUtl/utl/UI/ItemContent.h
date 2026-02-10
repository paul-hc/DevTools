#ifndef ItemContent_h
#define ItemContent_h
#pragma once

#include "utl/Path_fwd.h"


class CEnumTags;


namespace ui
{
	enum ContentType
	{
		String,
		FilePath,
		DirPath,
		MixedPath		// a mix of directory/file/wildcard-spec
	};

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

		CItemContent( ui::ContentType type = ui::String, const TCHAR* pFileFilter = nullptr, int itemsFlags = All )
			: m_type( type ), m_pFileFilter( pFileFilter ), m_itemsFlags( itemsFlags ) {}

		bool IsValidItem( const std::tstring& item ) const;
		std::tstring EditItem( const TCHAR* pItem, CWnd* pParent, UINT cmdId = 0 ) const;

		void SplitItems( std::vector<std::tstring>& rItems, const std::tstring& source, const TCHAR sep[] ) const;
		void FilterItems( std::vector<std::tstring>& rItems ) const;

		bool IsPathContent( void ) const { return ui::FilePath == m_type || ui::DirPath == m_type || ui::MixedPath == m_type; }
		bool IsValidPathItem( const std::tstring& pathItem ) const;
	private:
		bool BrowseMixedPath( shell::TPath& rNewItem, CWnd* pParent, UINT cmdId ) const;
	public:
		ui::ContentType m_type;
		const TCHAR* m_pFileFilter;
		int m_itemsFlags;
	};


	interface IContentValidator
	{
		virtual void ValidateContent( void ) = 0;
	};
}


#endif // ItemContent_h
