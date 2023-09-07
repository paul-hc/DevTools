#ifndef ToolbarImagesDialog_h
#define ToolbarImagesDialog_h
#pragma once

#include "LayoutDialog.h"
#include "LayoutChildPropertySheet.h"
#include "LayoutPropertyPage.h"
#include "ReportListControl.h"
#include <afxtempl.h>


typedef CList<CRuntimeClass*, CRuntimeClass*> TRuntimeClassList;


class CToolbarImagesDialog : public CLayoutDialog
{
public:
	CToolbarImagesDialog( CWnd* pParent );
	virtual ~CToolbarImagesDialog();

	static TRuntimeClassList* GetCustomPages( void );		// additional pages to add to CMFCToolBarsCustomizeDialog
private:
	// enum { IDD = IDD_TOOLBAR_IMAGES_DIALOG };
	CLayoutChildPropertySheet m_childSheet;

	enum Page { MfcToolBarImagesPage };

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


class CMFCToolBarImages;
struct CImageItem;


#include "Image_fwd.h"


class CToolbarImagesPage : public CLayoutPropertyPage
	, private ui::ICustomImageDraw
{
	DECLARE_DYNCREATE( CToolbarImagesPage )
public:
	CToolbarImagesPage( CMFCToolBarImages* pImages = nullptr );
	virtual ~CToolbarImagesPage();
private:
	void OutputList( void );

	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const implements(ui::ICustomImageDraw);
	virtual bool SetItemImageSize( const CSize& imageBoundsSize ) implements(ui::ICustomImageDraw);
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) implements(ui::ICustomImageDraw);
private:
	// enum { IDD = IDD_TOOLBAR_IMAGES_PAGE };
	CMFCToolBarImages* m_pImages;
	CSize m_imageSize;
	std::vector<CImageItem*> m_imageItems;

	persist int m_selItemIndex;
	persist bool m_drawDisabled;
	persist BYTE m_alphaSrc;

	CReportListControl m_imageListCtrl;
	CSize m_imageBoundsSize;

	enum Column { CommandName, Index, CmdId, CmdLiteral };

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnCopyItems( void );
	afx_msg void OnUpdateCopyItems( CCmdUI* pCmdUI );
	afx_msg void OnLvnItemChanged_ImageListCtrl( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnRedrawImagesList( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ToolbarImagesDialog_h
