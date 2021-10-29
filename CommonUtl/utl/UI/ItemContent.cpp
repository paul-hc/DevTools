
#include "stdafx.h"
#include "ItemContent.h"
#include "ContainerUtilities.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "ShellDialogs.h"
#include "StringUtilities.h"
#include "resource.h"

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
			if ( IsPathContent() )
				if ( !IsValidPathItem( item ) )
					return false;

		return true;
	}

	bool CItemContent::IsValidPathItem( const std::tstring& pathItem ) const
	{
		REQUIRE( IsPathContent() );

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
				utl::Uniquify< pred::TStringyCompareIntuitive >( rItems );
			else
				utl::Uniquify< pred::CompareNaturalPath >( rItems );
	}

	std::tstring CItemContent::EditItem( const TCHAR* pItem, CWnd* pParent, UINT cmdId ) const
	{
		ASSERT_PTR( pItem );

		fs::CPath newItem;

		if ( ui::String == m_type )
			return str::GetEmpty();

		newItem.Set( str::ExpandEnvironmentStrings( pItem ) );

		bool picked = false;

		switch ( m_type )
		{
			case ui::DirPath:
				picked = shell::PickFolder( newItem, pParent );
				break;
			case ui::FilePath:
				picked = shell::BrowseForFile( newItem, pParent, shell::FileOpen, m_pFileFilter );
				break;
			case ui::MixedPath:
				picked = BrowseMixedPath( newItem, pParent, cmdId );
				break;
			default:
				ASSERT( false );
		}
		if ( !picked )
			return str::GetEmpty();

		if ( IsPathContent() )
		{
			std::vector< std::tstring > variables;
			str::QueryEnvironmentVariables( variables, pItem );

			std::vector< std::tstring > values;
			str::ExpandEnvironmentVariables( values, variables );
			ENSURE( variables.size() == values.size() );

			for ( unsigned int i = 0; i != variables.size(); ++i )
				str::Replace( newItem.Ref(), values[ i ].c_str(), variables[ i ].c_str() );
		}

		if ( HasFlag( m_itemsFlags, Trim ) )
			str::Trim( newItem.Ref() );

		return newItem.Get();
	}

	bool CItemContent::BrowseMixedPath( fs::CPath& rNewItem, CWnd* pParent, UINT cmdId ) const
	{
		switch ( cmdId )
		{
			case ID_BROWSE_FILE:
				return shell::BrowseForFile( rNewItem, pParent, shell::FileBrowse, m_pFileFilter );
			case ID_BROWSE_FOLDER:
				return shell::PickFolder( rNewItem, pParent );
		}
		return shell::BrowseAutoPath( rNewItem, pParent, m_pFileFilter );		// choose the browse file/folder based on current path
	}

} //namespace ui
