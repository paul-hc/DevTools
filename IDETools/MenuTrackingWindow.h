#ifndef MenuTrackingWindow_h
#define MenuTrackingWindow_h
#pragma once


enum MenuTrackMode
{
	MTM_None,
	MTM_Command,
	MTM_CommandRepeat,
};


interface IMenuCommandTarget
{
	virtual bool OnMenuCommand( UINT cmdId ) = 0;
};


class CMenuTrackingWindow : public CFrameWnd
{
private:
	virtual ~CMenuTrackingWindow();
public:
	CMenuTrackingWindow( IMenuCommandTarget* pCommandTarget, CMenu& rTrackingMenu, MenuTrackMode menuTrackMode = MTM_None );

	void SetHilightId( UINT hilightId )
	{
		m_hilightId = hilightId;
	}

	void Create( CWnd* pOwner );
	void InvalidateMenuWindow( void );
private:
	void OnMenuEnterIdle( int idleCount );
private:
	IMenuCommandTarget* m_pCommandTarget;
	CMenu& m_rTrackingMenu;
	MenuTrackMode m_menuTrackMode;
	HMENU m_hMenuHover;
	UINT m_hilightId;		// ID of the item to auto hilight

	std::vector< HMENU > m_popups;
private:
	int m_menuIdleCount;
public:
	// generated overrides
	protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
protected:
	// generated message map
	DECLARE_MESSAGE_MAP()
};


#endif // MenuTrackingWindow_h
