
#include "stdafx.h"
#include "FileSetUi.h"
#include "TitleCapitalizer.h"
#include "resource.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pred
{
	struct CompareLength
	{
		template< typename StringType >
		CompareResult operator()( const StringType& left, const StringType& right ) const
		{
			return Compare_Scalar( left.length(), right.length() );
		}
	};
}


CFileSetUi::CFileSetUi( CFileWorkingSet* pFileWorkingSet )
	: m_renamePairs( pFileWorkingSet->GetRenamePairs() )
{
	ASSERT( !m_renamePairs.empty() );
}

void CFileSetUi::QueryDestFilenames( std::vector< std::tstring >& rDestFnames ) const
{
	rDestFnames.clear();
	rDestFnames.reserve( m_renamePairs.size() );

	for ( fs::PathPairMap::const_iterator itPair = m_renamePairs.begin(); itPair != m_renamePairs.end(); ++itPair )
		rDestFnames.push_back( GetDestFname( itPair ) );

	if ( rDestFnames.size() > PickFilenameMaxCount )
		rDestFnames.resize( PickFilenameMaxCount );

	ENSURE( !rDestFnames.empty() );
}

void CFileSetUi::QuerySubDirs( std::vector< std::tstring >& rSubDirs ) const
{
	fs::CPathParts parts( GetDestPath( m_renamePairs.begin() ) );			// use the first path
	str::Tokenize( rSubDirs, parts.m_dir.c_str(), _T("\\/") );

	if ( rSubDirs.size() > PickDirPathMaxCount )
		rSubDirs.erase( rSubDirs.begin(), rSubDirs.begin() + rSubDirs.size() - PickDirPathMaxCount );
}

std::tstring CFileSetUi::ExtractLongestCommonPrefix( const std::vector< std::tstring >& destFnames ) const
{
	ASSERT( !destFnames.empty() );

	if ( 1 == destFnames.size() )
		return destFnames.front();
	else
	{
		size_t maxLen = std::max_element( destFnames.begin(), destFnames.end(), pred::LessBy< pred::CompareLength >() )->length();

		for ( size_t prefixLen = 1; prefixLen <= maxLen; ++prefixLen )
			if ( !AllHavePrefix( destFnames, prefixLen ) )
				return destFnames.front().substr( 0, prefixLen - 1 );		// previous max common prefix
	}

	return std::tstring();
}

bool CFileSetUi::AllHavePrefix( const std::vector< std::tstring >& destFnames, size_t prefixLen ) const
{
	ASSERT( prefixLen != 0 );

	if ( destFnames.empty() )
		return false;

	std::vector< std::tstring >::const_iterator itFirstFname = destFnames.begin();

	if ( destFnames.size() > 1 )
		for ( std::vector< std::tstring >::const_iterator itFname = itFirstFname + 1; itFname != destFnames.end(); ++itFname )
			if ( !str::EqualsN( itFname->c_str(), itFirstFname->c_str(), prefixLen, false ) )
				return false;

	return true;		// all strings match the prefix
}

void CFileSetUi::MakePickFnamePatternMenu( std::tstring* pSinglePattern, CMenu* pPopupMenu, const TCHAR* pSelFname /*= NULL*/ ) const
{
	ASSERT_PTR( pSinglePattern );
	ASSERT_PTR( pPopupMenu );
	pSinglePattern->clear();
	pPopupMenu->DestroyMenu();

	std::vector< std::tstring > destFnames;
	QueryDestFilenames( destFnames );

	if ( 1 == destFnames.size() || ui::IsKeyPressed( VK_CONTROL ) )
	{
		std::tstring singlePattern = destFnames.front();
		if ( destFnames.size() != 1 )
		{
			std::tstring maxCommonPrefix = ExtractLongestCommonPrefix( destFnames );
			str::Trim( maxCommonPrefix );				// remove trailing spaces
			if ( !maxCommonPrefix.empty() )
				singlePattern = maxCommonPrefix;
		}

		if ( !singlePattern.empty() )
		{
			*pSinglePattern = singlePattern;
			return;								// found single pattern, no menu
		}
	}

	// multiple pick menu
	pPopupMenu->CreatePopupMenu();
	UINT cmdId = IDC_PICK_FILENAME_BASE, selId = 0;

	for ( std::vector< std::tstring >::iterator itFname = destFnames.begin(); itFname != destFnames.end(); ++itFname, ++cmdId )
	{
		EscapeAmpersand( *itFname );
		pPopupMenu->AppendMenu( MF_STRING, cmdId, itFname->c_str() );

		if ( pSelFname != NULL && str::Matches( itFname->c_str(), pSelFname, false, false ) )
			selId = cmdId;
	}

	if ( selId != 0 )
		pPopupMenu->CheckMenuRadioItem( IDC_PICK_FILENAME_BASE, selId, selId, MF_BYCOMMAND );
}

bool CFileSetUi::MakePickDirPathMenu( UINT* pSingleCmdId, CMenu* pPopupMenu ) const
{
	ASSERT_PTR( pSingleCmdId );
	ASSERT_PTR( pPopupMenu );
	*pSingleCmdId = 0;
	pPopupMenu->DestroyMenu();

	std::vector< std::tstring > subDirs;
	QuerySubDirs( subDirs );
	if ( subDirs.empty() )
		return false;															// root path, no sub-dirs to pick

	if ( 1 == subDirs.size() || ui::IsKeyPressed( VK_CONTROL ) )
		*pSingleCmdId = IDC_PICK_DIR_PATH_BASE + (UINT)subDirs.size() - 1;		// pick the parent directory
	else
	{
		// multiple pick menu
		pPopupMenu->CreatePopupMenu();
		UINT cmdId = IDC_PICK_DIR_PATH_BASE;

		for ( std::vector< std::tstring >::iterator itSubDir = subDirs.begin(), itLast = subDirs.end() - 1; itSubDir != subDirs.end(); ++itSubDir, ++cmdId )
		{
			EscapeAmpersand( *itSubDir );
			pPopupMenu->AppendMenu( MF_STRING, cmdId, itSubDir->c_str() );
			ui::SetMenuItemImage( *pPopupMenu, cmdId, itSubDir != itLast ? ID_BROWSE_FILE : ID_PARENT_FOLDER );
		}
	}
	return true;
}

std::tstring CFileSetUi::GetPickedFname( UINT cmdId, std::vector< std::tstring >* pDestFnames /*= NULL*/ ) const
{
	std::vector< std::tstring > destFnames;
	QueryDestFilenames( destFnames );

	size_t pos = cmdId - IDC_PICK_FILENAME_BASE;
	std::tstring selFname;
	if ( pos < destFnames.size() )
		selFname = destFnames[ pos ];
	else
		ASSERT( false );

	if ( pDestFnames != NULL )
		pDestFnames->swap( destFnames );

	return selFname;
}

std::tstring CFileSetUi::GetPickedDirectory( UINT cmdId ) const
{
	std::vector< std::tstring > subDirs;
	QuerySubDirs( subDirs );

	size_t pos = cmdId - IDC_PICK_DIR_PATH_BASE;
	if ( pos < subDirs.size() )
		return subDirs[ pos ];

	ASSERT( false );
	return std::tstring();
}

std::tstring CFileSetUi::GetDestPath( fs::PathPairMap::const_iterator itPair )
{
	return itPair->second.IsEmpty() ? itPair->first.Get() : itPair->second.Get();
}

std::tstring CFileSetUi::GetDestFname( fs::PathPairMap::const_iterator itPair )
{
	return fs::CPathParts( GetDestPath( itPair ) ).m_fname;		// DEST (if not empty) or SRC
}

std::tstring& CFileSetUi::EscapeAmpersand( std::tstring& rText )
{
	str::Replace( rText, _T("&"), _T("&&") );
	return rText;
}

std::tstring CFileSetUi::ApplyTextTool( UINT cmdId, const std::tstring& text )
{
	std::tstring output = text;
	static const TCHAR defaultDelimiterSet[] = _T(".;-_ \t");

	switch ( cmdId )
	{
		case ID_TEXT_TITLE_CASE:
		{
			CTitleCapitalizer capitalizer;
			capitalizer.Capitalize( output );
			break;
		}
		case ID_TEXT_LOWER_CASE:
			str::ToLower( output );
			break;
		case ID_TEXT_UPPER_CASE:
			str::ToUpper( output );
			break;
		case ID_TEXT_REPLACE_DELIMS:
			str::ReplaceDelimiters( output, defaultDelimiterSet, _T(" ") );
			str::EnsureSingleSpace( output );
			break;
		case ID_TEXT_SINGLE_WHITESPACE:
			str::EnsureSingleSpace( output );
			break;
		case ID_TEXT_REMOVE_WHITESPACE:
			str::ReplaceDelimiters( output, _T(" \t"), _T("") );
			break;
		case ID_TEXT_DASH_TO_SPACE:
			str::ReplaceDelimiters( output, _T("-"), _T(" ") );
			break;
		case ID_TEXT_SPACE_TO_DASH:
			str::ReplaceDelimiters( output, _T(" "), _T("-") );
			break;
		case ID_TEXT_UNDERBAR_TO_SPACE:
			str::ReplaceDelimiters( output, _T("_"), _T(" ") );
			break;
		case ID_TEXT_SPACE_TO_UNDERBAR:
			str::ReplaceDelimiters( output, _T(" "), _T("_") );
			break;
	}
	return output;
}