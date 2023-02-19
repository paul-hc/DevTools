#ifndef AppTools_h
#define AppTools_h
#pragma once

#include "Path.h"


class CEnumTags;
class CLogger;
class CImageStore;
namespace utl { class CResourcePool; }


namespace app
{
	void TraceException( const std::exception& exc );
	void TraceException( const CException* pExc );


	enum MsgType { Error, Warning, Info };

	const CEnumTags& GetTags_MsgType( void );
	std::tstring FormatMsg( const std::tstring& message, app::MsgType msgType );


	interface IAppTools
	{
		virtual bool IsConsoleApp( void ) const = 0;
		virtual bool IsInteractive( void ) const = 0;
		virtual CLogger& GetLogger( void ) = 0;
		virtual utl::CResourcePool& GetSharedResources( void ) = 0;
		virtual CImageStore* GetSharedImageStore( void ) = 0;
		virtual bool LazyInitAppResources( void ) = 0;

		virtual bool BeepSignal( app::MsgType msgType = app::Info ) = 0;									// returns false for convenience
		virtual bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error ) = 0;		// returns false for convenience
		virtual int ReportException( const std::exception& exc ) = 0;
		virtual int ReportException( const CException* pExc ) = 0;
	};
}


// global singleton that provides basic input/output and shared resources (e.g. the logger) - concrete application class specializes for console or Windows UI.
//
abstract class CAppTools
	: public app::IAppTools
	, private utl::noncopyable
{
protected:
	CAppTools( void );
	~CAppTools();
public:
	static CAppTools* Instance( void ) { ASSERT_PTR( s_pAppTools ); return s_pAppTools; }

	const fs::CPath& GetModulePath( void ) const { return m_modulePath; }

	// main() result code:
	static int GetMainResultCode( void ) { return s_mainResultCode; }
	static void AddMainResultError( void ) { ++s_mainResultCode; }
protected:
	fs::CPath m_modulePath;
private:
	static CAppTools* s_pAppTools;
	static int s_mainResultCode;		// can be used as a return result of main()
};


namespace app
{
	inline bool IsInteractive( void ) { return CAppTools::Instance()->IsInteractive(); }
	inline const fs::CPath& GetModulePath( void ) { return CAppTools::Instance()->GetModulePath(); }
	inline CLogger* GetLogger( void ) { return &CAppTools::Instance()->GetLogger(); }
	inline utl::CResourcePool& GetSharedResources( void ) { return CAppTools::Instance()->GetSharedResources(); }
	inline CImageStore* GetSharedImageStore( void ) { return CAppTools::Instance()->GetSharedImageStore(); }

	inline bool BeepSignal( app::MsgType msgType = app::Info ) { return CAppTools::Instance()->BeepSignal( msgType ); }
	inline bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error ) { return CAppTools::Instance()->ReportError( message, msgType ); }
	inline int ReportException( const std::exception& exc ) { return CAppTools::Instance()->ReportException( exc ); }
	inline int ReportException( const CException* pExc ) { return CAppTools::Instance()->ReportException( pExc ); }
}


namespace app
{
	class CLazyInitAppResources		// use as a base class or data-member for object relying on lazy resource initialization (e.g. automation server classes, etc)
	{
	public:
		CLazyInitAppResources( void ) { CAppTools::Instance()->LazyInitAppResources(); }
	};
}


#endif // AppTools_h
