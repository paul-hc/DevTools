#ifndef ThemeSampleStatic_h
#define ThemeSampleStatic_h
#pragma once

#include "utl/BufferedStatic.h"
#include "utl/GpUtilities.h"
#include "utl/RegistryOptions.h"
#include "utl/ThemeItem.h"


class CHistoryComboBox;


interface ISampleOptionsCallback
{
	virtual CWnd* GetWnd( void ) = 0;
	virtual void RedrawSamples( void ) = 0;
};


struct CThemeSampleOptions : public CRegistryOptions
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
	bool m_showThemeGlyphs;

	// external options
	bool& m_enableThemes;
	bool& m_enableThemesFallback;
protected:
	// base overrides
	virtual void OnOptionChanged( const void* pDataMember );

	// generated stuff
protected:
	afx_msg void OnChange_BkColor( void );

	DECLARE_MESSAGE_MAP()
};


class CThemeSampleStatic : public CBufferedStatic
{
public:
	CThemeSampleStatic( void );
	virtual ~CThemeSampleStatic();

	void SetOptions( const CThemeSampleOptions* pOptions ) { m_pOptions = pOptions; }
	void SetThemeItem( const CThemeItem& themeItem );

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
	bool SizeToContent( CRect& rCoreRect, CDC* pDC );

	static void DrawGuides( CDC* pDC, CRect coreRect, Color guideColor );
	static void DrawError( CDC* pDC, const CRect& coreRect );
public:
	CSize m_margins;
	CSize m_coreSize;
	std::tstring m_sampleText;
	UINT m_stcInfoId;				// labels for size-to-content mode
private:
	const CThemeSampleOptions* m_pOptions;
	CThemeItem m_themeItem;

	enum { Margin = 8 };

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // ThemeSampleStatic_h
