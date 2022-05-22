#ifndef BaseTrackMenuWnd_h
#define BaseTrackMenuWnd_h
#pragma once


// abstract base for controls with an optional frame OR focus rect (edits, combos, etc)

template< typename BaseWndT >
abstract class CBaseTrackMenuWnd : public BaseWndT
{
protected:
	CBaseTrackMenuWnd( void ) : BaseWndT() {}

	// generated stuff
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseTrackMenuWnd_h
