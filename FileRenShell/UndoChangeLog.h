#ifndef UndoChangeLog_h
#define UndoChangeLog_h
#pragma once

#include <list>
#include <map>
#include <iosfwd>
#include "utl/FileState.h"
#include "utl/Path.h"
#include "utl/StringRange_fwd.h"
#include "Application_fwd.h"


class CUndoChangeLog : private utl::noncopyable
{
	friend class CUndoChangeLogTests;
public:
	CUndoChangeLog( void ) {}

	bool Save( void ) const;
	bool Load( void );

	static const fs::CPath& GetFilePath( void );
public:
	template< typename BatchType >
	struct CBatch
	{
		CBatch( const CTime& timestamp = CTime::GetCurrentTime() ) : m_timestamp( timestamp ) {}

		bool operator==( const CBatch& right ) const { return m_timestamp == right.m_timestamp && m_batch == right.m_batch; }	// UT only
	public:
		CTime m_timestamp;
		BatchType m_batch;		// batch entries
	};

	template< typename BatchType >
	class CUndoStack
	{
	public:
		CUndoStack( void ) {}

		const std::list< CBatch< BatchType > >& Get( void ) const { return m_stack; }

		bool CanUndo( void ) const { return !m_stack.empty(); }

		BatchType* GetTop( void ) { return !m_stack.empty() ? &m_stack.back().m_batch : NULL; }
		BatchType& Push( const CTime& timestamp = CTime::GetCurrentTime() );
		void Pop( void ) { ASSERT( CanUndo() ); m_stack.pop_back(); }

		void Clear( void ) { m_stack.clear(); }
	private:
		std::list< CBatch< BatchType > > m_stack;

		enum { MaxUndoSize = 20 };
	};

	CUndoStack< fs::TPathPairMap >& GetRenameUndoStack( void ) { return m_renameUndoStack; }
	CUndoStack< fs::TFileStatePairMap >& GetTouchUndoStack( void ) { return m_touchUndoStack; }
private:
	void Clear( void );

	void Save( std::ostream& os ) const;
	void Load( std::istream& is );

	void SaveRenameBatches( std::ostream& os ) const;
	void SaveTouchBatches( std::ostream& os ) const;

	static bool AddRenameLine( fs::TPathPairMap& rBatchRename, const str::TStringRange& textRange );
	static bool AddTouchLine( fs::TFileStatePairMap& rBatchTouch, const str::TStringRange& textRange );

	static std::tstring FormatActionTag( app::Action action, const CTime& timestamp );
	static bool ParseActionTag( app::Action& rAction, CTime& rTimestamp, const str::TStringRange& tagRange );
private:
	CUndoStack< fs::TPathPairMap > m_renameUndoStack;
	CUndoStack< fs::TFileStatePairMap > m_touchUndoStack;
};


// template code

template< typename BatchType >
BatchType& CUndoChangeLog::CUndoStack< BatchType >::Push( const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
{
	m_stack.push_back( CBatch< BatchType >( timestamp ) );

	if ( m_stack.size() > MaxUndoSize )
		m_stack.pop_front();				// remove expired
	ENSURE( m_stack.size() <= MaxUndoSize );

	return m_stack.back().m_batch;
}


#endif // UndoChangeLog_h
