
#include "stdafx.h"
#include "Observers.h"
#include "AppService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void IWndObserver::OutputTargetWnd( void )
{
	OnTargetWndChanged( app::GetTargetWnd() );
}
