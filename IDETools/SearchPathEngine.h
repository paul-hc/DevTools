#ifndef SearchPathEngine_h
#define SearchPathEngine_h
#pragma once

#include "utl/Path.h"
#include "IncludePaths.h"


class CIncludeTag;


namespace inc
{
	class CFoundPaths
	{
	public:
		CFoundPaths( size_t maxCount = utl::npos ) : m_maxCount( maxCount ) { ASSERT( m_maxCount != 0 ); }

		bool IsFull( void ) const { return m_foundFiles.size() >= m_maxCount; }

		const std::vector<TPathLocPair>& Get( void ) const { return m_foundFiles; }
		void Swap( std::vector<TPathLocPair>& rFoundFiles ) { rFoundFiles.swap( m_foundFiles ); }

		bool AddValidPath( const fs::CPath& fullPath, Location location );
	private:
		size_t m_maxCount;
		std::vector<TPathLocPair> m_foundFiles;
		fs::TPathSet m_uniquePaths;
	};


	class CSearchPathEngine
	{
	public:
		CSearchPathEngine( const fs::CPath& localDirPath, TSearchFlags searchFlags = Mask_AllIncludePaths ) : m_localDirPath( localDirPath ), m_searchFlags( searchFlags ) {}

		void QueryIncludeFiles( CFoundPaths& rResults, const CIncludeTag& includeTag ) const;
		TPathLocPair FindFirstIncludeFile( const CIncludeTag& includeTag ) const;
	private:
		fs::CPath MakeDirPath( const fs::CPath& srcDirPath ) const;
		void SearchIncludePaths( CFoundPaths& rResults, const CIncludeTag& includeTag ) const;
	private:
		fs::CPath m_localDirPath;
		inc::TSearchFlags m_searchFlags;
	};
}


#endif // SearchPathEngine_h
