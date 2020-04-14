#ifndef BaseDetailHostCtrl_h
#define BaseDetailHostCtrl_h
#pragma once

#include "IconButton.h"


namespace ui
{
	interface IBuddyCommand
	{
		virtual void OnBuddyCommand( UINT cmdId ) = 0;
	};
}


// works in tandem with a host control (e.g. an CEdit)

class CDetailButton : public CIconButton
{
public:
	CDetailButton( ui::IBuddyCommand* pOwnerCallback, UINT iconId = 0 );

	void Create( CWnd* pHostCtrl );
	void Layout( void );				// both host & detail

	enum Metrics { SpacingToButton = 2 };

	void SetSpacing( int spacingToButton ) { m_spacingToButton = spacingToButton; }
protected:
	CRect GetHostCtrlRect( void ) const;
private:
	ui::IBuddyCommand* m_pOwnerCallback;
	CWnd* m_pHostCtrl;
	int m_spacingToButton;

	enum { ButtonId = -1 };
private:
	// generated message map
	afx_msg BOOL OnReflect_BnClicked( void );

	DECLARE_MESSAGE_MAP()
};


// a control that has a detail button for editing

template< typename BaseCtrl >
abstract class CBaseDetailHostCtrl : public BaseCtrl
								   , protected ui::IBuddyCommand
{
protected:
	CBaseDetailHostCtrl( void ) : BaseCtrl(), m_pDetailButton( new CDetailButton( this ) ), m_ignoreResize( false ) {}
public:
	CDetailButton* GetDetailButton( void ) const { return m_pDetailButton.get(); }
	void SetDetailButton( CDetailButton* pDetailButton ) { m_pDetailButton.reset( pDetailButton ); }
private:
	std::auto_ptr< CDetailButton > m_pDetailButton;
	bool m_ignoreResize;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnSize( UINT sizeType, int cx, int cy );

	DECLARE_MESSAGE_MAP()
};


#include "ItemContent.h"


#define ON_CN_EDITDETAILS( id, memberFxn )		ON_CONTROL( CBaseItemContentCtrl< CStatic >::CN_EDITDETAILS, id, memberFxn )
#define ON_CN_DETAILSCHANGED( id, memberFxn )	ON_CONTROL( CBaseItemContentCtrl< CStatic >::CN_DETAILSCHANGED, id, memberFxn )


// content control with details button

template< typename BaseCtrl >
abstract class CBaseItemContentCtrl : public CBaseDetailHostCtrl< BaseCtrl >
{
protected:
	CBaseItemContentCtrl( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL ) : m_content( type, pFileFilter ) {}

	// interface IBuddyCommand (may be overridden)
	virtual void OnBuddyCommand( UINT cmdId );
public:
	// custom notifications: handled the standard way with ON_CONTROL( NotifyCode, id, memberFxn )
	enum NotifCode { CN_EDITDETAILS = 0x0a00, CN_DETAILSCHANGED = 0x0b00 };

	ui::CItemContent& GetContent( void ) { return m_content; }

	virtual void SetContentType( ui::ContentType type );
	void SetFileFilter( const TCHAR* pFileFilter );
	void SetEnsurePathExist( void ) { SetFlag( m_content.m_itemsFlags, ui::CItemContent::EnsurePathExist ); }

	void SetStringContent( bool allowEmptyItem = true, bool noDetailsButton = true )		// by default allow a single empty item
	{
		SetFlag( m_content.m_itemsFlags, ui::CItemContent::RemoveEmpty, !allowEmptyItem );
		if ( noDetailsButton )
			SetDetailButton( NULL );
	}
protected:
	bool UseStockButtonIcon( void ) const;
	virtual UINT GetStockButtonIconId( void ) const;
protected:
	ui::CItemContent m_content;
};


#include "BaseDetailHostCtrl.hxx"


#endif // BaseDetailHostCtrl_h
