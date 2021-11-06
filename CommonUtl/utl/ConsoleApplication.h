#ifndef ConsoleApplication_h
#define ConsoleApplication_h
#pragma once

#include "AppTools.h"
#include "FileSystem_fwd.h"
#include "Console_fwd.h"
#include "StdOutput.h"
#include "Logger.h"
#include "ResourcePool.h"


class CConsoleApplication : public CAppTools
{
public:
	CConsoleApplication( io::TranslationMode translationMode );
	~CConsoleApplication();

	bool IsConsoleOutput( void ) const { return m_stdOutput.IsConsoleOutput(); }
	bool IsFileRedirectOutput( void ) const { return io::FileRedirectedOutput == m_stdOutput.GetOutputMode(); }

	io::OutputMode GetOutputMode( void ) const { return m_stdOutput.GetOutputMode(); }

	io::CStdOutput& GetStdOutput( void ) { return m_stdOutput; }

	// IAppTools interface
	virtual bool IsConsoleApp( void ) const;
	virtual bool IsInteractive( void ) const;
	virtual CLogger& GetLogger( void );
	virtual utl::CResourcePool& GetSharedResources( void );
	virtual bool BeepSignal( app::MsgType msgType = app::Info );
	virtual bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error );
	virtual int ReportException( const std::exception& exc );
	virtual int ReportException( const CException* pExc );

	static io::TranslationMode GetTranslationMode( void ) { return s_translationMode; }
private:
	static void SetTranslationMode( io::TranslationMode translationMode );
private:
	io::CStdOutput m_stdOutput;								// wrapper for fast output to console or redirected file
	utl::CResourcePool m_resourcePool;
	std::auto_ptr<CLogger> m_pLogger;

	static io::TranslationMode s_translationMode;		// global text mode of standard output/error streams
};


namespace io
{
	char InputUserKey( bool skipWhitespace = true );
	char PressAnyKey( const char* pMessage = "Press any key to continue..." );


	enum UserChoices { YesNo, YesNoAll };

	const CEnumTags& GetTags_UserChoices( void );


	class CUserQuery
	{
	public:
		CUserQuery( bool ask = true, UserChoices choices = YesNoAll, const TCHAR* pPromptPrefix = _T("") )
			: m_ask( ask ), m_choices( choices ), m_promptPrefix( pPromptPrefix ), m_response( m_ask ? Undefined : Yes ) {}

		void SetPromptPrefix( const std::tstring& promptPrefix ) { m_promptPrefix = promptPrefix; }

		bool MustAsk( void ) const { return m_ask; }
		bool Ask( const std::tstring& assetName );		// assetName refers to the actual resource being created (a directory name, etc)

		bool IsNo( void ) const { ASSERT( m_response != Undefined ); return No == m_response; }
		bool IsYes( void ) const { return !IsNo(); }
		bool IsYesAll( void ) const { return IsYes() && !MustAsk(); }
	private:
		enum Response { Undefined = -1, No, Yes };

		bool m_ask;
		UserChoices m_choices;
		std::tstring m_promptPrefix;

		Response m_response;
	};


	class CUserQueryCreateDirectory			// acquire directory prompt
		: public CUserQuery
	{
		using CUserQuery::Ask;				// hidden, call Acquire()
	public:
		CUserQueryCreateDirectory( const TCHAR promptPrefix[], bool ask = true, UserChoices choices = YesNoAll )
			: CUserQuery( ask, choices, promptPrefix ) {}

		fs::AcquireResult Acquire( const fs::CPath& directoryPath );

		bool IsDenied( const fs::CPath& directoryPath ) const;
	private:
		std::vector< fs::CPath > m_deniedDirPaths;		// directories denied by the user or with creation error: cached to skip further prompts
	};
}


namespace func
{
	struct PtrStreamInserter : std::unary_function< void*, void >
	{
		PtrStreamInserter( std::tostream& os, const TCHAR* pSuffix = _T("") ) : m_os( os ), m_pSuffix( pSuffix ) {}

		template< typename ObjectType >
		void operator()( ObjectType* pObject ) const
		{
			m_os << *pObject << m_pSuffix;
		}
	private:
		std::tostream& m_os;
		const TCHAR* m_pSuffix;
	};
}


#endif // ConsoleApplication_h
