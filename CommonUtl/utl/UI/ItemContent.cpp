
#include "stdafx.h"
#include "ItemContent.h"
#include "ShellDialogs.h"
#include "StringUtilities.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Algorithms.hxx"


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

		fs::CPath path = str::ExpandEnvironmentStrings( pathItem.c_str() );
		fs::CPath actualPath = path::StripWildcards( path );

		switch ( m_type )
		{
			case ui::FilePath:
				return fs::IsValidFile( path.GetPtr() );
			case ui::DirPath:
				return fs::IsValidDirectory( actualPath.GetPtr() );
			case ui::MixedPath:
				return fs::FileExist( actualPath.GetPtr() );
			default:
				ASSERT( false );
				return false;
		}
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
				utl::Uniquify<pred::TLess_StringyIntuitive>( rItems );
			else
				utl::Uniquify<pred::TLess_NaturalPath>( rItems );
	}

	std::tstring CItemContent::EditItem( const TCHAR* pItem, CWnd* pParent, UINT cmdId /*= 0*/ ) const
	{
		ASSERT_PTR( pItem );

		if ( ui::String == m_type )
			return str::GetEmpty();

		fs::CPath newItem( str::ExpandEnvironmentStrings( pItem ) );
		bool picked = false;

		switch ( m_type )
		{
			case ui::FilePath:
				picked = shell::BrowseForFile( newItem, pParent, shell::FileOpen, m_pFileFilter );
				break;
			case ui::DirPath:
				picked = shell::PickFolder( newItem, pParent );
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
