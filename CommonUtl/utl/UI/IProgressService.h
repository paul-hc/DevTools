#ifndef IProgressService_h
#define IProgressService_h
#pragma once


class CUserAbortedException;


namespace ui
{
	interface IProgressHeader
	{
		virtual void SetDialogTitle( const std::tstring& title ) = 0;
		virtual void SetOperationLabel( const std::tstring& operationLabel ) = 0;

		virtual void ShowStage( bool show = true ) = 0;
		virtual void SetStageLabel( const std::tstring& stageLabel ) = 0;

		virtual void ShowItem( bool show = true ) = 0;
		virtual void SetItemLabel( const std::tstring& itemLabel ) = 0;
	};

	interface IProgressService
	{
		virtual ui::IProgressHeader* GetHeader( void ) = 0;

		// progress type
		virtual bool SetMarqueeProgress( void ) = 0;
		virtual void SetBoundedProgressCount( size_t itemCount, bool rewindPos = true ) = 0;

		// progress bar colour
		virtual void SetProgressState( int barState = PBST_NORMAL ) = 0;		// PBST_NORMAL/PBST_PAUSED/PBST_ERROR

		// advancing
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException ) = 0;
		virtual void ProcessInput( void ) const throws_( CUserAbortedException ) = 0;		// yield cooperatively
	};
}


namespace ui
{
	// Null Pattern: placeholder for a valid ui::IProgressService interface with empty implementation
	//
	class CNoProgressService : private ui::IProgressHeader
							 , public ui::IProgressService
	{
		CNoProgressService( void ) {}

		// ui::IProgressHeader interface
		virtual void SetDialogTitle( const std::tstring& title );
		virtual void SetOperationLabel( const std::tstring& operationLabel );
		virtual void ShowStage( bool show = true );
		virtual void SetStageLabel( const std::tstring& stageLabel );
		virtual void ShowItem( bool show = true );
		virtual void SetItemLabel( const std::tstring& itemLabel );

		// ui::IProgressService interface
		virtual ui::IProgressHeader* GetHeader( void );
		virtual bool SetMarqueeProgress( void );
		virtual void SetBoundedProgressCount( size_t itemCount, bool rewindPos = true );
		virtual void SetProgressState( int barState = PBST_NORMAL );
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException );
		virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException );
		virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException );
		virtual void ProcessInput( void ) const throws_( CUserAbortedException );
	public:
		static ui::IProgressService* Instance( void );
	};
}


#endif // IProgressService_h
