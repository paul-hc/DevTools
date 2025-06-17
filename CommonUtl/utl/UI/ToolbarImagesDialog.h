#ifndef ToolbarImagesDialog_h
#define ToolbarImagesDialog_h
#pragma once

#include "LayoutDialog.h"
#include "LayoutChildPropertySheet.h"
#include "SampleView_fwd.h"
#include <afxtempl.h>


namespace mfc
{
	typedef CList<CRuntimeClass*, CRuntimeClass*> TRuntimeClassList;
}

class CResizeFrameStatic;
class CLayoutStatic;
class CBaseImagesPage;


class CToolbarImagesDialog : public CLayoutDialog
	, private ui::ISampleCallback
{
public:
	CToolbarImagesDialog( CWnd* pParentWnd );
	virtual ~CToolbarImagesDialog();

	static mfc::TRuntimeClassList* GetCustomPages( void );		// additional pages to add to CMFCToolBarsCustomizeDialog
private:
	CBaseImagesPage* GetActiveChildPage( void ) const;

	// ui::ISampleCallback interface
	virtual bool RenderSample( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) implements(ui::ISampleCallback);
private:
	// enum { IDD = IDD_IMAGES_TOOLBAR_DIALOG };
	CLayoutChildPropertySheet m_childSheet;
	std::auto_ptr<CSampleView> m_pSampleView;

	std::auto_ptr<CLayoutStatic> m_pSplitBottomLayoutStatic;
	std::auto_ptr<CResizeFrameStatic> m_pUpDownSplitterFrame;		// embedded inside of vertical splitter
public:
	enum Page { MfcToolBarImagesPage, StoreToolbarImagesPage, IconImagesPage };

	struct CData
	{
		CData( void ) : m_drawDisabled( false ), m_srcAlpha( 255 ), m_disabledAlpha( 127 ), m_pSampleView( nullptr ) {}
	public:
		persist bool m_drawDisabled;
		persist BYTE m_srcAlpha;
		persist BYTE m_disabledAlpha;
		CSampleView* m_pSampleView;
	};

	CData m_sharedData;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnIdleUpdateControls( void ) overrides(CLayoutDialog);			// override to update specific controls
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnRedrawImagesList( void );
	afx_msg void OnUpdateUseAlpha( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


class CMFCToolBarImages;
struct CBaseImageItem;


#include "LayoutPropertyPage.h"
#include "ReportListControl.h"
#include "Image_fwd.h"


class CBaseImagesPage : public CLayoutPropertyPage
	, private ui::ICustomImageDraw
{
	DECLARE_DYNAMIC( CBaseImagesPage )
protected:
	CBaseImagesPage( UINT templateId, const TCHAR* pTitle );
	virtual ~CBaseImagesPage();

	virtual void AddListItems( void );
	void AddListItem( int itemPos, const CBaseImageItem* pImageItem );
public:
	virtual bool RenderImageSample( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) implements(ui::ISampleCallback);
private:
	void OutputList( void );

	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const implements(ui::ICustomImageDraw);
	virtual bool SetItemImageSize( const CSize& imageBoundsSize ) implements(ui::ICustomImageDraw);
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) implements(ui::ICustomImageDraw);
protected:
	const CToolbarImagesDialog::CData* m_pDlgData;	// from parent dialog
	std::tstring m_regSection;

	std::vector<CBaseImageItem*> m_imageItems;
	CSize m_imageBoundsSize;
	std::tstring m_imageCountText;					// for label output

	persist int m_selItemIndex;

	CReportListControl m_imageListCtrl;

	enum Column { CommandName, Index, Format, CmdId, CmdLiteral };
private:
	static CToolbarImagesDialog::CData s_defaultDlgData;	// use when pages are added in Tools > Customize dialog

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnCopyItems( void );
	afx_msg void OnUpdateCopyItems( CCmdUI* pCmdUI );
	afx_msg void OnLvnItemChanged_ImageListCtrl( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


class CMfcToolbarImagesPage : public CBaseImagesPage
{
	DECLARE_DYNCREATE( CMfcToolbarImagesPage )
public:
	CMfcToolbarImagesPage( CMFCToolBarImages* pImages = nullptr );
	virtual ~CMfcToolbarImagesPage();
private:
	// enum { IDD = IDD_IMAGES_MFC_TOOLBAR_PAGE };
	CMFCToolBarImages* m_pImages;
};


namespace ui
{
	interface IImageStore;
	class CToolbarDescr;
}


class CBaseStoreImagesPage : public CBaseImagesPage
{
	DECLARE_DYNAMIC( CBaseStoreImagesPage )
protected:
	CBaseStoreImagesPage( const TCHAR* pTitle, ui::IImageStore* pImageStore );
	virtual ~CBaseStoreImagesPage();
protected:
	// enum { IDD = IDD_IMAGES_STORE_PAGE };
	ui::IImageStore* m_pImageStore;
};


class CStoreToolbarImagesPage : public CBaseStoreImagesPage
{
	DECLARE_DYNCREATE( CStoreToolbarImagesPage )
public:
	CStoreToolbarImagesPage( ui::IImageStore* pImageStore = nullptr );
	virtual ~CStoreToolbarImagesPage();
protected:
	virtual void AddListItems( void ) override;
private:
	typedef std::pair< std::tstring, std::vector<CBaseImageItem*> > TToolbarGroupPair;		// <groupName, imageItems>

	std::vector<TToolbarGroupPair> m_toolbarGroups;
};


class CIconImagesPage : public CBaseStoreImagesPage
{
	DECLARE_DYNCREATE( CIconImagesPage )
public:
	CIconImagesPage( ui::IImageStore* pImageStore = nullptr );
	virtual ~CIconImagesPage();
};


#endif // ToolbarImagesDialog_h
