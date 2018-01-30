#ifndef SearchPathEngine_h
#define SearchPathEngine_h
#pragma once

#include "utl/Path.h"
#include "IncludePaths.h"
#include "IncludeNode.h"
#include "SearchPathEngine_fwd.h"


namespace inc
{
	class CFoundPaths
	{
	public:
		CFoundPaths( size_t maxCount = utl::npos ) : m_maxCount( maxCount ) { ASSERT( m_maxCount != 0 ); }

		bool IsFull( void ) const { return m_foundFiles.size() >= m_maxCount; }

		const std::vector< TPathLocPair >& Get( void ) const { return m_foundFiles; }
		void Swap( std::vector< TPathLocPair >& rFoundFiles ) { rFoundFiles.swap( m_foundFiles ); }

		bool AddValidPath( const fs::CPath& fullPath, Location location );
	private:
		size_t m_maxCount;
		std::vector< TPathLocPair > m_foundFiles;
		fs::PathSet m_uniquePaths;
	};


	class CSearchPathEngine
	{
	public:
		CSearchPathEngine( const fs::CPath& localDirPath, int searchInPath = sp::AllIncludePaths ) : m_localDirPath( localDirPath ), m_searchInPath( searchInPath ) {}

		void QueryIncludeFiles( CFoundPaths& rResults, const CIncludeTag& includeTag ) const;
		TPathLocPair FindFirstIncludeFile( const CIncludeTag& includeTag ) const;
	private:
		void SearchIncludePaths( CFoundPaths& rResults, const CIncludeTag& includeTag ) const;

		typedef std::pair< const CDirPathGroup*, sp::SearchInPath > DirSearchPair;

		static const std::vector< DirSearchPair >& GetSearchSpecs( void );
	private:
		fs::CPath m_localDirPath;
		int m_searchInPath;
	};
}


#endif // SearchPathEngine_h
