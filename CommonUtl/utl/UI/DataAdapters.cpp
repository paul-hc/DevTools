
#include "pch.h"
#include "DataAdapters.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CPositivePercentageAdapter implementation

	const CPositivePercentageAdapter* CPositivePercentageAdapter::Instance( void )
	{
		static const CPositivePercentageAdapter s_adapter;
		return &s_adapter;
	}
}
