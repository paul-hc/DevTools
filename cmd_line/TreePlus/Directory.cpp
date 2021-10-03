
#include "stdafx.h"
#include "Directory.h"
#include "CmdLineOptions.h"
#include "OutputProfile.h"
#include <iostream>
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CDirectory::s_wildSpec[] = _T("*.*");

CDirectory::CDirectory( const fs::CPath& dirPath )
	: m_dirPath( dirPath )
	, m_level( 0 )
{
}

CDirectory::CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath )
	: m_dirPath( subDirPath )
	, m_level( pParent->m_level + 1 )
{
}

void CDirectory::List( std::wostream& os, const CCmdLineOptions& options, const CGuideParts& guideParts, const std::wstring& parentNodePrefix )
{
	if ( 0 == m_level )
		os << m_dirPath.GetPtr() << std::endl;			// print root directory

	fs::CEnumerator enumerator;
	fs::EnumFiles( &enumerator, m_dirPath, s_wildSpec, Shallow, false );

	if ( !HasFlag( options.m_optionFlags, app::NoSorting ) )
		fs::SortPaths( enumerator.m_subDirPaths );

	if ( HasFlag( options.m_optionFlags, app::DisplayFiles ) && !HasFlag( options.m_optionFlags, app::NoOutput ) )
		if ( !enumerator.m_filePaths.empty() )
		{
			if ( !HasFlag( options.m_optionFlags, app::NoSorting ) )
				fs::SortPaths( enumerator.m_filePaths );

			std::tstring fileFullPrefix = parentNodePrefix + guideParts.GetFilePrefix( !enumerator.m_subDirPaths.empty() );

			for ( CPagePos filePos( enumerator.m_filePaths ); !filePos.AtEnd(); ++filePos )
				os
					<< fileFullPrefix
					<< enumerator.m_filePaths[ filePos.m_pos ].GetFilenamePtr()
					<< std::endl;

			os << fileFullPrefix << std::endl;
		}

	size_t deepLevel = m_level + 1;

	for ( CPagePos subDirPos( enumerator.m_subDirPaths ); !subDirPos.AtEnd(); ++subDirPos )
	{
		const fs::CPath& subDirPath = enumerator.m_subDirPaths[ subDirPos.m_pos ];

		if ( !HasFlag( options.m_optionFlags, app::NoOutput ) )
			os
				<< parentNodePrefix << guideParts.GetSubDirPrefix( subDirPos )
				<< subDirPath.GetFilenamePtr()
				<< std::endl;

		if ( deepLevel <= options.m_maxDepthLevel )
		{
			CDirectory subDirectory( this, subDirPath );
			subDirectory.List( os, options, guideParts, parentNodePrefix + guideParts.GetSubDirRecursePrefix( subDirPos ) );
		}
	}
}
