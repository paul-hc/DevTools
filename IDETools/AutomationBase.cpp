
#include "stdafx.h"
#include "AutomationBase.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


CAutomationBase::CAutomationBase( void )
{
	CApplication::GetApp()->LazyInitAppResources();		// initialize once application resources since this is not a regsvr32.exe invocation
}
