#ifndef IProgressBox_h
#define IProgressBox_h
#pragma once


class CUserAbortedException;


namespace ui
{
	interface IProgressBox
	{
		virtual void SetProgressRange( int lower, int upper, bool rewindPos = false ) = 0;
		virtual bool SetMarqueeProgress( bool marquee = true ) = 0;

		// advancing
		virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException ) = 0;
		virtual void AdvanceStepItem( const std::tstring& stepItemName ) throws_( CUserAbortedException ) = 0;

		void SetProgressItemCount( size_t itemCount, bool rewindPos = true ) { SetProgressRange( 0, static_cast< int >( itemCount ), rewindPos ); }
	};
}


#endif // IProgressBox_h
