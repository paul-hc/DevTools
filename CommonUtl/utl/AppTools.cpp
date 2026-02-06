
#include "pch.h"
#include "AppTools.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	const CEnumTags& GetTags_MsgType( void )
	{
		static const CEnumTags s_tags( _T("* Error|! Warning|- Info"), _T("* ERROR|! WARNING|") );
		return s_tags;
	}

	std::tstring FormatMsg( const std::tstring& message, app::MsgType msgType )
	{
		std::tstring text = GetTags_MsgType().FormatUi( msgType );
		stream::Tag( text, message, _T(": ") );
		return text;
	}


	void TraceException( const std::exception& exc )
	{
		exc;
		TRACE( _T("* STL exception (%s): %s\n"), str::GetTypeName( typeid( exc ) ).c_str(), CRuntimeException::MessageOf(exc).c_str());
	}

	void TraceException( const CException* pExc )
	{
		pExc;
		ASSERT_PTR( pExc );

		if ( !CAppTools::Instance()->IsConsoleApp() )		// avoid console applications, since usually they don't include MFC resource strings required to format the exception message
			TRACE( _T("* MFC exception (%s): %s\n"), str::GetTypeName( typeid( *pExc ) ).c_str(), mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
	}
}


// CAppTools implementation

int CAppTools::s_mainResultCode = 0;
CAppTools* CAppTools::s_pAppTools = nullptr;

CAppTools::CAppTools( void )
{
	ASSERT_NULL( s_pAppTools );
	s_pAppTools = this;
}

CAppTools::~CAppTools()
{
	ASSERT( this == s_pAppTools );
	s_pAppTools = nullptr;
}

bool CAppTools::IsRunningUnderWow64( void ) const implement
{	// WOW64 (Windows on Windows 64-bit) - is this a 32-bit process running on Windows 64-bit (using WOW64 redirection)?
	static BOOL s_isWow64 = FALSE;
#if defined(_WIN32) && !defined(_WIN64)
	static bool s_isInit = false;

	if ( !s_isInit )
	{	// IsWow64Process is available on Windows XP SP2 and later
		::IsWow64Process( ::GetCurrentProcess(), &s_isWow64 );
		s_isInit = true;
	}
#endif
	return s_isWow64 != FALSE;
}
