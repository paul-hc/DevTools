
#include "stdafx.h"
#include "BatchTransactions.h"
#include "Logger.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	CBatchRename::CBatchRename( IBatchTransactionCallback* pCallback )
		: m_pCallback( pCallback )
	{
		ASSERT_PTR( m_pCallback );
	}

	CBatchRename::~CBatchRename()
	{
	}

	bool CBatchRename::Rename( const fs::PathPairMap& renamePairs )
	{
		CWaitCursor wait;

		MakeIntermPaths( renamePairs );
		m_renamed.clear();
		m_errorMap.clear();

		// rename in 2 steps to avoid colissions of new paths within the range of original paths (overlapping reallocation)
		// SRC -> INTERM -> DEST
		bool aborted = !RenameSrcToInterm( renamePairs ) || !RenameIntermToDest( renamePairs );
		LogTransaction( renamePairs );
		return !aborted;
	}

	bool CBatchRename::RenameSrcToInterm( const fs::PathPairMap& renamePairs )
	{
		// step 1: rename SOURCE -> INTERMEDIATE
		std::vector< fs::CPath >::const_iterator itInterm = m_intermPaths.begin();
		for ( fs::PathPairMap::const_iterator it = renamePairs.begin(); it != renamePairs.end(); )
		{
			try
			{
				CFile::Rename( it->first.GetPtr(), itInterm->GetPtr() );
				m_renamed.insert( it->first );				// mark as done
			}
			catch ( CException* pExc )
			{
				std::tstring message = ExtractMessage( it->first, pExc );
				m_errorMap[ it->first ] = message;			// mark as error
				switch ( m_pCallback->HandleFileError( it->first, message ) )
				{
					case Abort:	 return false;
					case Retry:	 continue;
					case Ignore: break;
				}
			}

			++it; ++itInterm;
		}
		return true;
	}

	bool CBatchRename::RenameIntermToDest( const fs::PathPairMap& renamePairs )
	{
		// step 2: rename INTERMEDIATE -> DESTINATION
		std::vector< fs::CPath >::const_iterator itInterm = m_intermPaths.begin();
		for ( fs::PathPairMap::const_iterator it = renamePairs.begin(); it != renamePairs.end(); )
		{
			if ( m_renamed.find( it->first ) != m_renamed.end() )
			{
				try
				{
					CFile::Rename( itInterm->GetPtr(), it->second.GetPtr() );
				}
				catch ( CException* pExc )
				{
					m_renamed.erase( it->first );		// exclude erroneous from done

					std::tstring message = ExtractMessage( *itInterm, pExc );
					m_errorMap[ it->first ] = message;
					switch ( m_pCallback->HandleFileError( it->first, message ) )
					{
						case Abort:	 return false;
						case Retry:	 continue;
						case Ignore: break;
					}
				}
			}

			++it; ++itInterm;
		}
		return true;
	}

	void CBatchRename::MakeIntermPaths( const fs::PathPairMap& renamePairs )
	{
		const std::tstring intermSuffix = str::Format( _T("_%x"), GetTickCount() );		// random fname sufffix

		m_intermPaths.clear();
		for ( fs::PathPairMap::const_iterator it = renamePairs.begin(); it != renamePairs.end(); ++it )
		{
			fs::CPathParts parts( it->second.Get() );
			parts.m_fname += intermSuffix;
			m_intermPaths.push_back( parts.MakePath() );
		}

		ENSURE( m_intermPaths.size() == renamePairs.size() );
	}

	std::tstring CBatchRename::ExtractMessage( const fs::CPath& path, CException* pExc )
	{
		std::tstring message = ui::GetErrorMessage( pExc );
		pExc->Delete();
		if ( !path.FileExist() )
			message = str::Format( _T("Cannot find file:\n%s"), path.GetPtr() );
		return message;
	}

	void CBatchRename::LogTransaction( const fs::PathPairMap& renamePairs ) const
	{
		if ( CLogger* pLogger = m_pCallback->GetLogger() )
			for ( fs::PathPairMap::const_iterator it = renamePairs.begin(); it != renamePairs.end(); ++it )
			{
				std::map< fs::CPath, std::tstring >::const_iterator itError = m_errorMap.find( it->first );
				if ( itError != m_errorMap.end() )
					pLogger->Log( _T("%s -> ERROR: %s\n* %s"), it->first.Get().c_str(), it->second.GetNameExt(), itError->second.c_str() );
				else if ( m_renamed.find( it->first ) != m_renamed.end() )
					pLogger->Log( _T("%s -> %s"), it->first.Get().c_str(), it->second.GetNameExt() );
				// else: not processed because transaction was aborted on error
			}
	}
}
