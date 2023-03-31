#ifndef SyncScrolling_h
#define SyncScrolling_h
#pragma once


class CThumbTrackScrollHook;


class CSyncScrolling
{
public:
	// scrollType: SB_HORZ, SB_VERT, SB_BOTH
	CSyncScrolling( int scrollType = SB_VERT ) { SetScrollType( scrollType ); }

	void SetScrollType( int scrollType );
	bool SyncHorizontal( void ) const;
	bool SyncVertical( void ) const;

	void SetCtrls( CWnd* pDlg, const UINT ctrlIds[], size_t count );

	CSyncScrolling& AddCtrl( CWnd* pCtrl );

	void HookThumbTrack( void );			// track thumb track scrolling events (edit controls don't send EN_VSCROLL on thumb track scrolling)

	bool Synchronize( CWnd* pRefCtrl );		// call from a notification handler such as for edits: ON_CONTROL_RANGE( EN_VSCROLL, IDC_EDIT_1, IDC_EDIT_N, OnEnVScroll )
private:
	std::vector<int> m_scrollTypes;
	std::vector<CWnd*> m_ctrls;
	std::vector<CThumbTrackScrollHook*> m_scrollHooks;			// with auto-delete
};


#endif // SyncScrolling_h
