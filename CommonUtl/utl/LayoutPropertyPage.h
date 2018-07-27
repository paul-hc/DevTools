#ifndef LayoutPropertyPage_h
#define LayoutPropertyPage_h
#pragma once

#include "Dialog_fwd.h"
#include "PopupDlgBase.h"
#include "LayoutMetrics.h"
#include "RuntimeException.h"
#include <afxdlgs.h>


class CLayoutPropertyPage;
class CLayoutBasePropertySheet;
class CMacroCommand;


// implemented by the parent of an embedded child page; routes deep control notifications to the parent

interface IEmbeddedPageCallback
{
	virtual void OnChildPageNotify( CLayoutPropertyPage* pEmbeddedPage, CWnd* pCtrl, int notifCode ) = 0;
};


class CLayoutPropertyPage : public CPropertyPage
						  , public ui::ILayoutEngine
						  , public ui::ICustomCmdInfo
{
protected:
	CLayoutPropertyPage( UINT templateId, UINT titleId = 0 );
public:
	virtual ~CLayoutPropertyPage();

	CLayoutBasePropertySheet* GetParentSheet( void ) const;
	CWnd* GetParentOwner( void ) const;

	CMacroCommand* GetApplyMacroCmd( void ) const;

	bool UseLazyUpdateData( void ) const { return m_useLazyUpdateData; }
	void SetUseLazyUpdateData( bool useLazyUpdateData = true ) { m_useLazyUpdateData = useLazyUpdateData; }

	UINT GetTemplateId( void ) const { return m_templateId; }

	void SetPremature( void ) { m_psp.dwFlags |= PSP_PREMATURE; }		// create page when sheet is created
	void SetTitle( const std::tstring& pageTitle );

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void );
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );
	virtual bool HasControlLayout( void ) const;

	// overridables
	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
	virtual bool LayoutPage( void );

	virtual void SetModified( bool changed );
protected:
	bool StoreFocusControl( void );
	bool RestoreFocusControl( void );

	virtual void OnIdleUpdateControls( void );
private:
	using CPropertyPage::SetModified;
private:
	UINT m_templateId;
	std::auto_ptr< CLayoutEngine > m_pLayoutEngine;
	bool m_useLazyUpdateData;		// call UpdateData on page activation change

	HWND m_hCtrlFocus;
protected:
	bool m_idleUpdateDeep;			// send WM_IDLEUPDATECMDUI to all descendants
	CAccelPool m_accelPool;
private:
	// virtual function overrides
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL OnSetActive( void );
	virtual BOOL OnKillActive( void );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg LRESULT OnKickIdle( WPARAM, LPARAM );
	virtual BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );
	virtual void OnFieldModified( void );
	afx_msg void OnFieldModified( UINT ctrlId );

	DECLARE_MESSAGE_MAP()
};


class CPageValidationException : public CRuntimeException
{
public:
	explicit CPageValidationException( const std::tstring& message, CWnd* pErrorCtrl = NULL );
	virtual ~CPageValidationException() throw();

	CWnd* GetErrorCtrl( void ) const { return m_pErrorCtrl; }
	bool FocusErrorCtrl( void ) const;
protected:
	CWnd* m_pErrorCtrl;
};


#endif // LayoutPropertyPage_h
