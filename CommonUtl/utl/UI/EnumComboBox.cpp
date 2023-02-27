
#include "stdafx.h"
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

int CEnumComboBox::GetValue( void ) const
{
	int selIndex = GetCurSel();
	if ( -1 == selIndex )
	{
		ASSERT( HasValidValue() );
		return -1;
	}

	return m_pEnumTags->GetSelValue< int >( selIndex );
}

bool CEnumComboBox::SetValue( int value )
{
	if ( HasValidValue() && value == GetValue() )
		return false;

	VERIFY( SetCurSel( m_pEnumTags->GetTagIndex( value ) ) != CB_ERR );
	return true;
}

void CEnumComboBox::PreSubclassWindow( void )
{
	ASSERT_PTR( m_pEnumTags );
	__super::PreSubclassWindow();
}
