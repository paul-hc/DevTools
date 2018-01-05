#ifndef OleDropSource_h
#define OleDropSource_h
#pragma once

#include <afxole.h>


namespace shell { class CDropTip; }


namespace ole
{
	enum DragResultFlags
	{
		DragStarted			= BIT_FLAG( 0 ),		// drag started (timeout or mouse left the no-drag rect)
		DragCancelled		= BIT_FLAG( 1 ),		// cancelled (whether started or not): ESC key or other mouse button was pressed
		DragLButtonReleased	= BIT_FLAG( 2 ),		// left mouse button released (whether started or not)
		DragDropped			= BIT_FLAG( 3 )			// data has been dropped (effect not DROPEEFECT_NONE)
	};


	// provides drop-tip support (aka drop description in shell interface)
	//
	class CDropSource : public COleDropSource
	{
	public:
		CDropSource( void );

		int GetDragResult( void ) const { return m_dragResult; }
		void SetDataSource( COleDataSource* pDataSource );
		void SetDropTip( const shell::CDropTip* pDropTip ) { m_pDropTip = pDropTip; }
	private:
		bool SetDragImageCursor( DROPEFFECT dwEffect );

		virtual BOOL OnBeginDrag( CWnd* pWnd );
		virtual SCODE QueryContinueDrag( BOOL escapePressed, DWORD keyState );
		virtual SCODE GiveFeedback( DROPEFFECT dropEffect );

		static HCURSOR GetNormalCursor( void );
	private:
		bool m_setCursor;						// internal flag set when Windows cursor must be set
		int m_dragResult;						// drag result flags
		IDataObject* m_pDataObject;				// from COleDataSource
		const shell::CDropTip* m_pDropTip;		// from COleDataSource
	};
}


#endif // OleDropSource_h
