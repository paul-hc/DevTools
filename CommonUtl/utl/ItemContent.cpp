
#include "stdafx.h"
#include "ItemContent.h"
#include "ContainerUtilities.h"
#include "FileSystem.h"
#include "ShellUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CItemContent implementation

	void CItemContent::SplitItems( std::vector< std::tstring >& rItems, const std::tstring& source, const TCHAR* pSep ) const
	{
		str::Split( rItems, source.c_str(), pSep );
		FilterItems( rItems );
	}

	void CItemContent::FilterItems( std::vector< std::tstring >& rItems ) const
	{
		if ( HasFlag( m_itemsFlags, Trim ) )
			str::TrimItems( rItems );

		if ( HasFlag( m_itemsFlags, RemoveEmpty ) )
			str::RemoveEmptyItems( rItems );

		if ( HasFlag( m_itemsFlags, EnsureUnique ) )
			if ( ui::String == m_type )
				utl::RemoveDuplicates< pred::EqualString< std::tstring > >( rItems );
			else
				utl::RemoveDuplicates< pred::EquivalentPathString >( rItems );

		if ( HasFlag( m_itemsFlags, EnsurePathExist ) )
			if ( ui::DirPath == m_type || ui::FilePath == m_type )
				for ( std::vector< std::tstring >::const_iterator itItem = rItems.begin(); itItem != rItems.end(); )
				{
					std::tstring path = str::ExpandEnvironmentStrings( itItem->c_str() );

					if ( ( ui::DirPath == m_type ? fs::IsValidDirectory( path.c_str() ) : fs::IsValidFile( path.c_str() ) ) ||
						 path::ContainsWildcards( path.c_str() ) )
						++itItem;
					else
						itItem = rItems.erase( itItem );
				}
	}

	std::tstring CItemContent::EditItem( const TCHAR* pItem, CWnd* pParent ) const
	{
		ASSERT_PTR( pItem );
		static const std::tstring emptyText;

		std::tstring newItem;

		switch ( m_type )
		{
			default: ASSERT( false );
			case ui::String:
				return emptyText;
			case ui::DirPath:
				newItem = str::ExpandEnvironmentStrings( pItem );
				if ( !shell::BrowseForFolder( newItem, pParent ) )
					return emptyText;
				break;
			case ui::FilePath:
				newItem = str::ExpandEnvironmentStrings( pItem );
				if ( !shell::BrowseForFile( newItem, pParent, shell::FileOpen, m_pFileFilter ) )
					return emptyText;
				break;
		}

		switch ( m_type )
		{
			case ui::DirPath:
			case ui::FilePath:
			{
				std::vector< std::tstring > variables;
				str::QueryEnvironmentVariables( variables, pItem );

				std::vector< std::tstring > values;
				str::ExpandEnvironmentVariables( values, variables );
				ENSURE( variables.size() == values.size() );

				for ( unsigned int i = 0; i != variables.size(); ++i )
					str::Replace( newItem, values[ i ].c_str(), variables[ i ].c_str() );
				break;
			}
		}
		if ( HasFlag( m_itemsFlags, Trim ) )
			str::Trim( newItem );
		return newItem;
	}

} //namespace ui