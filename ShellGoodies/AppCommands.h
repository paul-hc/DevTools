#ifndef AppCommands_h
#define AppCommands_h
#pragma once

#include "AppCommands_fwd.h"


class CLogger;
namespace app { enum MsgType; }
namespace ut { class CScopedCmdLogger; }


namespace cmd
{
	void PrefixMsgTypeLine( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType );
	void SuffixMsgType( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType );
	void FormatLogMessage( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType );		// for single-line core message

	// command formatting
	const std::tstring& FormatCmdTag( const utl::ICommand* pCmd, utl::Verbosity verbosity );
	const TCHAR* GetSeparator( utl::Verbosity verbosity );

	void AppendDetailCount( std::tstring* pOutput, utl::Verbosity verbosity, size_t count );
	bool AppendTimestamp( std::tstring* pOutput, utl::Verbosity verbosity, const CTime& timestamp );

	std::tstring FormatCmdLine( const utl::ICommand* pCmd, utl::Verbosity verbosity );
	void QueryCmdFields( std::vector< std::tstring >& rFields, const utl::ICommand* pCmd );
}


namespace cmd
{
	abstract class CBaseSerialCmd : public CObject
		, public CCommand
	{
		DECLARE_DYNAMIC( CBaseSerialCmd )

		friend class ut::CScopedCmdLogger;
	protected:
		CBaseSerialCmd( CommandType cmdType = CommandType() );
	public:
		cmd::CommandType GetCmdType( void ) const { return static_cast<cmd::CommandType>( GetTypeID() ); }

		// base overrides
		virtual void Serialize( CArchive& archive );

		static void LogOutput( const std::tstring& message );
	protected:
		std::tstring FormatLogMessage( const std::tstring& coreMessage, app::MsgType msgType, const TCHAR* pTitle = NULL ) const;

		virtual void RecordMessage( const std::tstring& coreMessage, app::MsgType msgType );

		bool ShowBalloon( const std::tstring& coreMessage, app::MsgType msgType, const TCHAR* pTitle = NULL ) const;
	private:
		static CLogger* s_pLogger;
	public:
		static IErrorObserver* s_pErrorObserver;
	};


	class CScopedErrorObserver
	{
	public:
		CScopedErrorObserver( IErrorObserver* pErrorObserver ) : m_pOldErrorObserver( CBaseSerialCmd::s_pErrorObserver ) { CBaseSerialCmd::s_pErrorObserver = pErrorObserver; }
		~CScopedErrorObserver() { CBaseSerialCmd::s_pErrorObserver = m_pOldErrorObserver; }
	private:
		IErrorObserver* m_pOldErrorObserver;
	};
}


#endif // AppCommands_h
