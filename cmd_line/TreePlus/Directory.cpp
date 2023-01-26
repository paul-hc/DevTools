
#include "stdafx.h"
#include "Directory.h"
#include "CmdLineOptions.h"
#include "Table.h"
#include "TreeGuides.h"
#include "utl/FileEnumerator.h"
#include "utl/Timer.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	struct CDirEnumerator : public fs::CPathEnumerator
	{
		CDirEnumerator( const CCmdLineOptions& appOptions );

		// IEnumerator interface
		virtual void AddFoundFile( const fs::CPath& filePath );
		virtual bool AddFoundSubDir( const fs::TDirPath& subDirPath );

		void OnCompleted( void );
	private:
		const CCmdLineOptions& m_appOptions;
	public:
		size_t m_moreFilesCount;			// incremented when it reaches the limit
	};


	// CDirEnumerator implementation

	CDirEnumerator::CDirEnumerator( const CCmdLineOptions& appOptions )
		: fs::CPathEnumerator()
		, m_appOptions( appOptions )
		, m_moreFilesCount( 0 )
	{
		RefFlags().Set( fs::EF_IgnoreFiles, !m_appOptions.HasOptionFlag( app::DisplayFiles ) );
		RefFlags().Set( fs::EF_IgnoreHiddenNodes, !m_appOptions.HasOptionFlag( app::ShowHiddenNodes ) );
		RefFlags().Set( fs::EF_NoSortSubDirs, m_appOptions.HasOptionFlag( app::NoSorting ) );
	}

	void CDirEnumerator::AddFoundFile( const fs::CPath& filePath )
	{
		if ( m_filePaths.size() < m_appOptions.m_maxDirFiles )
			m_filePaths.push_back( filePath );
		else
			++m_moreFilesCount;
	}

	bool CDirEnumerator::AddFoundSubDir( const fs::TDirPath& subDirPath )
	{
		m_subDirPaths.push_back( subDirPath );
		return true;
	}

	void CDirEnumerator::OnCompleted( void )
	{
		if ( m_appOptions.HasOptionFlag( app::DisplayFiles ) && !m_appOptions.HasOptionFlag( app::NoOutput ) )
		{
			if ( !m_appOptions.HasOptionFlag( app::NoSorting ) )
				fs::SortPaths( m_filePaths );

			if ( m_moreFilesCount != 0 )
				m_filePaths.push_back( str::Format( m_filePaths.empty() ? _T("(%d files)") : _T("(+ %d more files)"), m_moreFilesCount ) );
		}
	}
}


const TCHAR CDirectory::s_wildSpec[] = _T("*");
double CDirectory::s_totalElapsedEnum = 0.0;
fs::CPath CDirectory::s_nullPath;

CDirectory::CDirectory( const CCmdLineOptions& options )
	: m_options( options )
	, m_dirPath( m_options.m_dirPath )
	, m_pTableFolder( m_options.GetTable() != NULL ? m_options.GetTable()->GetRoot() : NULL )
	, m_depth( 0 )
{
	s_totalElapsedEnum = 0.0;
}

CDirectory::CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath )
	: m_options( pParent->m_options )
	, m_dirPath( subDirPath )
	, m_pTableFolder( NULL )
	, m_depth( pParent->m_depth + 1 )
{
}

CDirectory::CDirectory( const CDirectory* pParent, const CTextCell* pTableFolder )
	: m_options( pParent->m_options )
	, m_dirPath( s_nullPath )
	, m_pTableFolder( pTableFolder )
	, m_depth( pParent->m_depth + 1 )
{
}

void CDirectory::ListContents( std::wostream& os, const CTreeGuides& guideParts )
{
	const std::wstring rootNodePrefix;

	if ( m_options.HasOptionFlag( app::TableInputMode ) )
		ListTableFolder( os, guideParts, rootNodePrefix );
	else
		ListDir( os, guideParts, rootNodePrefix );
}

void CDirectory::ListDir( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix )
{
	app::CDirEnumerator found( m_options );
	CTimer enumTimer;

	fs::EnumFiles( &found, m_dirPath, s_wildSpec );
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

			if ( !m_options.HasOptionFlag( app::SkipFileGroupLine ) )
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
			subDirectory.ListDir( os, guideParts, parentNodePrefix + guideParts.GetSubDirRecursePrefix( subDirPos ) );
		}
	}
}

void CDirectory::ListTableFolder( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix )
{
	ASSERT_PTR( m_pTableFolder );

	std::vector< CTextCell* > subFolders;
	m_pTableFolder->QuerySubFolders( subFolders );

	if ( m_options.HasOptionFlag( app::DisplayFiles ) && !m_options.HasOptionFlag( app::NoOutput ) )
	{
		std::vector< CTextCell* > leafs;
		m_pTableFolder->QueryLeafs( leafs );

		if ( !leafs.empty() )
		{
			std::wstring fullPrefix = parentNodePrefix + guideParts.GetFilePrefix( !subFolders.empty() );

			for ( size_t pos = 0; pos != leafs.size(); ++pos )
				os
					<< fullPrefix
					<< leafs[ pos ]->GetName()
					<< std::endl;

			if ( !m_options.HasOptionFlag( app::SkipFileGroupLine ) )
				if ( !str::TrimRight( fullPrefix ).empty() )		// remove trailing spaces on empty line
					os << fullPrefix << std::endl;
		}
	}

	for ( CPagePos subDirPos( subFolders ); !subDirPos.AtEnd(); ++subDirPos )
	{
		const CTextCell* pSubFolder = subFolders[ subDirPos.m_pos ];

		if ( !m_options.HasOptionFlag( app::NoOutput ) )
			os
				<< parentNodePrefix << guideParts.GetTable_SubDirPrefix( m_depth, subDirPos )
				<< pSubFolder->GetName()
				<< std::endl;

		if ( m_depth + 1 < m_options.m_maxDepthLevel )
		{	// recurse
			CDirectory subDirectory( this, pSubFolder );
			subDirectory.ListTableFolder( os, guideParts, parentNodePrefix + guideParts.GetTable_SubDirRecursePrefix( m_depth, subDirPos ) );
		}

		if ( !m_options.HasOptionFlag( app::SkipFileGroupLine ) )
			if ( !m_options.HasOptionFlag( app::NoOutput ) && 0 == m_depth && !subDirPos.IsLast() )
				os << std::endl;		// print extra line as root cell separator
	}
}
