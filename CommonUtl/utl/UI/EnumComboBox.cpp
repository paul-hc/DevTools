
#include "pch.h"
#include "EnumComboBox.h"
#include "WndUtils.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "BaseStockContentCtrl.hxx"


// maxEnumValue is used to display fewer enumerations than defined in the enum formatter

CEnumComboBox::CEnumComboBox( const CEnumTags* pEnumTags /*= nullptr*/ )
	: CBaseStockContentCtrl<CComboBox>()
	, m_pEnumTags( pEnumTags )
{
}

CEnumComboBox::~CEnumComboBox()
{
}

void CEnumComboBox::InitStockContent( void ) override
{
	ASSERT_PTR( m_pEnumTags );
	ASSERT( !HasFlag( GetStyle(), CBS_SORT ) );			// auto-sorting combos don't work with enumerations

	ui::WriteComboItems( *this, m_pEnumTags->GetUiTags() );
}

bool CEnumComboBox::IsValidValue( int value ) const
{
	ASSERT_PTR( m_pEnumTags );
	return value >= 0 && value < (int)m_pEnumTags->GetUiTags().size();
}

int CEnumComboBox::GetValue( void ) const
{
	int selIndex = GetCurSel();
	if ( CB_ERR == selIndex )
	{
		ASSERT( HasValidValue() );
		return CB_ERR;
	}

	return m_pEnumTags->GetSelValue<int>( selIndex );
}

bool CEnumComboBox::SetValue( int value )
{
	if ( HasValidValue() && value == GetValue() )
		return false;

	return SetCurSel( m_pEnumTags->GetTagIndex( value ) ) != CB_ERR;
}

void CEnumComboBox::PreSubclassWindow( void )
{
	ASSERT_PTR( m_pEnumTags );
	__super::PreSubclassWindow();
}
