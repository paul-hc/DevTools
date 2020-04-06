
#include "stdafx.h"
#include "ItemContent.h"
#include "ContainerUtilities.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "ShellDialogs.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	const CEnumTags& GetTags_ContentType( void )
	{
		static const CEnumTags s_tags( _T("Text|Folder|File|Folder, File or Wildcard") );
		return s_tags;
	}


	// CItemContent implementation

	void CItemContent::SplitItems( std::vector< std::tstring >& rItems, const std::tstring& source, const TCHAR sep[] ) const
	{
		str::Split( rItems, source.c_str(), sep );
		FilterItems( rItems );
	}

	bool CItemContent::IsValidItem( const std::tstring& item ) const
	{
		if ( HasFlag( m_itemsFlags, RemoveEmpty ) )
			if ( item.empty() )
				return false;

		if ( HasFlag( m_itemsFlags, EnsurePathExist ) )
			switch ( m_type )
			{
				case ui::DirPath:
				case ui::FilePath:
				case ui::MixedPath:
					if ( !IsValidPathItem( item ) )
						return false;
					break;
			}

		return true;
	}

	void CItemContent::FilterItems( std::vector< std::tstring >& rItems ) const
	{
		if ( HasFlag( m_itemsFlags, Trim ) )
			str::TrimItems( rItems );

		for ( std::vector< std::tstring >::const_iterator itItem = rItems.begin(); itItem != rItems.end(); )
			if ( IsValidItem( *itItem ) )
				++itItem;
			else
				itItem = rItems.erase( itItem );

		if ( HasFlag( m_itemsFlags, EnsureUnique ) )
			if ( ui::String == m_type )
				utl::RemoveDuplicates< pred::EqualString< std::tstring > >( rItems );
			else
				utl::RemoveDuplicates< pred::IsEquivalentPathString >( rItems );
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
			case ui::MixedPath:
				newItem = str::ExpandEnvironmentStrings( pItem );
				if ( !AutoBrowsePath( newItem, pParent ) )
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
			case ui::MixedPath:
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

	bool CItemContent::IsValidPathItem( const std::tstring& pathItem ) const
	{
		ASSERT( ui::DirPath == m_type || ui::FilePath == m_type || ui::MixedPath == m_type );

		std::tstring path = str::ExpandEnvironmentStrings( pathItem.c_str() );

		switch ( m_type )
		{
			case ui::DirPath:
				if ( fs::IsValidDirectory( path.c_str() ) )
					return true;
				break;
			case ui::FilePath:
				if ( fs::IsValidFile( path.c_str() ) )
					return true;
				break;
			case ui::MixedPath:
				if ( fs::FileExist( path.c_str() ) )
					return true;
				break;
		}

		return path::ContainsWildcards( path.c_str() );
	}

	bool CItemContent::AutoBrowsePath( std::tstring& rNewItem, CWnd* pParent ) const
	{
		if ( ui::FilePath == m_type || fs::IsValidFile( rNewItem.c_str() ) || path::ContainsWildcards( rNewItem.c_str() ) )
			return shell::BrowseForFile( rNewItem, pParent, MixedPath == m_type ? shell::FileBrowse : shell::FileOpen, m_pFileFilter );

		return shell::PickFolder( rNewItem, pParent );
	}

} //namespace ui
