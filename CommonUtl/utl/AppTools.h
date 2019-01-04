#ifndef AppTools_h
#define AppTools_h
#pragma once


class CEnumTags;
class CLogger;
namespace fs { class CPath; }
namespace utl { class CResourcePool; }


namespace app
{
	fs::CPath GetModuleFilePath( HINSTANCE hInstance = NULL );		// AfxGetApp()->m_hInstance


	void TraceException( const std::exception& exc );
	void TraceException( const CException* pExc );


	enum MsgType { Error, Warning, Info };

	const CEnumTags& GetTags_MsgType( void );
	std::tstring FormatMsg( const std::tstring& message, app::MsgType msgType );


	interface IAppTools
	{
		virtual CLogger& GetLogger( void ) = 0;
		virtual utl::CResourcePool& GetSharedResources( void ) = 0;

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
{
protected:
	CAppTools( void );
	~CAppTools();
public:
	static CAppTools* Instance( void ) { ASSERT_PTR( s_pAppTools ); return s_pAppTools; }
private:
	static CAppTools* s_pAppTools;
};


namespace app
{
	inline bool BeepSignal( app::MsgType msgType = app::Info ) { return CAppTools::Instance()->BeepSignal( msgType ); }
	inline bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error ) { return CAppTools::Instance()->ReportError( message, msgType ); }
	inline int ReportException( const std::exception& exc ) { return CAppTools::Instance()->ReportException( exc ); }
	inline int ReportException( const CException* pExc ) { return CAppTools::Instance()->ReportException( pExc ); }

	inline CLogger* GetLogger( void ) { return &CAppTools::Instance()->GetLogger(); }
	inline utl::CResourcePool& GetSharedResources( void ) { return CAppTools::Instance()->GetSharedResources(); }
}


#endif // AppTools_h
