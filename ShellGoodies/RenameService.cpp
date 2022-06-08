
#include "stdafx.h"
#include "RenameService.h"
#include "RenameItem.h"
#include "TextAlgorithms.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/LongestCommonDuplicate.h"
#include "utl/PathGenerator.h"
#include "utl/StringUtilities.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"

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


void CRenameService::StoreRenameItems( const std::vector< CRenameItem* >& renameItems )
{
	REQUIRE( !renameItems.empty() );

	m_renamePairs.Clear();
	ren::MakePairsFromItems( &m_renamePairs, renameItems );

	ENSURE( m_renamePairs.GetPairs().size() == renameItems.size() );			// all SRC keys unique?
}

UINT CRenameService::FindNextAvailSeqCount( const CPathFormatter& formatter ) const
{
	CPathGenerator generator( m_renamePairs, formatter );
	return generator.FindNextAvailSeqCount();
}

bool CRenameService::CheckPathCollisions( cmd::IErrorObserver* pErrorObserver )
{
	if ( pErrorObserver != NULL )
		pErrorObserver->ClearFileErrors();

	fs::TPathSet destPaths;
	size_t dupCount = 0, emptyCount = 0;

	for ( CPathRenamePairs::const_iterator itPair = m_renamePairs.Begin(); itPair != m_renamePairs.End(); ++itPair )
		if ( itPair->second.IsEmpty() )									// ignore empty dest paths
			++emptyCount;
		else if ( !destPaths.insert( itPair->second ).second ||			// not unique in the working set
				  FileExistOutsideWorkingSet( itPair->second ) )		// collides with an existing file/dir outside of the working set
		{
			static const std::tstring s_errMsg = _T("Destination file collision");
			if ( pErrorObserver != NULL )
				pErrorObserver->OnFileError( itPair->first, s_errMsg );
			++dupCount;
		}

	return 0 == dupCount;
}

bool CRenameService::FileExistOutsideWorkingSet( const fs::CPath& filePath ) const
{
	return
		!m_renamePairs.ContainsSrc( filePath ) &&
		filePath.FileExist();
}

void CRenameService::QueryDestFilenames( std::vector< std::tstring >& rDestFnames ) const
{
	rDestFnames.clear();
	rDestFnames.reserve( m_renamePairs.GetPairs().size() );

	for ( CPathRenamePairs::const_iterator itPair = m_renamePairs.Begin(); itPair != m_renamePairs.End(); ++itPair )
		rDestFnames.push_back( GetDestFname( itPair ) );

	if ( rDestFnames.size() > PickFilenameMaxCount )
		rDestFnames.resize( PickFilenameMaxCount );

	ENSURE( !rDestFnames.empty() );
}

std::auto_ptr<CPickDataset> CRenameService::MakeFnamePickDataset( void ) const
{
	std::vector< std::tstring > destFnames;
	QueryDestFilenames( destFnames );

	return std::auto_ptr<CPickDataset>( new CPickDataset( &destFnames ) );
}

std::auto_ptr<CPickDataset> CRenameService::MakeDirPickDataset( void ) const
{
	return std::auto_ptr<CPickDataset>( new CPickDataset( GetDestPath( m_renamePairs.Begin() ) ) );
}

fs::CPath CRenameService::GetDestPath( CPathRenamePairs::const_iterator itPair )
{
	return itPair->second.IsEmpty() ? itPair->first : itPair->second;
}

std::tstring CRenameService::GetDestFname( CPathRenamePairs::const_iterator itPair )
{
	fs::CPathParts destParts;
	ren::SplitPath( &destParts, &itPair->first, GetDestPath( itPair ) );
	return destParts.m_fname;		// DEST (if not empty) or SRC
}

std::tstring CRenameService::ApplyTextTool( UINT menuId, const std::tstring& text )
{
	std::tstring output = text;

	switch ( menuId )
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
			text_tool::ExecuteTextTool( output, func::ReplaceDelimiterSet( delim::GetAllDelimitersSet(), _T(" ") ) );
			break;
		case ID_TEXT_REPLACE_UNICODE_SYMBOLS:
			text_tool::ExecuteTextTool( output, func::ReplaceMultiDelimiterSets( &text_tool::GetStdUnicodeToAnsiPairs() ) );
			break;
		case ID_TEXT_SINGLE_WHITESPACE:
			text_tool::ExecuteTextTool( output, func::SingleWhitespace() );
			break;
		case ID_TEXT_REMOVE_WHITESPACE:
			text_tool::ExecuteTextTool( output, func::RemoveWhitespace() );
			break;
		case ID_TEXT_DASH_TO_SPACE:
			text_tool::ExecuteTextTool( output, func::ReplaceDelimiterSet( delim::s_dashes, _T(" ") ) );
			break;
		case ID_TEXT_SPACE_TO_DASH:
			text_tool::ExecuteTextTool( output, func::ReplaceDelimiterSet( _T(" "), _T("-") ) );
			break;
		case ID_TEXT_UNDERBAR_TO_SPACE:
			text_tool::ExecuteTextTool( output, func::ReplaceDelimiterSet( _T("_"), _T(" ") ) );
			break;
		case ID_TEXT_SPACE_TO_UNDERBAR:
			text_tool::ExecuteTextTool( output, func::ReplaceDelimiterSet( _T(" "), _T("_") ) );
			break;
		default:
			ASSERT( false );
	}
	return output;
}


// CPickDataset implementation

CPickDataset::CPickDataset( std::vector< std::tstring >* pDestFnames )
	: m_bestMatch( Empty )
{
	ASSERT_PTR( pDestFnames );
	m_destFnames.swap( *pDestFnames );

	if ( m_destFnames.size() > 1 )			// multiple files?
	{
		std::tstring commonPrefix = ExtractLongestCommonPrefix();
		std::tstring commonSubstring = str::FindLongestCommonSubstring( m_destFnames, pred::TCompareNoCase() );

		str::Trim( commonPrefix );			// remove trailing spaces
		str::Trim( commonSubstring );

		if ( commonPrefix.size() > commonSubstring.size() )
		{
			m_bestMatch = CommonPrefix;
			m_commonSequence = commonPrefix;
		}
		else if ( !commonSubstring.empty() )
		{
			m_bestMatch = CommonSubstring;
			m_commonSequence = commonSubstring;
		}
	}

	ENSURE( !m_destFnames.empty() );
}

CPickDataset::CPickDataset( const fs::CPath& firstDestPath )
	: m_bestMatch( Empty )
{
	fs::CPathParts parts( firstDestPath.Get() );
	str::Tokenize( m_subDirs, parts.m_dir.c_str(), _T("\\/") );

	if ( m_subDirs.size() > PickDirPathMaxCount )
		m_subDirs.erase( m_subDirs.begin(), m_subDirs.begin() + m_subDirs.size() - PickDirPathMaxCount );
}

std::tstring CPickDataset::ExtractLongestCommonPrefix( void ) const
{
	REQUIRE( m_destFnames.size() > 1 );

	size_t maxLen = std::max_element( m_destFnames.begin(), m_destFnames.end(), pred::LessValue< pred::CompareLength >() )->length();

	for ( size_t prefixLen = 1; prefixLen <= maxLen; ++prefixLen )
		if ( !AllHavePrefix( prefixLen ) )
			return m_destFnames.front().substr( 0, prefixLen - 1 );		// previous max common prefix

	return std::tstring();
}

bool CPickDataset::AllHavePrefix( size_t prefixLen ) const
{
	REQUIRE( prefixLen != 0 );
	REQUIRE( m_destFnames.size() > 1 );

	std::vector< std::tstring >::const_iterator itFirstFname = m_destFnames.begin();

	if ( m_destFnames.size() > 1 )
		for ( std::vector< std::tstring >::const_iterator itFname = itFirstFname + 1; itFname != m_destFnames.end(); ++itFname )
			if ( !str::EqualsIN( itFname->c_str(), itFirstFname->c_str(), prefixLen ) )
				return false;

	return true;		// all strings match the prefix
}

void CPickDataset::MakePickFnameMenu( CMenu* pPopupMenu, const TCHAR* pSelFname /*= NULL*/ ) const
{
	REQUIRE( !m_destFnames.empty() );

	ASSERT_PTR( pPopupMenu );
	pPopupMenu->DestroyMenu();

	pPopupMenu->CreatePopupMenu();						// multiple pick menu
	UINT cmdId = IDC_PICK_FILENAME_BASE, selId = 0;

	if ( HasCommonSequence() )
	{
		pPopupMenu->AppendMenu( MF_STRING, cmdId++, EscapeAmpersand( m_commonSequence ) );
		pPopupMenu->AppendMenu( MF_SEPARATOR );
	}

	for ( std::vector< std::tstring >::const_iterator itFname = m_destFnames.begin(); itFname != m_destFnames.end(); ++itFname, ++cmdId )
	{
		pPopupMenu->AppendMenu( MF_STRING, cmdId, EscapeAmpersand( *itFname ) );

		if ( 0 == selId )
			if ( pSelFname != NULL && str::Matches( itFname->c_str(), pSelFname, false, false ) )
				selId = cmdId;
	}

	if ( selId != 0 )
		pPopupMenu->CheckMenuRadioItem( IDC_PICK_FILENAME_BASE, selId, selId, MF_BYCOMMAND );
}

std::tstring CPickDataset::GetPickedFname( UINT cmdId ) const
{
	REQUIRE( !m_destFnames.empty() );

	size_t pos = cmdId - IDC_PICK_FILENAME_BASE;

	if ( HasCommonSequence() )
		if ( 0 == pos )				// picked the common prefix?
			return m_commonSequence;
		else
			--pos;					// index in m_destFnames

	if ( pos < m_destFnames.size() )
		return m_destFnames[ pos ];

	ASSERT( false );
	return std::tstring();
}

void CPickDataset::MakePickDirMenu( CMenu* pPopupMenu ) const
{
	REQUIRE( !m_subDirs.empty() );

	ASSERT_PTR( pPopupMenu );
	pPopupMenu->DestroyMenu();

	pPopupMenu->CreatePopupMenu();
	UINT cmdId = IDC_PICK_DIR_PATH_BASE;

	for ( std::vector< std::tstring >::const_iterator itSubDir = m_subDirs.begin(), itLast = m_subDirs.end() - 1; itSubDir != m_subDirs.end(); ++itSubDir, ++cmdId )
	{
		pPopupMenu->AppendMenu( MF_STRING, cmdId, EscapeAmpersand( *itSubDir ) );
		ui::SetMenuItemImage( *pPopupMenu, cmdId, itSubDir != itLast ? ID_BROWSE_FILE : ID_PARENT_FOLDER );
	}
}

std::tstring CPickDataset::GetPickedDirectory( UINT cmdId ) const
{
	REQUIRE( !m_subDirs.empty() );

	size_t pos = cmdId - IDC_PICK_DIR_PATH_BASE;
	if ( pos < m_subDirs.size() )
		return m_subDirs[ pos ];

	ASSERT( false );
	return std::tstring();
}

const TCHAR* CPickDataset::EscapeAmpersand( const std::tstring& text )
{
	static std::tstring s_uiText;
	s_uiText = text;
	str::Replace( s_uiText, _T("&"), _T("&&") );
	return s_uiText.c_str();
}
