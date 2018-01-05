#ifndef ItemContent_h
#define ItemContent_h
#pragma once


namespace ui
{
	enum ContentType { String, DirPath, FilePath };


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

		void SplitItems( std::vector< std::tstring >& rItems, const std::tstring& source, const TCHAR* pSep ) const;
		void FilterItems( std::vector< std::tstring >& rItems ) const;
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
