#ifndef BaseDetailHostCtrl_h
#define BaseDetailHostCtrl_h
#pragma once

#include "Control_fwd.h"


class CDialogToolBar;


// a control that has a buddy details toolbar with editing commands (possibly multiple)

template< typename BaseCtrl >
abstract class CBaseDetailHostCtrl
	: public BaseCtrl
	, protected ui::IBuddyCommand
{
protected:
	CBaseDetailHostCtrl( void );
public:
	enum Metrics { Spacing = 2 };

	const ui::CBuddyLayout& GetBuddyLayout( void ) const { return m_buddyLayout; }
	ui::CBuddyLayout& RefBuddyLayout( void ) { return m_buddyLayout; }

	bool HasDetailToolbar( void ) const { return m_pDetailToolbar.get() != NULL; }
	CDialogToolBar* GetDetailToolbar( void ) const { return m_pDetailToolbar.get(); }
	void SetDetailToolbar( CDialogToolBar* pDetailToolbar );

	const std::vector< UINT >& GetDetailCommands( void ) const;
	bool ContainsDetailCommand( UINT cmdId ) const;
private:
	enum { MinCmdId = 1, MaxCmdId = 0x7FFF };

	void LayoutDetails( void );
private:
	CWnd* m_pParentWnd;
	std::auto_ptr< CDialogToolBar > m_pDetailToolbar;
	ui::CBuddyLayout m_buddyLayout;
	bool m_ignoreResize;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnDetailCommand( UINT cmdId );
	afx_msg void OnUpdateDetailCommand( CCmdUI* pCmdUI );

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

	void SetStringContent( bool allowEmptyItem = true, bool noDetailsButton = true );		// by default allow a single empty item
protected:
	ui::CItemContent m_content;
};


#include "BaseDetailHostCtrl.hxx"


#endif // BaseDetailHostCtrl_h
