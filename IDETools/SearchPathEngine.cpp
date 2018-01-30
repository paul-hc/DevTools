
#include "stdafx.h"
#include "SearchPathEngine.h"
#include "IncludeOptions.h"
#include "ModuleSession.h"
#include "Application.h"
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
			if ( includeTag.IsLocalInclude() && HasFlag( m_searchInPath, sp::LocalPath ) )		// local directory?
				rResults.AddValidPath( path::Combine( m_localDirPath.GetPtr(), includeTag.GetFilePath().GetPtr() ), LocalPath );

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

	void CSearchPathEngine::SearchIncludePaths( CFoundPaths& rResults, const CIncludeTag& includeTag ) const
	{
		const std::vector< CSearchPathEngine::DirSearchPair >& specs = GetSearchSpecs();

		for ( std::vector< CSearchPathEngine::DirSearchPair >::const_iterator itSpec = specs.begin(); itSpec != specs.end(); ++itSpec )
			if ( HasFlag( m_searchInPath, itSpec->second ) )		// location selected?
				for ( std::vector< fs::CPath >::const_iterator itDirPath = itSpec->first->GetPaths().begin(); itDirPath != itSpec->first->GetPaths().end(); ++itDirPath )
				{
					fs::CPath dirPath = path::IsRelative( itDirPath->GetPtr() )
						? path::Combine( m_localDirPath.GetPtr(), itDirPath->GetPtr() )
						: *itDirPath;

					rResults.AddValidPath( path::Combine( dirPath.GetPtr(), includeTag.GetFilePath().GetPtr() ), itSpec->first->GetLocation() );
					if ( rResults.IsFull() )
						return;
				}
	}

	const std::vector< CSearchPathEngine::DirSearchPair >& CSearchPathEngine::GetSearchSpecs( void )
	{
		static std::vector< DirSearchPair > groupSepcs;
		if ( groupSepcs.empty() )
		{
			const CIncludePaths& includePaths = app::GetIncludePaths();

			groupSepcs.push_back( DirSearchPair( &includePaths.GetStandard(), sp::StandardPath ) );
			groupSepcs.push_back( DirSearchPair( &CIncludePaths::Get_INCLUDE(), sp::StandardPath ) );
			groupSepcs.push_back( DirSearchPair( &CIncludeOptions::Instance().m_additionalIncludePath, sp::AdditionalPath ) );
			groupSepcs.push_back( DirSearchPair( &app::GetModuleSession().m_moreAdditionalIncludePath, sp::AdditionalPath ) );
			groupSepcs.push_back( DirSearchPair( &includePaths.GetSource(), sp::SourcePath ) );
			groupSepcs.push_back( DirSearchPair( &includePaths.GetLibrary(), sp::LibraryPath ) );
			groupSepcs.push_back( DirSearchPair( &CIncludePaths::Get_LIB(), sp::LibraryPath ) );
			groupSepcs.push_back( DirSearchPair( &includePaths.GetBinary(), sp::BinaryPath ) );
			groupSepcs.push_back( DirSearchPair( &CIncludePaths::Get_PATH(), sp::BinaryPath ) );
		}
		return groupSepcs;
	}
}
