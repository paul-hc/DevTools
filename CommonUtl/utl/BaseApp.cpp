
#include "stdafx.h"
#include "BaseApp.h"
#include "MenuUtilities.h"
#include "RuntimeException.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//ATL::CTraceCategory traceThumbs( _T("UTL Thumbnails") );


namespace app
{
	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos )
	{
	#ifdef _DEBUG
		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, ui::DebugPopup );
		ui::TrackPopupMenu( contextMenu, pTargetWnd, screenPos, TPM_RIGHTBUTTON );
	#else
		pTargetWnd;
		screenPos;
	#endif
	}

	void TraceException( const std::exception& exc )
	{
		exc;
		TRACE( _T("* STL exception: %s\n"), CRuntimeException::MessageOf( exc ).c_str() );
	}

	void TraceException( const CException& exc )
	{
		exc;
		TRACE( _T("* MFC exception: %s\n"), mfc::CRuntimeException::MessageOf( exc ).c_str() );
	}
}
