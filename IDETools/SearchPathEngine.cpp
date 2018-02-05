
#include "stdafx.h"
#include "SearchPathEngine.h"
#include "IncludeDirectories.h"
#include "IncludeNode.h"
#include "utl/StringUtilities.h"
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace inc
{
	// CFoundPaths implementation

	bool CFoundPaths::AddValidPath( const fs::CPath& fullPath, Location location )
	{
		if ( fullPath.FileExist() )
			if ( m_uniquePaths.insert( fullPath ).second )		// unique?
			{
				m_foundFiles.push_back( TPathLocPair( fullPath.Get(), location ) );
				return true;
			}

		return false;
	}


	// CSearchPathEngine implementation

	void CSearchPathEngine::QueryIncludeFiles( CFoundPaths& rResults, const CIncludeTag& includeTag ) const
	{
		// includeTag can have any of following formats: "Header.h", <Header.h> or Header.h
		ASSERT( !includeTag.IsEmpty() );

		if ( path::IsAbsolute( includeTag.GetFilePath().GetPtr() ) )							// #include <D:\MFC\Include\stdio.h>
			rResults.AddValidPath( includeTag.GetFilePath().Get(), AbsolutePath );				// add absolute path without searching any include path
		else	// #include <stdio.h>, #include <atl/stdio.h>, #include "Options.h", #include "utl/Icon.h"
		{
			if ( includeTag.IsLocalInclude() && HasFlag( m_searchFlags, Flag_LocalPath ) )		// local directory?
				rResults.AddValidPath( m_localDirPath / includeTag.GetFilePath(), LocalPath );

			if ( !rResults.IsFull() )
				SearchIncludePaths( rResults, includeTag );				// search in the specified dir-paths
		}
	}

	TPathLocPair CSearchPathEngine::FindFirstIncludeFile( const CIncludeTag& includeTag ) const
	{
		CFoundPaths foundResults( 1 );		// stop after the first match

		QueryIncludeFiles( foundResults, includeTag );
		if ( foundResults.Get().empty() )
			return TPathLocPair();

		ENSURE( 1 == foundResults.Get().size() );
		return foundResults.Get().front();
	}

	fs::CPath CSearchPathEngine::MakeDirPath( const fs::CPath& srcDirPath ) const
	{
		if ( path::IsRelative( srcDirPath.GetPtr() ) )
			return m_localDirPath / srcDirPath;

		return srcDirPath;
	}

	void CSearchPathEngine::SearchIncludePaths( CFoundPaths& rResults, const CIncludeTag& includeTag ) const
	{
		const std::vector< TDirSearchPair >& specs = CIncludeDirectories::Instance().GetSearchSpecs();

		for ( std::vector< TDirSearchPair >::const_iterator itSpec = specs.begin(); itSpec != specs.end(); ++itSpec )
			if ( HasFlag( m_searchFlags, itSpec->second ) )			// location selected?
				for ( std::vector< fs::CPath >::const_iterator itDirPath = itSpec->first->GetPaths().begin(); itDirPath != itSpec->first->GetPaths().end(); ++itDirPath )
				{
					fs::CPath fullPath = MakeDirPath( *itDirPath ) / includeTag.GetFilePath();

					rResults.AddValidPath( fullPath, itSpec->first->GetLocation() );
					if ( rResults.IsFull() )
						return;
				}
	}
}
