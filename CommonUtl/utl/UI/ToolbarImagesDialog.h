#ifndef ToolbarImagesDialog_h
#define ToolbarImagesDialog_h
#pragma once

#include "LayoutDialog.h"
#include "LayoutChildPropertySheet.h"
#include <afxtempl.h>


namespace mfc
{
	typedef CList<CRuntimeClass*, CRuntimeClass*> TRuntimeClassList;
}


class CToolbarImagesDialog : public CLayoutDialog
{
public:
	CToolbarImagesDialog( CWnd* pParentWnd );
	virtual ~CToolbarImagesDialog();

	static mfc::TRuntimeClassList* GetCustomPages( void );		// additional pages to add to CMFCToolBarsCustomizeDialog
private:
	// enum { IDD = IDD_IMAGES_TOOLBAR_DIALOG };
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


#include "LayoutPropertyPage.h"
#include "ReportListControl.h"
#include "SampleView_fwd.h"
#include "Image_fwd.h"


class CResizeFrameStatic;
class CLayoutStatic;


class CBaseImagesPage : public CLayoutPropertyPage
	, private ui::ICustomImageDraw
	, public ui::ISampleCallback
{
	DECLARE_DYNAMIC( CBaseImagesPage )
protected:
	CBaseImagesPage( UINT templateId, const CLayoutStyle layoutStyles[], unsigned int count );
	virtual ~CBaseImagesPage();

	void OutputList( void );
private:
	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const implements(ui::ICustomImageDraw);
	virtual bool SetItemImageSize( const CSize& imageBoundsSize ) implements(ui::ICustomImageDraw);
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) implements(ui::ICustomImageDraw);

	// ui::ISampleCallback interface
	virtual bool RenderSample( CDC* pDC, const CRect& boundsRect ) implements(ui::ISampleCallback);
protected:
	std::vector<CImageItem*> m_imageItems;
	CSize m_imageBoundsSize;

	persist int m_selItemIndex;
	persist bool m_drawDisabled;
	persist BYTE m_alphaSrc;

	CReportListControl m_imageListCtrl;
	std::auto_ptr<CSampleView> m_pSampleView;

	std::auto_ptr<CLayoutStatic> m_pBottomLayoutStatic;
	std::auto_ptr<CResizeFrameStatic> m_pHorizSplitterFrame;		// embedded inside of vertical splitter

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


class CToolbarImagesPage : public CBaseImagesPage
{
	DECLARE_DYNCREATE( CToolbarImagesPage )
public:
	CToolbarImagesPage( CMFCToolBarImages* pImages = nullptr );
	virtual ~CToolbarImagesPage();
private:
	// enum { IDD = IDD_IMAGES_TOOLBAR_PAGE };
	CMFCToolBarImages* m_pImages;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );

	DECLARE_MESSAGE_MAP()
};


namespace ui { interface IImageStore; }


class CIconImagesPage : public CBaseImagesPage
{
	DECLARE_DYNCREATE( CIconImagesPage )
public:
	CIconImagesPage( ui::IImageStore* pImageStore = nullptr );
	virtual ~CIconImagesPage();
private:
	// enum { IDD = IDD_IMAGES_ICONS_PAGE };
	ui::IImageStore* m_pImageStore;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ToolbarImagesDialog_h
