
#include "stdafx.h"
#include "Directory.h"
#include "CmdLineOptions.h"
#include "TreeGuides.h"
#include "utl/FileEnumerator.h"
#include "utl/Timer.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace impl
{
	struct CEnumerator : public fs::IEnumerator
	{
		CEnumerator( const CCmdLineOptions& options ) : m_options( options ), m_moreFilesCount( 0 ) {}

		// IEnumerator interface
		virtual bool IncludeNode( const CFileFind& foundNode );
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );
		virtual void AddFoundFile( const TCHAR* pFilePath );

		void OnCompleted( void );
	private:
		const CCmdLineOptions& m_options;
	public:
		std::vector< fs::CPath > m_filePaths;
		std::vector< fs::CPath > m_subDirPaths;
		size_t m_moreFilesCount;			// incremented when it reaches the limit
	};


	// CEnumerator implementation

	bool CEnumerator::IncludeNode( const CFileFind& foundNode )
	{
		if ( foundNode.IsHidden() )
			if ( !m_options.HasOptionFlag( app::ShowHiddenNodes ) )
				return false;

		return true;
	}

	void CEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		if ( m_options.HasOptionFlag( app::DisplayFiles ) )
			if ( m_filePaths.size() < m_options.m_maxDirFiles )
				m_filePaths.push_back( fs::CPath( pFilePath ) );
			else
				++m_moreFilesCount;
	}

	bool CEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		m_subDirPaths.push_back( fs::CPath( pSubDirPath ) );
		return true;
	}

	void CEnumerator::OnCompleted( void )
	{
		if ( !m_options.HasOptionFlag( app::NoSorting ) )
			fs::SortPaths( m_subDirPaths );

		if ( m_options.HasOptionFlag( app::DisplayFiles ) && !m_options.HasOptionFlag( app::NoOutput ) )
		{
			if ( !m_options.HasOptionFlag( app::NoSorting ) )
				fs::SortPaths( m_filePaths );

			if ( m_moreFilesCount != 0 )
				m_filePaths.push_back( str::Format( m_filePaths.empty() ? _T("(%d files)") : _T("(+ %d more files)"), m_moreFilesCount ) );
		}
	}
}


const TCHAR CDirectory::s_wildSpec[] = _T("*");
double CDirectory::s_totalElapsedEnum = 0.0;

CDirectory::CDirectory( const CCmdLineOptions& options )
	: m_options( options )
	, m_dirPath( m_options.m_dirPath )
	, m_depth( 0 )
{
	s_totalElapsedEnum = 0.0;
}

CDirectory::CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath )
	: m_options( pParent->m_options )
	, m_dirPath( subDirPath )
	, m_depth( pParent->m_depth + 1 )
{
}

void CDirectory::List( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix )
{
	impl::CEnumerator found( m_options );
	CTimer enumTimer;

	fs::EnumFiles( &found, m_dirPath, s_wildSpec, fs::EF_NoSortSubDirs );
	found.OnCompleted();
	s_totalElapsedEnum += enumTimer.ElapsedSeconds();

	if ( m_options.HasOptionFlag( app::DisplayFiles ) && !m_options.HasOptionFlag( app::NoOutput ) )
		if ( !found.m_filePaths.empty() )
		{
			std::wstring fullPrefix = parentNodePrefix + guideParts.GetFilePrefix( !found.m_subDirPaths.empty() );

			for ( size_t pos = 0; pos != found.m_filePaths.size(); ++pos )
				os
					<< fullPrefix
					<< found.m_filePaths[ pos ].GetFilenamePtr()
					<< std::endl;

			if ( !str::TrimRight( fullPrefix ).empty() )		// remove trailing spaces on empty line
				os << fullPrefix << std::endl;
		}

	for ( CPagePos subDirPos( found.m_subDirPaths ); !subDirPos.AtEnd(); ++subDirPos )
	{
		const fs::CPath& subDirPath = found.m_subDirPaths[ subDirPos.m_pos ];

		if ( !m_options.HasOptionFlag( app::NoOutput ) )
			os
				<< parentNodePrefix << guideParts.GetSubDirPrefix( subDirPos )
				<< subDirPath.GetFilenamePtr()
				<< std::endl;

		if ( m_depth + 1 < m_options.m_maxDepthLevel )
		{	// recurse
			CDirectory subDirectory( this, subDirPath );
			subDirectory.List( os, guideParts, parentNodePrefix + guideParts.GetSubDirRecursePrefix( subDirPos ) );
		}
	}
}
