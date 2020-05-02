
#include "stdafx.h"
#include "IProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CNullProgressService implementation

	ui::IProgressService* CNullProgressService::Instance( void )
	{
		static CNullProgressService s_nullProgressPlaceholder;
		return &s_nullProgressPlaceholder;
	}


	// ui::IProgressHeader interface implementation

	void CNullProgressService::SetDialogTitle( const std::tstring& title )
	{
		title;
	}

	void CNullProgressService::SetOperationLabel( const std::tstring& operationLabel )
	{
		operationLabel;
	}

	void CNullProgressService::ShowStage( bool show /*= true*/ )
	{
		show;
	}

	void CNullProgressService::SetStageLabel( const std::tstring& stageLabel )
	{
		stageLabel;
	}

	void CNullProgressService::ShowItem( bool show /*= true*/ )
	{
		show;
	}

	void CNullProgressService::SetItemLabel( const std::tstring& itemLabel )
	{
		itemLabel;
	}


	// ui::IProgressService interface implementation

	ui::IProgressHeader* CNullProgressService::GetHeader( void )
	{
		return this;
	}

	bool CNullProgressService::SetMarqueeProgress( void )
	{
		return false;
	}

	void CNullProgressService::SetBoundedProgressCount( size_t itemCount, bool rewindPos /*= true*/ )
	{
		itemCount, rewindPos;
	}

	void CNullProgressService::SetProgressState( int barState /*= PBST_NORMAL*/ )
	{
		barState;
	}

	void CNullProgressService::AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException )
	{
		stageName;
	}

	void CNullProgressService::AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException )
	{
		itemName;
	}

	void CNullProgressService::AdvanceItemToEnd( void ) throws_( CUserAbortedException )
	{
	}

	void CNullProgressService::ProcessInput( void ) const throws_( CUserAbortedException )
	{
	}
}
