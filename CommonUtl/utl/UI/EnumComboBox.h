#ifndef EnumComboBox_h
#define EnumComboBox_h
#pragma once

#include "BaseStockContentCtrl.h"


class CEnumTags;


// works with enum types that have contiguous values, not necessarily zero based
//
class CEnumComboBox : public CBaseStockContentCtrl<CComboBox>
{
public:
	CEnumComboBox( const CEnumTags* pEnumTags = nullptr );
	virtual ~CEnumComboBox();

	template< typename EnumType >
	void DDX_EnumValue( CDataExchange* pDX, int ctrlId, EnumType& rValue );

	bool HasValidValue( void ) const { return GetCurSel() != -1; }
	int GetValue( void ) const;
	bool SetValue( int value );

	template< typename EnumType >
	EnumType GetEnum( void ) const { return (EnumType)GetValue(); }

	template<>
	bool GetEnum<bool>( void ) const { return GetValue() != 0; }

	const CEnumTags* GetTags( void ) const { return m_pEnumTags; }
	void SetTags( const CEnumTags* pEnumTags ) { m_pEnumTags = pEnumTags; }
protected:
	// hidden methods (use specialized getters & setters)
	using CComboBox::GetCurSel;
	using CComboBox::SetCurSel;
protected:
	// base overrides
	virtual void InitStockContent( void ) override;
private:
	const CEnumTags* m_pEnumTags;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
};


#include "Dialog_fwd.h"


// template code

template< typename EnumType >
void CEnumComboBox::DDX_EnumValue( CDataExchange* pDX, int ctrlId, EnumType& rValue )
{
	DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetValue( rValue );
	else
		rValue = GetEnum<EnumType>();
}


#endif // EnumComboBox_h
