
#include "stdafx.h"
#include "BatchTransactions.h"
#include "FmtUtils.h"
#include "Logger.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CBaseBatchOperation implementation

	const TCHAR CBaseBatchOperation::s_fmtError[] = _T("* %s\n ERROR: %s");

	CBaseBatchOperation::~CBaseBatchOperation()
	{
	}

	void CBaseBatchOperation::Clear( void )
	{
		m_committed.clear();
		m_errorMap.clear();
	}

	std::tstring CBaseBatchOperation::ExtractMessage( const fs::CPath& path, CException* pExc )
	{
		std::tstring message = ui::GetErrorMessage( pExc );
		pExc->Delete();

		if ( !path.FileExist() )
			return str::Format( _T("Cannot find file:\n%s\n%s"), path.GetPtr(), message.c_str() );
		return message;
	}


	// CBatchRename implementation

	CBatchRename::CBatchRename( const fs::TPathPairMap& renamePairs, IBatchTransactionCallback* pCallback )
		: CBaseBatchOperation( pCallback )
		, m_rRenamePairs( renamePairs )
	{
	}

	bool CBatchRename::RenameFiles( void )
	{
		CWaitCursor wait;

		Clear();
		MakeIntermPaths();

		// rename in 2 steps to avoid colissions of new paths within the range of original paths (overlapping reallocation)
		// SRC -> INTERM -> DEST
		bool aborted = !RenameSrcToInterm() || !RenameIntermToDest();
		LogTransaction();
		return !aborted;
	}

	bool CBatchRename::RenameSrcToInterm( void )
	{
		// step 1: rename SOURCE -> INTERMEDIATE
		std::vector< fs::CPath >::const_iterator itInterm = m_intermPaths.begin();
		for ( fs::TPathPairMap::const_iterator it = m_rRenamePairs.begin(); it != m_rRenamePairs.end(); )
		{
			try
			{
				CFile::Rename( it->first.GetPtr(), itInterm->GetPtr() );
				m_committed.insert( it->first );				// mark as done
			}
			catch ( CException* pExc )
			{
				std::tstring message = ExtractMessage( it->first, pExc );
				m_errorMap[ it->first ] = message;			// mark as error

				if ( m_pCallback != NULL )
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

	bool CBatchRename::RenameIntermToDest( void )
	{
		// step 2: rename INTERMEDIATE -> DESTINATION
		std::vector< fs::CPath >::const_iterator itInterm = m_intermPaths.begin();
		for ( fs::TPathPairMap::const_iterator it = m_rRenamePairs.begin(); it != m_rRenamePairs.end(); )
		{
			if ( m_committed.find( it->first ) != m_committed.end() )
			{
				try
				{
					CFile::Rename( itInterm->GetPtr(), it->second.GetPtr() );
				}
				catch ( CException* pExc )
				{
					m_committed.erase( it->first );		// exclude erroneous from done

					std::tstring message = ExtractMessage( *itInterm, pExc );
					m_errorMap[ it->first ] = message;

					if ( m_pCallback != NULL )
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

	void CBatchRename::MakeIntermPaths( void )
	{
		const std::tstring intermSuffix = str::Format( _T("_(%x)"), ::GetTickCount() );		// random fname sufffix

		m_intermPaths.clear();
		for ( fs::TPathPairMap::const_iterator it = m_rRenamePairs.begin(); it != m_rRenamePairs.end(); ++it )
		{
			fs::CPathParts parts( it->second.Get() );
			parts.m_fname += intermSuffix;
			m_intermPaths.push_back( parts.MakePath() );
		}

		ENSURE( m_intermPaths.size() == m_rRenamePairs.size() );
	}

	void CBatchRename::LogTransaction( void ) const
	{
		CLogger* pLogger = m_pCallback != NULL ? m_pCallback->GetLogger() : NULL;
		if ( NULL == pLogger )
			return;

		for ( fs::TPathPairMap::const_iterator itPair = m_rRenamePairs.begin(); itPair != m_rRenamePairs.end(); ++itPair )
		{
			std::tstring entryText = fmt::FormatRenameEntry( itPair->first, itPair->second );

			std::map< fs::CPath, std::tstring >::const_iterator itError = m_errorMap.find( itPair->first );
			if ( itError != m_errorMap.end() )
				pLogger->Log( s_fmtError, entryText.c_str(), itError->second.c_str() );
			else if ( m_committed.find( itPair->first ) != m_committed.end() )
				pLogger->Log( entryText.c_str() );
			else
			{	// ignore not processed because transaction was aborted on error
			}
		}
	}


	// CBatchTouch implementation

	CBatchTouch::CBatchTouch( const fs::TFileStatePairMap& touchMap, IBatchTransactionCallback* pCallback )
		: CBaseBatchOperation( pCallback )
		, m_rTouchMap( touchMap )
	{
	}

	bool CBatchTouch::TouchFiles( void )
	{
		CWaitCursor wait;

		Clear();

		bool aborted = !_TouchFiles();
		LogTransaction();
		return !aborted;
	}

	bool CBatchTouch::_TouchFiles( void )
	{
		for ( fs::TFileStatePairMap::const_iterator itStatePair = m_rTouchMap.begin(); itStatePair != m_rTouchMap.end(); )
		{
			try
			{
				if ( itStatePair->second != itStatePair->first )
				{
					itStatePair->second.WriteToFile();
					m_committed.insert( itStatePair->first.m_fullPath );		// mark as done
				}
			}
			catch ( CException* pExc )
			{
				std::tstring message = ExtractMessage( itStatePair->second.m_fullPath, pExc );
				m_errorMap[ itStatePair->first.m_fullPath ] = message;	// mark as error

				if ( m_pCallback != NULL )
					switch ( m_pCallback->HandleFileError( itStatePair->first.m_fullPath, message ) )
					{
						case Abort:	 return false;
						case Retry:	 continue;
						case Ignore: break;
					}
			}

			++itStatePair;			// advance to next file if no Retry
		}
		return true;
	}

	void CBatchTouch::LogTransaction( void ) const
	{
		CLogger* pLogger = m_pCallback != NULL ? m_pCallback->GetLogger() : NULL;
		if ( NULL == pLogger )
			return;

		for ( fs::TFileStatePairMap::const_iterator itStatePair = m_rTouchMap.begin(); itStatePair != m_rTouchMap.end(); ++itStatePair )
		{
			std::tstring entryText = fmt::FormatTouchEntry( itStatePair->first, itStatePair->second );

			std::map< fs::CPath, std::tstring >::const_iterator itError = m_errorMap.find( itStatePair->first.m_fullPath );
			if ( itError != m_errorMap.end() )
				pLogger->Log( s_fmtError, entryText.c_str(), itError->second.c_str() );
			else if ( m_committed.find( itStatePair->first.m_fullPath ) != m_committed.end() )
				pLogger->Log( entryText.c_str() );
			else
			{	// ignore not processed because transaction was aborted on error
			}
		}
	}
}
