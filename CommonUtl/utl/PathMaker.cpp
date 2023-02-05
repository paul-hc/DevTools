
#include "pch.h"
#include "PathMaker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathMaker::CPathMaker( void )
	: m_pRenamePairs( new CPathRenamePairs() )
	, m_mapOwnership( true )
{
	ASSERT_PTR( m_pRenamePairs );
}

CPathMaker::CPathMaker( CPathRenamePairs* pRenamePairs )
	: m_pRenamePairs( pRenamePairs )
	, m_mapOwnership( false )
{
	ASSERT_PTR( m_pRenamePairs );
}

CPathMaker::~CPathMaker()
{
	if ( m_mapOwnership )
		delete m_pRenamePairs;
}

bool CPathMaker::MakeDestRelative( const std::tstring& prefixDirPath )
{
	// for each copy SRC path to DEST path, and make relative DEST path by removing the reference dir path suffix.

	for ( CPathRenamePairs::TPairVector::iterator itPair = m_pRenamePairs->RefPairs().begin(); itPair != m_pRenamePairs->RefPairs().end(); ++itPair )
	{
		std::tstring destRelPath = itPair->first.GetPtr();

		if ( path::StripPrefix( destRelPath, prefixDirPath.c_str() ) )
			itPair->second.Set( destRelPath );
		else
			return false;			// SRC path does not share a common prefix with prefixDirPath
	}
	return true;
}

bool CPathMaker::MakeDestStripCommonPrefix( void )
{
	std::tstring commonPrefix = FindSrcCommonPrefix();
	return !commonPrefix.empty() && MakeDestRelative( commonPrefix );
}

std::tstring CPathMaker::FindSrcCommonPrefix( void ) const
{
	std::tstring commonPrefix;

	if ( !m_pRenamePairs->GetPairs().empty() )
	{
		CPathRenamePairs::const_iterator itPair = m_pRenamePairs->Begin();
		commonPrefix = itPair->first.GetParentPath().Get();

		for ( ++itPair; itPair != m_pRenamePairs->End(); ++itPair )
		{
			commonPrefix = path::FindCommonPrefix( itPair->first.GetPtr(), commonPrefix.c_str() );
			if ( commonPrefix.empty() )
				break;						// no common prefix, abort search
		}
	}
	return commonPrefix;
}
