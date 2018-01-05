#ifndef LayoutChildPropertySheet_h
#define LayoutChildPropertySheet_h
#pragma once

#include "LayoutBasePropertySheet.h"
#include "LayoutMetrics.h"


// base class for top level modeless child property sheets

class CLayoutChildPropertySheet : public CLayoutBasePropertySheet
{
public:
	CLayoutChildPropertySheet( UINT selPageIndex = 0 );
	virtual ~CLayoutChildPropertySheet();

	void SetAutoDeletePages( bool autoDeletePages ) { m_autoDeletePages = autoDeletePages; }

	void DDX_DetailSheet( CDataExchange* pDX, UINT frameStaticId, bool singleLineTab = false );

	void CreateChildSheet( CWnd* pParent );

	// overridables
	virtual CRect GetTabControlRect( void );

	// base overrides
	virtual void LayoutSheet( void );

	using CLayoutBasePropertySheet::ApplyChanges;
protected:
	virtual CButton* GetSheetButton( UINT buttonId ) const;
private:
	// hide base creation methods
	using CLayoutBasePropertySheet::Create;
	using CLayoutBasePropertySheet::CreateEx;
protected:
	bool m_autoDeletePages;				// true by default
	CRect m_tabMargins;

	// generated overrides
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	// message map functions
	afx_msg LRESULT OnPageChanged( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // LayoutChildPropertySheet_h
