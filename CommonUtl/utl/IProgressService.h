#ifndef IProgressService_h
#define IProgressService_h
#pragma once


class CUserAbortedException;


namespace utl
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
		virtual utl::IProgressHeader* GetHeader( void ) = 0;

		// progress type
		virtual bool SetMarqueeProgress( void ) = 0;
		virtual void SetBoundedProgressCount( size_t itemCount, bool rewindPos = true ) = 0;
		virtual void SetProgressStep( int step ) = 0;							// step divider for less granular progress updates (default is 10 for CProgressCtrl)

		// progress bar colour
		virtual void SetProgressState( int barState = PBST_NORMAL ) = 0;		// PBST_NORMAL/PBST_PAUSED/PBST_ERROR

		// advancing
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException ) = 0;
		virtual void ProcessInput( void ) const throws_( CUserAbortedException ) = 0;		// yield cooperatively
	};


	interface IStatusProgressService		// implemented in status bar, simple service
	{
		virtual bool SetLabelText( const std::tstring& text, COLORREF labelTextColor = CLR_DEFAULT, COLORREF labelBkColor = CLR_DEFAULT ) = 0;
		virtual size_t GetMaxPos( void ) const = 0;
		virtual size_t GetPos( void ) const = 0;
		virtual void SetPos( size_t pos ) = 0;

		// implemented
		void Advance( ptrdiff_t step = 1 ) { SetPos( GetPos() + step ); }
		void SkipToMax( void ) { SetPos( GetMaxPos() ); }
	};
}


namespace svc
{
	// Null Pattern: placeholder for a valid svc::IProgressService interface with empty implementation
	//
	class CNoProgressService : private utl::IProgressHeader
							 , public utl::IProgressService
	{
		CNoProgressService( void ) {}

		// utl::IProgressHeader interface
		virtual void SetDialogTitle( const std::tstring& title );
		virtual void SetOperationLabel( const std::tstring& operationLabel );
		virtual void ShowStage( bool show = true );
		virtual void SetStageLabel( const std::tstring& stageLabel );
		virtual void ShowItem( bool show = true );
		virtual void SetItemLabel( const std::tstring& itemLabel );

		// utl::IProgressService interface
		virtual utl::IProgressHeader* GetHeader( void );
		virtual bool SetMarqueeProgress( void );
		virtual void SetBoundedProgressCount( size_t itemCount, bool rewindPos = true );
		virtual void SetProgressStep( int step );
		virtual void SetProgressState( int barState = PBST_NORMAL );
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException );
		virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException );
		virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException );
		virtual void ProcessInput( void ) const throws_( CUserAbortedException );
	public:
		static utl::IProgressService* Instance( void );
	};
}


#endif // IProgressService_h
