#ifndef LayoutFormView_h
#define LayoutFormView_h
#pragma once

#include "Dialog_fwd.h"
#include "LayoutMetrics.h"


// base class for form views that require dynamic control layout based on a layout description

class CLayoutFormView : public CFormView
					  , public ui::ILayoutEngine
					  , public ui::ICustomCmdInfo
{
protected:
	CLayoutFormView( UINT templateId );
	CLayoutFormView( const TCHAR* pTemplateName );
	virtual ~CLayoutFormView();
public:
	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void );
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );
	virtual bool HasControlLayout( void ) const;
private:
	std::auto_ptr< CLayoutEngine > m_pLayoutEngine;
protected:
	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
public:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // LayoutFormView_h
