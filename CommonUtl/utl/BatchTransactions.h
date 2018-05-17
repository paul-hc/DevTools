#ifndef BatchTransaction_h
#define BatchTransaction_h
#pragma once

#include <map>
#include <set>
#include "Path.h"


class CLogger;


namespace fs
{
	enum UserFeedback { Abort, Retry, Ignore };


	interface IBatchTransactionCallback
	{
		virtual CWnd* GetWnd( void ) = 0;
		virtual CLogger* GetLogger( void ) = 0;
		virtual UserFeedback HandleFileError( const fs::CPath& sourcePath, const std::tstring& message ) = 0;
	};


	class CBatchRename
	{
	public:
		CBatchRename( IBatchTransactionCallback* pCallback );
		~CBatchRename();

		const fs::TPathSet& GetRenamedKeys( void ) const { ASSERT_PTR( this ); return m_renamed; }

		bool HasErrors( void ) const { return !m_errorMap.empty(); }
		bool ContainsError( const fs::CPath& sourcePath ) const { return m_errorMap.find( sourcePath ) != m_errorMap.end(); }

		bool Rename( const fs::TPathPairMap& renamePairs );
	private:
		bool RenameSrcToInterm( const fs::TPathPairMap& renamePairs );
		bool RenameIntermToDest( const fs::TPathPairMap& renamePairs );

		void MakeIntermPaths( const fs::TPathPairMap& renamePairs );
		void LogTransaction( const fs::TPathPairMap& renamePairs ) const;
		static std::tstring ExtractMessage( const fs::CPath& path, CException* pExc );
	private:
		IBatchTransactionCallback* m_pCallback;
		std::vector< fs::CPath > m_intermPaths;				// intermediate paths in the rename cycle SRC -> INTERM -> DEST
		fs::TPathSet m_renamed;								// renamed successfuly
		std::map< fs::CPath, std::tstring > m_errorMap;		// source-path -> error
	};
}


#endif // BatchTransaction_h
