
#include "stdafx.h"
#include "SearchPathEngine.h"
#include "Application.h"
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CSearchPathEngine::SetDspAdditionalIncludePath( const std::tstring& dspDirPath, const std::tstring& dspAdditionalIncludePath, const TCHAR* pSep )
{
	str::Split( m_dspAdditionalIncludePath, str::ExpandEnvironmentStrings( dspAdditionalIncludePath.c_str() ).c_str(), pSep );

	// resolve relative dir-paths to dspDirPath
	if ( !dspDirPath.empty() )
		for ( std::vector< std::tstring >::iterator itDirPath = m_dspAdditionalIncludePath.begin(); itDirPath != m_dspAdditionalIncludePath.end(); ++itDirPath )
			if ( path::IsRelative( itDirPath->c_str() ) )
				*itDirPath = path::Combine( dspDirPath.c_str(), itDirPath->c_str() );
}

bool CSearchPathEngine::CheckValidFile( PathLocationPair& rFoundFile, const std::tstring& fullPath, loc::IncludeLocation location )
{
	rFoundFile.second = location;
	if ( fs::FileExist( fullPath.c_str() ) )
		rFoundFile.first = path::MakeCanonical( fullPath.c_str() );		// absolute full path
	else
		rFoundFile.first.clear();

	return !rFoundFile.first.empty();
}

// includeTag can have any of following formats: "Header.h", <Header.h> or Header.h

size_t CSearchPathEngine::QueryIncludeFiles( std::vector< PathLocationPair >& rFoundFiles, const CIncludeTag& includeTag, const std::tstring& localDirPath,
											 int searchInPath /*= sp::AllIncludePaths*/, size_t maxCount /*= std::tstring::npos*/ ) const
{
	ASSERT( !includeTag.IsEmpty() );

	const std::tstring& fileName = includeTag.GetFilePath().Get();
	std::set< std::tstring, pred::LessPath > uniquePaths;
	PathLocationPair foundFile;

	if ( path::IsAbsolute( includeTag.GetFilePath().GetPtr() ) )									// #include <D:\MFC\Include\stdio.h>
	{
		if ( CheckValidFile( foundFile, includeTag.GetFilePath().Get(), loc::AbsolutePath ) )		// add absolute path without searching any include path
			if ( rFoundFiles.size() < maxCount && uniquePaths.insert( foundFile.first ).second )	// no overflow and unique
				rFoundFiles.push_back( foundFile );
	}
	else		// #include <stdio.h>, #include <atl/stdio.h>, #include "Options.h", #include "utl/Icon.h"
	{
		if ( includeTag.IsLocalInclude() && HasFlag( searchInPath, sp::LocalPath ) )				// local directory?
			if ( CheckValidFile( foundFile, path::Combine( localDirPath.c_str(), fileName.c_str() ), loc::LocalPath ) )
				if ( rFoundFiles.size() < maxCount && uniquePaths.insert( foundFile.first ).second )
					rFoundFiles.push_back( foundFile );

		// search in the specified dir-paths
		const CIncludePaths& includePaths = app::GetIncludePaths();

		struct { const std::vector< std::tstring >* m_pDirPaths; sp::SearchInPath m_searchFlag; loc::IncludeLocation m_location; } specs[] =
		{
			{ &m_dspAdditionalIncludePath, sp::AdditionalPath, loc::AdditionalPath },
			{ &includePaths.GetStandard(), sp::StandardPath, loc::StandardPath },
			{ &includePaths.GetMoreAdditional(), sp::StandardPath, loc::AdditionalPath },
			{ &includePaths.GetSource(), sp::SourcePath, loc::SourcePath },
			{ &includePaths.GetLibrary(), sp::LibraryPath, loc::LibraryPath },
			{ &includePaths.GetBinary(), sp::BinaryPath, loc::BinaryPath }
		};

		for ( unsigned int i = 0; i != COUNT_OF( specs ); ++i )
			if ( HasFlag( searchInPath, specs[ i ].m_searchFlag ) )
				for ( std::vector< std::tstring >::const_iterator itDirPath = specs[ i ].m_pDirPaths->begin();
					  itDirPath != specs[ i ].m_pDirPaths->end(); ++itDirPath )
				{
					std::tstring dirPath;
					if ( path::IsRelative( itDirPath->c_str() ) )
						dirPath = path::Combine( localDirPath.c_str(), itDirPath->c_str() );
					else
						dirPath = *itDirPath;

					if ( CheckValidFile( foundFile, path::Combine( dirPath.c_str(), fileName.c_str() ), specs[ i ].m_location ) )
						if ( rFoundFiles.size() >= maxCount )
							return rFoundFiles.size();
						else if ( uniquePaths.insert( foundFile.first ).second )
							rFoundFiles.push_back( foundFile );
				}
	}

	return rFoundFiles.size();
}

bool CSearchPathEngine::FindFirstIncludeFile( PathLocationPair& rFoundFile, const CIncludeTag& includeTag, const std::tstring& localDirPath,
											  int searchInPath /*= sp::AllIncludePaths*/ ) const
{
	ASSERT( !includeTag.IsEmpty() );

	std::vector< PathLocationPair > foundFiles;
	QueryIncludeFiles( foundFiles, includeTag, localDirPath, searchInPath, 1 );			// stop after first found include file
	if ( foundFiles.empty() )
		return false;
	rFoundFile = foundFiles.front();
	return true;
}
