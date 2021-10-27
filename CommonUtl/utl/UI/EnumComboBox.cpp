
#include "stdafx.h"
#include "EnumComboBox.h"
#include "Utilities.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "BaseStockContentCtrl.hxx"


// maxEnumValue is used to display fewer enumerations than defined in the enum formatter

CEnumComboBox::CEnumComboBox( const CEnumTags* pEnumTags )
	: CBaseStockContentCtrl<CComboBox>()
	, m_pEnumTags( pEnumTags )
{
	ASSERT_PTR( m_pEnumTags );
}

CEnumComboBox::~CEnumComboBox()
{
}

void CEnumComboBox::InitStockContent( void )
{
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
