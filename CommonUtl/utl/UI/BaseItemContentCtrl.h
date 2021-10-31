#ifndef BaseItemContentCtrl_h
#define BaseItemContentCtrl_h
#pragma once

#include "Control_fwd.h"


class CDialogToolBar;


// a control that has a buddy details toolbar with editing commands (possibly multiple)

template< typename BaseCtrl >
abstract class CBaseDetailHostCtrl
	: public BaseCtrl
	, protected ui::IBuddyCommandHandler
{
	typedef BaseCtrl TBaseClass;
protected:
	CBaseDetailHostCtrl( void );
public:
	enum Metrics { Spacing = 2 };

	const ui::CTandemLayout& GetTandemLayout( void ) const { return m_tandemLayout; }
	ui::CTandemLayout& RefTandemLayout( void ) { return m_tandemLayout; }

	bool HasMateToolbar( void ) const { return m_pMateToolbar.get() != NULL; }
	CDialogToolBar* GetMateToolbar( void ) const { return m_pMateToolbar.get(); }
	void ResetMateToolbar( void );

	const std::vector< UINT >& GetMateCommands( void ) const;
	bool ContainsMateCommand( UINT cmdId ) const;
private:
	void LayoutMates( void );
private:
	CWnd* m_pParentWnd;
	std::auto_ptr<CDialogToolBar> m_pMateToolbar;
	ui::CTandemLayout m_tandemLayout;
	bool m_ignoreResize;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual BOOL OnCommand( WPARAM wParam, LPARAM lParam );
	virtual bool OnMateCommand( UINT cmdId );
protected:
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


#include "ItemContent.h"


#define ON_CN_EDITDETAILS( id, memberFxn )		ON_CONTROL( CBaseItemContentCtrl<CStatic>::CN_EDITDETAILS, id, memberFxn )
#define ON_CN_DETAILSCHANGED( id, memberFxn )	ON_CONTROL( CBaseItemContentCtrl<CStatic>::CN_DETAILSCHANGED, id, memberFxn )


// content control with details button

template< typename BaseCtrl >
abstract class CBaseItemContentCtrl : public CBaseDetailHostCtrl<BaseCtrl>
{
	typedef CBaseDetailHostCtrl<BaseCtrl> TBaseClass;
protected:
	CBaseItemContentCtrl( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL ) : m_content( type, pFileFilter ) {}

	// interface IBuddyCommandHandler (may be overridden)
	virtual bool OnBuddyCommand( UINT cmdId );
public:
	// custom notifications: handled the standard way with ON_CONTROL( NotifyCode, id, memberFxn )
	enum NotifCode { CN_EDITDETAILS = 0x0a00, CN_DETAILSCHANGED = 0x0b00 };

	ui::CItemContent& GetContent( void ) { return m_content; }

	virtual void SetContentType( ui::ContentType type );
	void SetFileFilter( const TCHAR* pFileFilter );
	void SetEnsurePathExist( void ) { SetFlag( m_content.m_itemsFlags, ui::CItemContent::EnsurePathExist ); }

	void SetStringContent( bool allowEmptyItem = true, bool noMateButton = true );		// by default allow a single empty item
public:
	virtual void PreSubclassWindow( void );
protected:
	virtual void OnDroppedFiles( const std::vector< fs::CPath >& filePaths ) = 0;
protected:
	ui::CItemContent m_content;

	// generated stuff
protected:
	afx_msg void OnDropFiles( HDROP hDropInfo );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseItemContentCtrl_h
