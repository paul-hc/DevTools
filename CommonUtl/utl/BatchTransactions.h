#ifndef BatchTransaction_h
#define BatchTransaction_h
#pragma once

#include <map>
#include <set>
#include "Path.h"
#include "utl/FileState.h"


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


	abstract class CBaseBatchOperation
	{
	protected:
		CBaseBatchOperation( IBatchTransactionCallback* pCallback ) : m_pCallback( pCallback ) {}
		~CBaseBatchOperation();

		static std::tstring ExtractMessage( const fs::CPath& path, CException* pExc );
	public:
		const fs::TPathSet& GetCommittedKeys( void ) const { ASSERT_PTR( this ); return m_committed; }

		bool HasErrors( void ) const { return !m_errorMap.empty(); }
		bool ContainsError( const fs::CPath& sourcePath ) const { return m_errorMap.find( sourcePath ) != m_errorMap.end(); }

		void Clear( void );
	protected:
		IBatchTransactionCallback* m_pCallback;
		fs::TPathSet m_committed;							// committed successfuly
		std::map< fs::CPath, std::tstring > m_errorMap;		// source-path -> error

		static const TCHAR s_fmtError[];
	};


	class CBatchRename : public CBaseBatchOperation
	{
	public:
		CBatchRename( const fs::TPathPairMap& renamePairs, IBatchTransactionCallback* pCallback );

		bool RenameFiles( void );
	private:
		bool RenameSrcToInterm( void );
		bool RenameIntermToDest( void );

		void MakeIntermPaths( void );
		void LogTransaction( void ) const;
	private:
		const fs::TPathPairMap& m_rRenamePairs;
		std::vector< fs::CPath > m_intermPaths;				// intermediate paths in the rename cycle SRC -> INTERM -> DEST
	};


	class CBatchTouch : public CBaseBatchOperation
	{
	public:
		CBatchTouch( const fs::TFileStatePairMap& touchMap, IBatchTransactionCallback* pCallback );

		bool TouchFiles( void );
	private:
		bool _TouchFiles( void );
		void LogTransaction( void ) const;
	private:
		const fs::TFileStatePairMap& m_rTouchMap;
	};
}


#endif // BatchTransaction_h
