#ifndef ThemeStatic_h
#define ThemeStatic_h
#pragma once

#include "BaseStatic.h"
#include "ThemeItem.h"
#include "ui_fwd.h"


class CThemeStatic : public CBaseStatic
{
public:
	CThemeStatic( const CThemeItem& bkgndItem, const CThemeItem& contentItem = CThemeItem::m_null );
	virtual ~CThemeStatic();

	bool IsThemed( void ) const { return m_bkgndItem.IsThemed() || m_contentItem.IsThemed(); }

	enum State { Normal, Hot, Pressed };

	State GetState( void ) const { return m_state; }
	bool SetState( State state );
protected:
	CThemeItem::Status GetDrawStatus( void ) const;
	CThemeItem* GetTextThemeItem( void );

	// base overrides
	virtual bool HasCustomFacet( void ) const;
	virtual void Draw( CDC* pDC, const CRect& clientRect );
	virtual void DrawTextContent( CDC* pDC, const CRect& textBounds, CThemeItem::Status drawStatus );

	void DrawFallbackText( const CThemeItem* pTextTheme, CThemeItem::Status drawStatus, CDC* pDC, CRect& rRect, const std::tstring& text, UINT dtFlags,
						   CFont* pFallbackFont = NULL ) const;
public:
	CThemeItem m_bkgndItem;
	CThemeItem m_contentItem;
	CThemeItem m_textItem;
	CSize m_textSpacing;
	bool m_useText;
private:
	State m_state;
protected:
	// message map functions
	afx_msg void OnMouseMove( UINT flags, CPoint point );
	afx_msg LRESULT OnMouseLeave( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


class CStatusStatic : public CThemeStatic
{
public:
	enum Style { Recessed, Rounded, RoundedHot };

	CStatusStatic( Style style = Recessed );

	void SetStyle( Style style );
};


class CPickMenuStatic : public CThemeStatic
{
public:
	CPickMenuStatic( ui::PopupAlign popupAlign = ui::DropRight );
	virtual ~CPickMenuStatic();

	ui::PopupAlign GetPopupAlign( void ) const { return m_popupAlign; }
	void SetPopupAlign( ui::PopupAlign popupAlign );

	void TrackMenu( CWnd* pTargetWnd, CMenu* pPopupMenu );
	void TrackMenu( CWnd* pTargetWnd, UINT menuId, int popupIndex, bool useCheckedBitmaps = false );
protected:
	// base overrides
	virtual void DrawTextContent( CDC* pDC, const CRect& textBounds, CThemeItem::Status drawStatus );
private:
	ui::PopupAlign m_popupAlign;
	enum { ArrowDelta = 4 };
};


class CDetailsButton : public CThemeStatic
{
public:
	CDetailsButton( void );
	virtual ~CDetailsButton();
};


class CLinkStatic : public CThemeStatic
{
public:
	enum { ColorVirginLink = RGB( 0, 0, 255 ), ColorVisitedLink = RGB( 128, 0, 128 ) };

	CLinkStatic( const TCHAR* pLinkPrefix = NULL );
	virtual ~CLinkStatic();

	std::tstring GetLinkText( void ) const;
	bool Navigate( void );
private:
	std::tstring m_linkPrefix;
protected:
	afx_msg LRESULT OnNcHitTest( CPoint point );
	afx_msg BOOL OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message );
	afx_msg BOOL OnBnClicked_Reflect( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ThemeStatic_h
