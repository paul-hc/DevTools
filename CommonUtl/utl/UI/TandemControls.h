#ifndef TandemControls_h
#define TandemControls_h
#pragma once

#include "Control_fwd.h"


class CDialogToolBar;


// A control that has a mate details toolbar with editing commands, that gets laid-out as a tandem.

template< typename BaseCtrl >
abstract class CBaseHostToolbarCtrl
	: public BaseCtrl
	, protected ui::IBuddyCommandHandler
{
	typedef BaseCtrl TBaseClass;
protected:
	CBaseHostToolbarCtrl( void );
public:
	virtual ~CBaseHostToolbarCtrl();

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
protected:
	CWnd* m_pParentWnd;
private:
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


// Concrete control class with mate toolbar; buddy commands are handled by BaseCtrl.

template< typename BaseCtrl >
class CHostToolbarCtrl : public CBaseHostToolbarCtrl<BaseCtrl>
{
public:
	CHostToolbarCtrl( ui::TTandemAlign tandemAlign = ui::EditShinkHost_MateOnRight );

	void DDX_Tandem( CDataExchange* pDX, int ctrlId, CWnd* pWndTarget = NULL );
protected:
	// interface IBuddyCommandHandler (may be overridden) - default implementation redirects handling to this host control
	virtual bool OnBuddyCommand( UINT cmdId );

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
};


#include "ItemContent.h"


#define ON_CN_EDITDETAILS( id, memberFxn )		ON_CONTROL( CBaseItemContentCtrl<CStatic>::CN_EDITDETAILS, id, memberFxn )
#define ON_CN_DETAILSCHANGED( id, memberFxn )	ON_CONTROL( CBaseItemContentCtrl<CStatic>::CN_DETAILSCHANGED, id, memberFxn )


// content control with details button

template< typename BaseCtrl >
abstract class CBaseItemContentCtrl : public CBaseHostToolbarCtrl<BaseCtrl>
{
	typedef CBaseHostToolbarCtrl<BaseCtrl> TBaseClass;
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


#endif // TandemControls_h