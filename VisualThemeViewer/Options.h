#ifndef Options_h
#define Options_h
#pragma once

#include "utl/RegistryOptions.h"


class CHistoryComboBox;


interface ISampleOptionsCallback
{
	virtual CWnd* GetWnd( void ) = 0;
	virtual void RedrawSamples( void ) = 0;
	virtual void UpdateGlyphPreview( void ) = 0;
	virtual void UpdateExplorerTheme( void ) = 0;
};


class COptions : public CRegistryOptions
{
public:
	COptions( ISampleOptionsCallback* pCallback );
	~COptions();

	COLORREF GetBkColor( void ) const;
private:
	CHistoryComboBox* GetBkColorCombo( void ) const;
public:
	ISampleOptionsCallback* m_pCallback;
	std::tstring m_bkColorText;
	bool m_useBorder;
	bool m_preBkGuides;
	bool m_postBkGuides;
	bool m_previewThemeGlyphs;
	bool m_useExplorerTheme;

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


#endif // Options_h
