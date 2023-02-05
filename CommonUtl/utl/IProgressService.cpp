
#include "pch.h"
#include "IProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace svc
{
	// CNoProgressService implementation

	utl::IProgressService* CNoProgressService::Instance( void )
	{
		static CNoProgressService s_nullProgressPlaceholder;
		return &s_nullProgressPlaceholder;
	}


	// utl::IProgressHeader interface implementation

	void CNoProgressService::SetDialogTitle( const std::tstring& title )
	{
		title;
	}

	void CNoProgressService::SetOperationLabel( const std::tstring& operationLabel )
	{
		operationLabel;
	}

	void CNoProgressService::ShowStage( bool show /*= true*/ )
	{
		show;
	}

	void CNoProgressService::SetStageLabel( const std::tstring& stageLabel )
	{
		stageLabel;
	}

	void CNoProgressService::ShowItem( bool show /*= true*/ )
	{
		show;
	}

	void CNoProgressService::SetItemLabel( const std::tstring& itemLabel )
	{
		itemLabel;
	}


	// svc::IProgressService interface implementation

	utl::IProgressHeader* CNoProgressService::GetHeader( void )
	{
		return this;
	}

	bool CNoProgressService::SetMarqueeProgress( void )
	{
		return false;
	}

	void CNoProgressService::SetBoundedProgressCount( size_t itemCount, bool rewindPos /*= true*/ )
	{
		itemCount, rewindPos;
	}

	void CNoProgressService::SetProgressStep( int step )
	{
		step;
	}

	void CNoProgressService::SetProgressState( int barState /*= PBST_NORMAL*/ )
	{
		barState;
	}

	void CNoProgressService::AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException )
	{
		stageName;
	}

	void CNoProgressService::AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException )
	{
		itemName;
	}

	void CNoProgressService::AdvanceItemToEnd( void ) throws_( CUserAbortedException )
	{
	}

	void CNoProgressService::ProcessInput( void ) const throws_( CUserAbortedException )
	{
	}
}
