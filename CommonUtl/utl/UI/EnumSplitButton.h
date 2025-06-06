#ifndef EnumSplitButton_h
#define EnumSplitButton_h
#pragma once

#include "PopupSplitButton.h"
#include "Dialog_fwd.h"


class CEnumTags;


class CEnumSplitButton : public CPopupSplitButton
	, public ui::ICustomCmdInfo
{
public:
	CEnumSplitButton( const CEnumTags* pEnumTags );
	virtual ~CEnumSplitButton();

	const CEnumTags* GetEnumTags( void ) const { return m_pEnumTags; }
	void SetEnumTags( const CEnumTags* pEnumTags );

	int GetSelValue( void ) const { return m_selValue; }
	void SetSelValue( int selValue );

	template< typename EnumType >
	EnumType GetSelEnum( void ) const
	{
		return static_cast<EnumType>( GetSelValue() );
	}
private:
	enum { IdFirstCommand = 1000, IdLastCommand = IdFirstCommand + 100 };

	const CEnumTags* m_pEnumTags;
	int m_selValue;
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	// generated overrides
	public:
	virtual void PreSubclassWindow( void );
protected:
	// message map functions
	afx_msg void OnSelectValue( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // EnumSplitButton_h
