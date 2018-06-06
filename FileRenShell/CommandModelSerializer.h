#ifndef CommandModelSerializer_h
#define CommandModelSerializer_h
#pragma once

#include <deque>
#include <iosfwd>
#include "utl/StringRange_fwd.h"
#include "FileCommands_fwd.h"


class CCommandModel;


class CCommandModelSerializer
{
public:
	CCommandModelSerializer( void );

	void Save( std::ostream& os, const CCommandModel& commandModel ) const;
	void Load( std::istream& is, CCommandModel* pOutCommandModel );
private:
	void SaveStack( std::ostream& os, cmd::UndoRedo section, const std::deque< utl::ICommand* >& cmdStack ) const;

	utl::ICommand* LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange );
	utl::ICommand* LoadSubCmd( cmd::CommandType cmdType, const str::TStringRange& textRange );

	static const CEnumTags& GetTags_Section( void );
	static std::tstring FormatSectionTag( const TCHAR tag[] );
	static bool ParseSectionTag( str::TStringRange& rTextRange );
	static std::tstring FormatTag( const TCHAR tag[] );
	static bool ParseTag( str::TStringRange& rTextRange );
	static bool ParseCommandTag( cmd::CommandType& rCmdType, CTime& rTimestamp, const str::TStringRange& tagRange );
private:
	unsigned int m_parseLineNo;

	static const TCHAR s_sectionTagSeps[];
	static const TCHAR s_tagSeps[];
	static const TCHAR s_tagEndOfBatch[];
};


#endif // CommandModelSerializer_h
