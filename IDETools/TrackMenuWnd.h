#ifndef TrackMenuWnd_h
#define TrackMenuWnd_h
#pragma once

#include "utl/UI/ui_fwd.h"


class CTrackMenuWnd : public CStatic
{
public:
	CTrackMenuWnd( CCmdTarget* pCmdTarget = NULL );
	virtual ~CTrackMenuWnd();

	bool Create( CWnd* pParentWnd );

	void SetRightClickRepeat( bool rightClickRepeat = true ) { m_rightClickRepeat = rightClickRepeat; }
	void SetHilightId( UINT hilightId ) { m_hilightId = hilightId; }

	UINT TrackContextMenu( CMenu* pPopupMenu, CPoint screenPos = ui::GetCursorPos(), UINT flags = TPM_RIGHTBUTTON );		// returns the selected command regardless of trackFlags

	UINT GetSelCmdId( void ) const { return m_selCmdId; }
private:
	bool _HighlightMenuItem( HMENU hHoverPopup );
	std::pair<HMENU, UINT> FindMenuItemFromPoint( const CPoint& screenPos ) const;
private:
	CCmdTarget* m_pCmdTarget;
	bool m_rightClickRepeat;
	UINT m_hilightId;					// item to auto-hilight when tracking the menu (simulate as selected)

	UINT m_selCmdId;
	std::vector< HMENU > m_subMenus;	// temporary

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override;
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnRButtonUp( UINT vkFlags, CPoint point );

	DECLARE_MESSAGE_MAP()
};


#endif // TrackMenuWnd_h
