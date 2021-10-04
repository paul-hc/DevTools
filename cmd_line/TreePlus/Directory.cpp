
#include "stdafx.h"
#include "Directory.h"
#include "CmdLineOptions.h"
#include "GuidesOutput.h"
#include "utl/FileEnumerator.h"
#include <iostream>
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace impl
{
	struct CEnumerator : public fs::IEnumerator
	{
		CEnumerator( const CCmdLineOptions& options ) : m_options( options ), m_moreFilesCount( 0 ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );

		void OnCompleted( void );
	private:
		const CCmdLineOptions& m_options;
	public:
		std::vector< fs::CPath > m_filePaths;
		std::vector< fs::CPath > m_subDirPaths;
		size_t m_moreFilesCount;			// incremented when it reaches the limit
	};


	// CEnumerator implementation

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

CDirectory::CDirectory( const CCmdLineOptions& options )
	: m_dirPath( options.m_dirPath )
	, m_level( 0 )
	, m_options( options )
{
}

CDirectory::CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath )
	: m_dirPath( subDirPath )
	, m_level( pParent->m_level + 1 )
	, m_options( pParent->m_options )
{
}

void CDirectory::List( std::wostream& os, const CGuideParts& guideParts, const std::wstring& parentNodePrefix )
{
	impl::CEnumerator found( m_options );

	fs::EnumFiles( &found, m_dirPath, s_wildSpec, Shallow, false );
	found.OnCompleted();

	if ( m_options.HasOptionFlag( app::DisplayFiles ) && !m_options.HasOptionFlag( app::NoOutput ) )
		if ( !found.m_filePaths.empty() )
		{
			std::tstring fileFullPrefix = parentNodePrefix + guideParts.GetFilePrefix( !found.m_subDirPaths.empty() );

			for ( CPagePos filePos( found.m_filePaths ); !filePos.AtEnd(); ++filePos )
				os
					<< fileFullPrefix
					<< found.m_filePaths[ filePos.m_pos ].GetFilenamePtr()
					<< std::endl;

			if ( !str::TrimRight( fileFullPrefix ).empty() )		// remove trailing spaces on empty line
				os << fileFullPrefix << std::endl;
		}

	size_t deepLevel = m_level + 1;

	for ( CPagePos subDirPos( found.m_subDirPaths ); !subDirPos.AtEnd(); ++subDirPos )
	{
		const fs::CPath& subDirPath = found.m_subDirPaths[ subDirPos.m_pos ];

		if ( !m_options.HasOptionFlag( app::NoOutput ) )
			os
				<< parentNodePrefix << guideParts.GetSubDirPrefix( subDirPos )
				<< subDirPath.GetFilenamePtr()
				<< std::endl;

		if ( deepLevel < m_options.m_maxDepthLevel )
		{
			CDirectory subDirectory( this, subDirPath );
			subDirectory.List( os, guideParts, parentNodePrefix + guideParts.GetSubDirRecursePrefix( subDirPos ) );
		}
	}
}
