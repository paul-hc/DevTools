#ifndef ThemeSampleStatic_h
#define ThemeSampleStatic_h
#pragma once

#include "utl/BufferedStatic.h"
#include "utl/ThemeItem.h"


class COptions;


class CThemeSampleStatic : public CBufferedStatic
{
public:
	CThemeSampleStatic( void );
	virtual ~CThemeSampleStatic();

	void SetOptions( const COptions* pOptions ) { m_pOptions = pOptions; }
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
public:
	CSize m_margins;
	CSize m_coreSize;
	std::tstring m_sampleText;
	UINT m_stcInfoId;				// labels for size-to-content mode
private:
	const COptions* m_pOptions;
	CThemeItem m_themeItem;

	enum { Margin = 8 };

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // ThemeSampleStatic_h
