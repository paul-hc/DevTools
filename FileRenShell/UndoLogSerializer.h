#ifndef UndoLogSerializer_h
#define UndoLogSerializer_h
#pragma once

#include <deque>
#include <iosfwd>
#include "utl/Command.h"
#include "utl/StringRange_fwd.h"


namespace cmd { enum Command; }


class CUndoLogSerializer
{
public:
	CUndoLogSerializer( std::deque< utl::ICommand* >* pOutUndoStack );

	void Save( std::ostream& os ) const;
	void Load( std::istream& is );
private:
	utl::ICommand* LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange );
	utl::ICommand* LoadSubCmd( cmd::Command command, const str::TStringRange& textRange );
private:
	std::deque< utl::ICommand* >* m_pUndoStack;
	unsigned int m_parseLineNo;
};


#endif // UndoLogSerializer_h
