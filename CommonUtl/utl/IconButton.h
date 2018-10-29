#ifndef IconButton_h
#define IconButton_h
#pragma once

#include "Image_fwd.h"


class CIcon;


class CIconButton : public CButton
{
public:
	CIconButton( const CIconId& iconId = CIconId(), bool useText = true );
	virtual ~CIconButton();

	const CIcon* GetIconPtr( void ) const;
	const CIconId& GetIconId( void ) const { return m_iconId; }
	void SetIconId( const CIconId& iconId );

	void SetUseText( bool useText = true ) { m_useText = useText; }
	void SetUseTextSpacing( bool useTextSpacing = true );

	std::tstring GetButtonCaption( void ) const;
	void SetButtonCaption( const std::tstring& caption );

	static void SetButtonIcon( CButton* pButton, const CIconId& iconId, bool useText = true, bool useTextSpacing = true );
protected:
	static std::tstring CaptionToText( const std::tstring& caption, bool useText, bool useTextSpacing );
	static std::tstring TextToCaption( const std::tstring& text, bool useText, bool useTextSpacing );

	bool UpdateIcon( void );
	static bool UpdateCaption( CButton* pButton, bool useText, bool useTextSpacing );
private:
	CIconId m_iconId;
	bool m_useText;
	bool m_useTextSpacing;				// compensate for BS_LEFT/BS_RIGHT hard text alignment

	static const TCHAR s_textSpacing[];
public:
	// generated overrides
	virtual void PreSubclassWindow( void );
};


#endif // IconButton_h
