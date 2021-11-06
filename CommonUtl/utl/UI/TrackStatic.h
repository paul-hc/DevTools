#ifndef TrackStatic_h
#define TrackStatic_h
#pragma once

#include "Image_fwd.h"


#define ON_TSN_BEGINTRACKING( id, memberFxn ) ON_CONTROL( CTrackStatic::TSN_BEGINTRACKING, id, memberFxn )
#define ON_TSN_ENDTRACKING( id, memberFxn ) ON_CONTROL( CTrackStatic::TSN_ENDTRACKING, id, memberFxn )
#define ON_TSN_TRACK( id, memberFxn ) ON_CONTROL( CTrackStatic::TSN_TRACK, id, memberFxn )


class CTrackStatic : public CStatic
{
public:
	enum NotifyCode { TSN_BEGINTRACKING, TSN_ENDTRACKING, TSN_TRACK, _TSN_LAST };
	enum TrackingResult { Commit, Cancel };

	CTrackStatic( void );
	virtual ~CTrackStatic();

	void SetTrackCursor( HCURSOR hTrackCursor ) { ASSERT_NULL( m_hWnd ); m_hTrackCursor = hTrackCursor; }
	void LoadTrackCursor( UINT trackCursorId );

	void SetTrackIconId( const CIconId& trackIconId ) { ASSERT_NULL( m_hWnd ); m_trackIconId = trackIconId; }
	void SetToolIconId( const CIconId& toolIconId ) { ASSERT_NULL( m_hWnd ); m_toolIconId = toolIconId; }

	bool IsTracking( void ) const { return m_pTrackData.get() != NULL; }
	CPoint GetCursorPos( void ) const { ASSERT( IsTracking() ); return m_pTrackData->m_cursorPos; }
	CSize GetDelta( void ) const { ASSERT( IsTracking() ); return m_pTrackData->m_delta; }
	CPoint GetStartPos( void ) const { ASSERT( IsTracking() ); return m_pTrackData->m_startPos; }
	TrackingResult GetTrackingResult( void ) const { return m_trackingResult; }

	void CancelTracking( void );
protected:
	// overridables
	virtual void BeginTracking( void );
	virtual void EndTracking( void );
	virtual void OnTrack( void );

	void NotifyParent( int notifyCode );
	bool CursorInTool( void ) const;
private:
	CPoint GetTrackCursorHotSpot( void ) const;
	void HandleMouseMove( const CPoint& screenPos );

	void StopTracking( TrackingResult trackingResult );
private:
	HCURSOR m_hTrackCursor;
	CIconId m_trackIconId, m_toolIconId;
	TrackingResult m_trackingResult;

	struct CTrackData
	{
		CTrackData( const CPoint& startPos )
			: m_startPos( startPos ), m_cursorPos( startPos ), m_delta( 0, 0 ), m_restoreStartPos( false ), m_hOldIcon( NULL ), m_hOldCursor( NULL ), m_hOldFocus( NULL ) {}
	public:
		CPoint m_startPos;			// original track start pos
		CPoint m_cursorPos;
		CSize m_delta;				// relative to previous OnTrack
		bool m_restoreStartPos;		// when tracking ends
		HICON m_hOldIcon;
		HCURSOR m_hOldCursor;
		HWND m_hOldFocus;
	};

	std::auto_ptr<CTrackData> m_pTrackData;
public:
	// generated overrides
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	// message map functions
	afx_msg void OnDestroy( void );
	afx_msg void OnLButtonDown( UINT vkFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT vkFlags, CPoint point );
	afx_msg void OnMouseMove( UINT vkFlags, CPoint point );
	afx_msg void OnCaptureChanged( CWnd *pWnd );
	afx_msg void OnCancelMode( void );

	DECLARE_MESSAGE_MAP()
};


interface ITrackToolCallback
{
	virtual bool OnPreTrackMoveCursor( CTrackStatic* pTracking ) = 0;
};


#endif // TrackStatic_h
