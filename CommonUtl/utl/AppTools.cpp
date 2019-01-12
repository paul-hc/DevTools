
#include "stdafx.h"
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
		static const CEnumTags s_tags( _T("* Error|! Warning|- Info") );
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
		TRACE( _T("* STL exception: %s\n"), CRuntimeException::MessageOf( exc ).c_str() );
	}

	void TraceException( const CException* pExc )
	{
		pExc;
		ASSERT_PTR( pExc );
		TRACE( _T("* MFC exception: %s\n"), mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
	}
}


// CAppTools implementation

CAppTools* CAppTools::s_pAppTools = NULL;

CAppTools::CAppTools( void )
{
	ASSERT_NULL( s_pAppTools );
	s_pAppTools = this;
}

CAppTools::~CAppTools()
{
	ASSERT( this == s_pAppTools );
	s_pAppTools = NULL;
}
