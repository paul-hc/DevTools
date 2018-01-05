#ifndef EditPlacementPage_h
#define EditPlacementPage_h
#pragma once

#include "utl/InternalChange.h"
#include "utl/TrackStatic.h"
#include "DetailBasePage.h"


class CEditPlacementPage : public CDetailBasePage
						 , public ITrackToolCallback
						 , private CInternalChange
{
public:
	CEditPlacementPage( void );
	virtual ~CEditPlacementPage();

	// IWndDetailObserver interface
	virtual bool IsDirty( void ) const;
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
private:
	// ITrackToolCallback interface
	virtual bool OnPreTrackMoveCursor( CTrackStatic* pTracking );

	void SetSpinRangeLimits( void );
	void OutputEdits( void );
	bool RepositionWnd( void );
	void HandleInput( void );

	bool IsAutoApply( void ) const;
private:
	CRect m_wndRect, m_oldRect;		// in parent's coords
	CRect m_trackingOldWndRect;
	HWND m_hWndLastTarget;
	static const CSize m_none;
private:
	// enum { IDD = IDD_EDIT_PLACEMENT_PAGE };

	CTrackStatic m_moveTracker;
	CTrackStatic m_sizeTracker;
	CTrackStatic m_topLeftTracker;
	CTrackStatic m_bottomRightTracker;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnTsnBeginTracking( UINT toolId );
	afx_msg void OnTsnEndTracking( UINT toolId );
	afx_msg void OnTsnTrack_Move( void );
	afx_msg void OnTsnTrack_Size( void );
	afx_msg void OnTsnTrack_TopLeft( void );
	afx_msg void OnTsnTrack_BottomRight( void );
	afx_msg void OnEnChange_Edit( UINT editId );
	afx_msg void OnToggle_AutoApply( void );

	DECLARE_MESSAGE_MAP()
};


#endif // EditPlacementPage_h
