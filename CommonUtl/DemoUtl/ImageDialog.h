#ifndef ImageDialog_h
#define ImageDialog_h
#pragma once

#include "utl/UI/DibSection.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/ItemContentEdit.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/LayoutChildPropertySheet.h"
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/MultiZone.h"
#include "utl/UI/SampleView.h"
#include "ImageDialogUtils.h"


class CEnumTags;
class CPixelInfoSample;
class CColorTableSample;
class CDialogToolBar;
namespace gdi { enum Effect; }


class CImageDialog : public CLayoutDialog
				   , public ISampleCallback
{
	friend class CModePage;
public:
	enum SampleMode { ShowImage, ConvertImage, GrayScale, AlphaBlend, BlendColor, Disabled, ImageList, RectsAlphaBlend, _ModeCount };
	static const CEnumTags& GetTags_SampleMode( void );

	CImageDialog( CWnd* pParent );
	virtual ~CImageDialog();

	CDibSection* GetImage( void ) const { return m_pDibSection.get(); }
	const CColorTable& GetColorTable( void ) const { return m_colorTable; }

	void SetImagePath( const std::tstring& imagePath ) { m_imagePath = imagePath; }
	void BuildColorTable( void );
	void ConvertImages( void ) { CreateEffectDibs(); RedrawSample(); }
private:
	enum Resolution { Auto, Monochrome, Color4, Color8, Color16, Color24, Color32 };
	static const CEnumTags& GetTags_Resolution( void );

	enum Zoom { Zoom25, Zoom50, Zoom100, Zoom200, Zoom400, Zoom800, Zoom1600, Zoom3200, StretchToFit };
	static const CEnumTags& GetTags_Zoom( void );

	bool IsValidImageMode( void ) const;
	WORD GetForceBpp( void ) const;
	COLORREF GetBkColor( void ) const;
	int GetZoomPct( void ) const;
	bool StretchImage( void ) const { return StretchToFit == m_zoom; }
	CSize ComputeContentSize( void );
	void LoadSampleImage( void );
	void UpdateImage( void );
	void RedrawSample( void );
	void RedrawColorTable( void );

	static std::tstring FormatDibInfo( const CDibSection& dib );

	// ISampleCallback interface
	virtual void RenderBackground( CDC* pDC, const CRect& clientRect );
	virtual bool RenderSample( CDC* pDC, const CRect& clientRect );
	virtual void ShowPixelInfo( const CPoint& pos, COLORREF color );

	// sample rendering
	void CreateEffectDibs( void );
	CDibSection* CloneSourceDib( void ) const;
	CDibSection* GetSafeDibAt( size_t dibPos ) const;
	bool BlendDib( CDibSection* pDib, CDC* pDC, const CRect& rect, BYTE srcAlpha = 255 );
	void DrawImageListEffect( CDC* pDC, gdi::Effect effect, COLORREF blendToColor, const CRect& boundsRect );
	CSize GetModeExtraSpacing( void ) const;
	CRect MakeContentRect( const CRect& clientRect ) const;

	enum { IL_SpacingX = 10 };

	void Render_AllImages( CDC* pDC, CMultiZoneIterator* pMultiZone );
	void Render_AlphaBlend( CDC* pDC, CMultiZoneIterator* pMultiZone );
	void Render_ImageList( CDC* pDC, CMultiZoneIterator* pMultiZone );
	bool Render_RectsAlphaBlend( CDC* pDC, const CRect& clientRect );
private:
	enum ShowFlags { ShowGuides = 1 << 0, ShowLabels = 1 << 1 };

	SampleMode m_sampleMode;
	ui::ImagingApi m_imagingApi;
	UINT m_framePos, m_frameCount;					// m_framePos is 1-based
	Resolution m_forceResolution;
	int m_showFlags;
	Zoom m_zoom;
	BYTE m_statusAlpha;
	std::tstring m_imagePath;
	std::tstring m_bkColorText;
	std::auto_ptr< CDibSection > m_pDibSection;
	CColorTable m_colorTable;
	CImageTranspColors m_transpColorCache;
	std::auto_ptr< CImageList > m_pImageList;

	CMultiZone m_multiZone;
	std::vector< CModeData* > m_modeData;
	CMenu m_contextMenu;

	//enum { IDD = IDD_IMAGE_DIALOG };
	CItemContentHistoryCombo m_imagePathCombo;
	CItemContentHistoryCombo m_bkColorCombo;
	CSpinEdit m_framePosEdit;
	CEnumComboBox m_imagingApiCombo;
	CEnumComboBox m_forceResolutionCombo;
	CEnumComboBox m_zoomCombo;
	CEnumComboBox m_stackingCombo;
	CEnumComboBox m_colorTableModeCombo;
	CSpinEdit m_spacingEdit;
	CColorSample m_transpColorSample;
	std::auto_ptr< CDialogToolBar > m_pImageToolbar;
	CSampleView m_sampleView;
	std::auto_ptr< CPixelInfoSample > m_pPixelInfoSample;
	std::auto_ptr< CColorTableSample > m_pColorTableSample;
	CColorChannelEdit m_statusAlphaEdit;
	CLayoutChildPropertySheet m_modeSheet;
public:
	// sample details
	int m_convertFlags;				// overides CDibSection::m_testFlags during conversion
	BYTE m_sourceAlpha;
	BYTE m_pixelAlpha;
	BYTE m_blendColor;
	BYTE m_disabledAlpha;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK( void );
	virtual void OnCancel( void ) { OnOK(); }
protected:
	// message map functions
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType );
	afx_msg void OnChange_ImageFile( void );
	afx_msg void OnChange_FramePos( void );
	afx_msg void OnChange_BkColor( void );
	afx_msg void OnChange_SampleMode( void );
	afx_msg void OnChange_ImagingApi( void );
	afx_msg void OnChange_ForceResolution( void );
	afx_msg void OnToggle_ForceResolution( void );
	afx_msg void OnToggle_SkipCopyImage( void );
	afx_msg void OnChange_Zoom( void );
	afx_msg void OnToggle_Zoom( void );
	afx_msg void OnChange_Stacking( void );
	afx_msg void OnChange_Spacing( void );
	afx_msg void OnToggle_ShowFlags( UINT checkId );
	afx_msg void OnToggle_ShowColorTable( void );
	afx_msg void OnChange_ColorTableMode( void );
	afx_msg void OnToggle_UniqueColors( void );
	afx_msg void OnToggle_ShowLabels( void );
	afx_msg void OnSetTranspColor( void );
	afx_msg void OnSetAutoTranspColor( void );
	afx_msg void OnResetTranspColor( void );
	afx_msg void OnUpdateTranspColor( CCmdUI* pCmdUI );
	afx_msg void OnRedrawSample( void );

	DECLARE_MESSAGE_MAP()
};


class CPixelInfoSample : public CColorSample
{
public:
	CPixelInfoSample( void ) : m_pos( -1, -1 ) {}

	void SetPixelInfo( COLORREF color, const CPoint& pos ) { m_pos = pos; SetColor( color ); }
	void Reset( void ) { SetPixelInfo( CLR_NONE, CPoint( -1, -1 ) ); }
private:
	CPoint m_pos;
};


class CColorTableSample : public CColorSample
{
public:
	CColorTableSample( const CColorTable* pColorTable, ISampleCallback* pRoutePixelInfo )
		: CColorSample( pRoutePixelInfo ), m_pColorTable( pColorTable ) { ASSERT_PTR( m_pColorTable ); }
protected:
	virtual bool RenderSample( CDC* pDC, const CRect& clientRect );
private:
	const CColorTable* m_pColorTable;
};


class CModePage : public CLayoutPropertyPage
{
protected:
	CModePage( CImageDialog* pDialog, CImageDialog::SampleMode mode, UINT templateId );
public:
	virtual ~CModePage();
	static CModePage* NewEmptyPage( CImageDialog* pDialog, CImageDialog::SampleMode mode );

	virtual BOOL OnSetActive( void );
protected:
	CImageDialog* m_pDialog;
	CImageDialog::SampleMode m_mode;

	std::vector< CColorChannelEdit* > m_channelEdits;
	std::vector< CColorChannelEdit* > m_keepEqualEdits;		// chanel edits with synced values
protected:
	virtual bool IsPixelCtrl( UINT ctrlId ) const;			// must regenerate effect DIBs
	bool SyncEditValues( const CColorChannelEdit* pRefEdit );

	virtual void DoDataExchange( CDataExchange* pDX );
	afx_msg void OnChangeField( UINT ctrlId );
	afx_msg void OnToggle_KeepEqual( void );
	afx_msg void OnPageInput( UINT checkId );

	DECLARE_MESSAGE_MAP()
};


class CShowImagePage : public CModePage
{
public:
	CShowImagePage( CImageDialog* pDialog );
private:
	CColorSample m_transpColorSample;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
};


class CConvertModePage : public CModePage
{
public:
	CConvertModePage( CImageDialog* pDialog );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	DECLARE_MESSAGE_MAP()
};


class CAlphaBlendModePage : public CModePage
{
public:
	CAlphaBlendModePage( CImageDialog* pDialog );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
};


class CBlendColorModePage : public CModePage
{
public:
	CBlendColorModePage( CImageDialog* pDialog );
};


class CDisabledModePage : public CModePage
{
public:
	CDisabledModePage( CImageDialog* pDialog );
};


#endif // ImageDialog_h
