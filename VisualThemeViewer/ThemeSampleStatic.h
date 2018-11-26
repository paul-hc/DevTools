#ifndef ThemeSampleStatic_h
#define ThemeSampleStatic_h
#pragma once

#include "utl/BufferedStatic.h"
#include "utl/ThemeItem.h"
#include "utl/GpUtilities.h"


class CHistoryComboBox;


interface ISampleOptionsCallback
{
	virtual CWnd* GetWnd( void ) = 0;
	virtual void RedrawSamples( void ) = 0;
};


struct CThemeSampleOptions : public CCmdTarget
{
	CThemeSampleOptions( ISampleOptionsCallback* pCallback );
	~CThemeSampleOptions();

	COLORREF GetBkColor( void ) const;
private:
	CHistoryComboBox* GetBkColorCombo( void ) const;
public:
	ISampleOptionsCallback* m_pCallback;
	std::tstring m_bkColorText;
	bool m_useBorder;
	bool m_preBkGuides;
	bool m_postBkGuides;
protected:
	// message map functions
	afx_msg void OnChange_BkColor( void );
	afx_msg void OnToggle_UseBorder( void );
	afx_msg void OnToggle_DrawBkGuides( UINT cmdId );
	afx_msg void OnToggle_DisableThemes( void );

	DECLARE_MESSAGE_MAP()
};


class CThemeSampleStatic : public CBufferedStatic
{
public:
	CThemeSampleStatic( void );
	virtual ~CThemeSampleStatic();

	void SetOptions( const CThemeSampleOptions* pOptions ) { m_pOptions = pOptions; }
	void SetThemeItem( CThemeItem& themeItem );

	bool InSizeToContentMode( void ) const { return m_stcInfoId != 0; }
	void SetSizeToContentMode( UINT infoId ) { m_stcInfoId = infoId; }

	// base overrides
	virtual CSize ComputeIdealSize( void );
protected:
	virtual bool HasCustomFacet( void ) const;
	virtual void DrawBackground( CDC* pDC, const CRect& clientRect );
	virtual void Draw( CDC* pDC, const CRect& clientRect );
private:
	CRect GetCoreRect( const CRect& clientRect ) const;
	void DrawGuides( CDC* pDC, CRect coreRect, Color guideColor );
	void DrawError( CDC* pDC, const CRect& coreRect );
	bool SizeToContent( CRect& rCoreRect, CDC* pDC );
public:
	CSize m_margins;
	CSize m_coreSize;
	std::tstring m_sampleText;
	UINT m_stcInfoId;				// labels for size-to-content mode
private:
	const CThemeSampleOptions* m_pOptions;
	CThemeItem m_themeItem;

	enum { Margin = 8 };
protected:
	// message map functions
	DECLARE_MESSAGE_MAP()
};


#endif // ThemeSampleStatic_h
