#ifndef Options_h
#define Options_h
#pragma once


class CEnumTags;


namespace opt
{
	enum FrameStyle { EntireWindow, NonClient, Frame };
	const CEnumTags& GetTags_FrameStyle( void );

	enum UpdateTarget { CurrentWnd, AtMouseWnd, ForegroundWnd, ActiveWnd, FocusWnd, TopmostWnd, TopmostPopupWnd, TopmostVisibleWnd };
	const CEnumTags& GetTags_AutoUpdate( void );
}


struct COptions
{
	COptions( void );

	void Load( void );
	void Save( void ) const;
public:
	bool m_keepTopmost;
	bool m_hideOnTrack;
	bool m_autoHighlight;
	bool m_ignoreHidden;
	bool m_ignoreDisabled;
	bool m_displayZeroFlags;

	opt::FrameStyle m_frameStyle;
	int m_frameSize;

	bool m_autoUpdate;
	bool m_autoUpdateRefresh;
	int m_autoUpdateTimeout;
	opt::UpdateTarget m_updateTarget;
};


#endif // Options_h
