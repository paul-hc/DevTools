#ifndef CommandModelPersist_h
#define CommandModelPersist_h
#pragma once

#include <deque>
#include <iosfwd>
#include "utl/ICommand.h"
#include "AppCommands_fwd.h"


class CCommandModel;
namespace cmd { class CLogSerializer; }


class CCommandModelPersist
{
public:
	static bool SaveUndoLog( const CCommandModel& commandModel, cmd::FileFormat fileFormat );
	static bool LoadUndoLog( CCommandModel* pOutCommandModel, cmd::FileFormat* pOutFileFormat = NULL );

	static fs::CPath GetUndoLogPath( cmd::FileFormat fileFormat );
	static bool FindSavedUndoLogPath( cmd::FileFormat& rFileFormat );
private:
	static cmd::CLogSerializer* CreateSerializer( CCommandModel* pCommandModel, cmd::FileFormat fileFormat );
};


#include "utl/Serialization.h"
#include "utl/StringRange_fwd.h"


namespace cmd
{
	abstract class CLogSerializer
	{
	protected:
		CLogSerializer( CCommandModel* pCommandModel ) : m_pCommandModel( pCommandModel ) { ASSERT_PTR( m_pCommandModel ); }
	public:
		virtual ~CLogSerializer() {}

		virtual bool Save( const fs::CPath& undoLogPath ) = 0;
		virtual bool Load( const fs::CPath& undoLogPath ) = 0;
	protected:
		static const CEnumTags& GetTags_Section( void );
		static std::tstring FormatSectionTag( const TCHAR tag[] );
		static bool ParseSectionTag( str::TStringRange& rTextRange );
	protected:
		CCommandModel* m_pCommandModel;

		static const TCHAR s_sectionTagSeps[];
	};


	class CTextLogSerializer : public CLogSerializer
	{
	public:
		CTextLogSerializer( CCommandModel* pCommandModel ) : CLogSerializer( pCommandModel ), m_parseLineNo( 0 ) {}

		// base overrides
		virtual bool Save( const fs::CPath& undoLogPath ) override;
		virtual bool Load( const fs::CPath& undoLogPath ) override;

		void Save( std::ostream& os ) const;
		void Load( std::istream& is );
	private:
		void SaveStack( std::ostream& os, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack ) const;

		utl::ICommand* LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange );
		utl::ICommand* LoadSubCmd( cmd::CommandType cmdType, const str::TStringRange& textRange );

		static std::tstring FormatTag( const TCHAR tag[] );
		static bool ParseTag( str::TStringRange& rTextRange );
		static bool ParseCommandTag( cmd::CommandType& rCmdType, CTime& rTimestamp, const str::TStringRange& tagRange );
	private:
		unsigned int m_parseLineNo;

		static const TCHAR s_tagSeps[];
		static const TCHAR s_tagEndOfBatch[];
	};


	class CBinaryLogSerializer : public CLogSerializer
							   , private serial::IStreamable
	{
	public:
		CBinaryLogSerializer( CCommandModel* pCommandModel ) : CLogSerializer( pCommandModel ) {}

		// base overrides
		virtual bool Save( const fs::CPath& undoLogPath ) override;
		virtual bool Load( const fs::CPath& undoLogPath ) override;
	private:
		// serial::IStreamable interface
		virtual void Save( CArchive& archive ) throws_( CException* );
		virtual void Load( CArchive& archive ) throws_( CException* );

		static void SaveStack( CArchive& archive, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack );
		static void LoadStack( CArchive& archive, std::deque< utl::ICommand* >& rCmdStack );
	};
}


#endif // CommandModelPersist_h
