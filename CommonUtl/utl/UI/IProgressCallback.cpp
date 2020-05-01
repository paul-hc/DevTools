
#include "stdafx.h"
#include "IProgressCallback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CNullProgress implementation

	ui::IProgressCallback* CNullProgress::Instance( void )
	{
		static CNullProgress s_nullProgressPlaceholder;
		return &s_nullProgressPlaceholder;
	}

	void CNullProgress::SetProgressRange( int lower, int upper, bool rewindPos /*= false*/ )
	{
		lower, upper, rewindPos;
	}

	bool CNullProgress::SetMarqueeProgress( bool marquee /*= true*/ )
	{
		marquee;
		return true;
	}

	void CNullProgress::SetProgressState( int barState /*= PBST_NORMAL*/ )
	{
		barState;
	}

	void CNullProgress::AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException )
	{
		stageName;
	}

	void CNullProgress::AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException )
	{
		itemName;
	}

	void CNullProgress::AdvanceItemToEnd( void ) throws_( CUserAbortedException )
	{
	}

	void CNullProgress::ProcessInput( void ) const throws_( CUserAbortedException )
	{
	}
}
