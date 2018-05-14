#ifndef UndoChangeLog_h
#define UndoChangeLog_h
#pragma once

#include <list>
#include <map>
#include "utl/Path.h"
#include "utl/FileInfo.h"


class CEnumTags;


class CUndoChangeLog : private utl::noncopyable
{
public:
	CUndoChangeLog( void ) {}

	// RENAME undo stack (top at the back)
	bool CanUndoRename( void ) const { return !m_renameUndoStack.empty(); }
	fs::PathPairMap* GetTopRename( void ) { return !m_renameUndoStack.empty() ? &m_renameUndoStack.back().m_batch : NULL; }
	fs::PathPairMap& PushRename( void ) { PushBatch( m_renameUndoStack ); return m_renameUndoStack.back().m_batch; }
	void PopRename( void ) { ASSERT( CanUndoRename() ); m_renameUndoStack.pop_back(); }

	// TOUCH undo stack (top at the back)
	bool CanUndoTouch( void ) const { return !m_touchUndoStack.empty(); }
	fs::TFileInfoSet* GetTopTouch( void ) { return !m_touchUndoStack.empty() ? &m_touchUndoStack.back().m_batch : NULL; }
	fs::TFileInfoSet& PushTouch( void ) { PushBatch( m_touchUndoStack ); return m_touchUndoStack.back().m_batch; }
	void PopTouch( void ) { ASSERT( CanUndoTouch() ); m_touchUndoStack.pop_back(); }

	bool Save( void ) const;
	bool Load( void );

	static const std::tstring& GetFilePath( void );
private:
	enum { MaxUndoSize = 20 };
	enum Action { Rename, Touch };

	static const CEnumTags& GetTags_Action( void );
	static bool ParseActionTag( Action& rAction, CTime& rTimestamp, const std::string& text );
	static std::tstring FormatActionTag( Action action, const CTime& timestamp );

	static bool ParseRenameLine( fs::PathPairMap& rBatchRename, const std::tstring& line );
	static bool ParseTouchLine( fs::TFileInfoSet& rBatchTouch, const std::tstring& line );

	void SaveRenameBatches( std::ostream& output ) const;
	void SaveTouchBatches( std::ostream& output ) const;


	template< typename BatchType >
	struct CBatch
	{
		CBatch( const CTime& timestamp = CTime::GetCurrentTime() ) : m_timestamp( timestamp ) {}
	public:
		CTime m_timestamp;
		BatchType m_batch;		// batch entries
	};


	template< typename BatchType >
	static void PushBatch( std::list< CBatch< BatchType > >& rUndoStack );
private:
	std::list< CBatch< fs::PathPairMap > > m_renameUndoStack;
	std::list< CBatch< fs::TFileInfoSet > > m_touchUndoStack;
};


template< typename BatchType >
void CUndoChangeLog::PushBatch( std::list< CBatch< BatchType > >& rUndoStack )
{
	rUndoStack.push_back( CBatch< BatchType >() );
	if ( rUndoStack.size() > MaxUndoSize )
		rUndoStack.pop_front();
	ENSURE( rUndoStack.size() <= MaxUndoSize );
}


#endif // UndoChangeLog_h
