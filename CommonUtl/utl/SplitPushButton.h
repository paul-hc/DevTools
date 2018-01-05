#ifndef SplitPushButton_h
#define SplitPushButton_h
#pragma once

#include "BaseSplitButton.h"


enum SplitPushNotification { SBN_RIGHTCLICKED = 100 };


// split push button notification macros
#define ON_SBN_RIGHTCLICKED( id, memberFxn ) ON_CONTROL( SBN_RIGHTCLICKED, id, memberFxn )


class CSplitPushButton : public CBaseSplitButton
{
public:
	CSplitPushButton( void );
	virtual ~CSplitPushButton();

	const CIcon* GetRhsIcon( void ) const;
	void SetRhsIconId( const CIconId& rhsIconId );

	const std::tstring& GetRhsText( void ) const { return m_rhsText; }
	void SetRhsText( const std::tstring& rhsText );

	ButtonSide GetActiveSide( void ) const { return m_activeSide; }
	void SetActiveSide( ButtonSide activeSide );
public:
	// base overrides
	virtual bool HasRhsPart( void ) const { return m_rhsIconId.IsValid() || !m_rhsText.empty(); }
	virtual CRect GetRhsPartRect( const CRect* pClientRect = NULL ) const;
protected:
	virtual void DrawRhsPart( CDC* pDC, const CRect& clientRect );
	virtual void DrawFocus( CDC* pDC, const CRect& clientRect );
private:
	CIconId m_rhsIconId;				// for smaller images use DefaultSize
	std::tstring m_rhsText;
	ButtonSide m_activeSide;

	enum { IconSpacing = 1, TextSpacing = 1 };
protected:
	// generated overrides
	public:
	virtual void PreSubclassWindow( void );
protected:
	// message map functions
	afx_msg void OnLButtonDown( UINT flags, CPoint point );
	afx_msg BOOL OnReflectClicked( void );

	DECLARE_MESSAGE_MAP()
};


#endif // SplitPushButton_h
