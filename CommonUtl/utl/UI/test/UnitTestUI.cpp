
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/UnitTestUI.h"
#include "utl/StringUtilities.h"
#include "utl/FlagTags.h"
#include "utl/TextClipboard.h"
#include "ShellPidl.h"
#include "ShellContextMenuHost.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	const CValueTags& GetTags_SIGDN( void )
	{
		static const CValueTags::ValueDef s_valueDefs[] =
		{
			{ VALUE_TAG( SIGDN_NORMALDISPLAY ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEPARSING ) },
			{ VALUE_TAG( SIGDN_DESKTOPABSOLUTEPARSING ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEEDITING ) },
			{ VALUE_TAG( SIGDN_DESKTOPABSOLUTEEDITING ) },
			{ VALUE_TAG( SIGDN_FILESYSPATH ) },
			{ VALUE_TAG( SIGDN_URL ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEFORADDRESSBAR ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVE ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEFORUI ) }
		};

		static const CValueTags s_tags( ARRAY_SPAN( s_valueDefs ) );
		return s_tags;
	}

	void TracePidlNames( PCIDLIST_ABSOLUTE pidl )
	{
		static const SIGDN sigdnType[] =
		{
			SIGDN_NORMALDISPLAY, SIGDN_PARENTRELATIVEPARSING, SIGDN_DESKTOPABSOLUTEPARSING, SIGDN_PARENTRELATIVEEDITING, SIGDN_DESKTOPABSOLUTEEDITING,
			SIGDN_FILESYSPATH, SIGDN_URL, SIGDN_PARENTRELATIVEFORADDRESSBAR, SIGDN_PARENTRELATIVE, SIGDN_PARENTRELATIVEFORUI
		};

		TRACE( "\n" );
		for ( size_t i = 0; i != COUNT_OF( sigdnType ); ++i )
			TRACE( _T("%s:\t\"%s\"\n"),
				   GetTags_SIGDN().FormatKey( sigdnType[i] ).c_str(),
				   shell::pidl::GetName( reinterpret_cast<PCIDLIST_ABSOLUTE>( pidl ), sigdnType[i] ).c_str() );
	}

	int TrackContextMenu( IContextMenu* pCtxMenu )
	{
		int cmdId = 0;

		if ( pCtxMenu != nullptr )
		{
			CTextClipboard::CMessageWnd msgWnd;
			CShellContextMenuHost menuHost( CWnd::FromHandle( msgWnd.GetWnd() ), pCtxMenu );

			cmdId = menuHost.TrackMenu( CPoint( 300, 100 ) );
		}
		else
			ASSERT( false );

		return cmdId;
	}

	std::tstring ParsingToEditingName( const shell::TPath& shellPath )
	{
		shell::CPidlAbsolute absPidl;

		if ( absPidl.CreateFromShellPath( shellPath.GetPtr() ) )
			return absPidl.GetEditingName();

		return shellPath.Get();		// on error fallback to the original parsing path
	}

	void QueryEditingNames( OUT std::vector<std::tstring>& rEditingNames, const std::vector<shell::TPath>& shellPaths, const std::tstring& stripCommonName /*= str::GetEmpty()*/ )
	{
		utl::transform( shellPaths, rEditingNames, &ut::ParsingToEditingName );

		if ( !stripCommonName.empty() )
			path::StripDirPrefixes( rEditingNames, stripCommonName.c_str() );
	}

	std::tstring JoinEditingNames( const std::vector<shell::TPath>& shellPaths, const std::tstring& stripCommonName /*= str::GetEmpty()*/, const TCHAR* pSep /*= _T("\n")*/ )
	{
		std::vector<std::tstring> editingNames;
		QueryEditingNames( editingNames, shellPaths, stripCommonName );
		return str::Join( editingNames, pSep );
	}
}


#endif //USE_UT
