#ifndef IProgressCallback_h
#define IProgressCallback_h
#pragma once


class CUserAbortedException;


namespace ui
{
	interface IProgressCallback
	{
		virtual void SetProgressRange( int lower, int upper, bool rewindPos = false ) = 0;
		virtual bool SetMarqueeProgress( bool useMarquee = true ) = 0;
		virtual void SetProgressState( int barState = PBST_NORMAL ) = 0;

		// advancing
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException ) = 0;
		virtual void ProcessInput( void ) const throws_( CUserAbortedException ) = 0;

		void SetProgressItemCount( size_t itemCount, bool rewindPos = true ) { SetProgressRange( 0, static_cast< int >( itemCount - 1 ), rewindPos ); }
	};
}


#endif // IProgressCallback_h
