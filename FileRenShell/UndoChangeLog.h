#ifndef UndoChangeLog_h
#define UndoChangeLog_h
#pragma once

#include <list>
#include <map>
#include "utl/Path.h"


class CUndoChangeLog
{
public:
	CUndoChangeLog( void );
	~CUndoChangeLog();

	static const std::tstring& GetFilePath( void );

	static bool Save( const std::list< fs::PathPairMap >& undoStack );
	static bool Load( std::list< fs::PathPairMap >& rUndoStack );
};


#endif // UndoChangeLog_h
