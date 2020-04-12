#ifndef StockValuesComboBox_hxx
#define StockValuesComboBox_hxx

#include "Utilities.h"


// CStockValuesComboBox< ValueT > template code

template< typename ValueT >
CStockValuesComboBox< ValueT >::CStockValuesComboBox( const ui::IValueSetAdapter< ValueT >* pStockAdapter, ui::TValueSetFlags flags /*= 0*/ )
	: CComboBox()
	, m_pStockAdapter( NULL )
	, m_flags( flags )
	, m_duringCreation( false )
{
	SetAdapter( pStockAdapter );
}

template< typename ValueT >
inline void CStockValuesComboBox< ValueT >::SetAdapter( const ui::IValueSetAdapter< ValueT >* pStockAdapter )
{
	m_pStockAdapter = pStockAdapter;
	ASSERT_PTR( m_pStockAdapter );

	const std::vector< ValueT >& stockValues = m_pStockAdapter->GetStockValues();
	if ( !stockValues.empty() )
	{
		m_validRange.m_start = *std::min_element( stockValues.begin(), stockValues.end() );
		m_validRange.m_end = *std::max_element( stockValues.begin(), stockValues.end() );
	}

	if ( m_hWnd != NULL )
		InitStockItems();
}

template< typename ValueT >
inline bool CStockValuesComboBox< ValueT >::OutputValue( ValueT value )
{
	return ui::SetComboEditText( *this, m_pStockAdapter->OutputValue( value ), str::IgnoreCase ).first;
}

template< typename ValueT >
inline bool CStockValuesComboBox< ValueT >::InputValue( ValueT* pOutValue, ui::ComboField byField ) const
{
	if ( m_pStockAdapter->ParseValue( pOutValue, ui::GetComboSelText( *this, byField ) ) )
		if ( IsValidValue( *pOutValue ) )
			return true;

	return ui::BeepSignal( MB_ICONWARNING );		// invalid input
}

template< typename ValueT >
bool CStockValuesComboBox< ValueT >::IsValidValue( ValueT value ) const
{
	const std::vector< ValueT >& stockValues = m_pStockAdapter->GetStockValues();

	if ( !stockValues.empty() )
	{
		ASSERT( !HasFlag( m_flags, ui::LimitMinValue || ui::LimitMaxValue ) || m_validRange.IsNonEmpty() );		// valid range defined?

		if ( HasFlag( m_flags, ui::LimitMinValue ) )
			if ( value < m_validRange.m_start )
				return false;		// value too small

		if ( HasFlag( m_flags, ui::LimitMaxValue ) )
			if ( value > m_validRange.m_end )
				return false;		// value too large
	}

	return true;
}

template< typename ValueT >
inline void CStockValuesComboBox< ValueT >::InitStockItems( void )
{
	ui::WriteComboItemValues( *this, m_pStockAdapter->GetStockValues(), OutputFormatter( m_pStockAdapter ) );
}

template< typename ValueT >
inline BOOL CStockValuesComboBox< ValueT >::Create( DWORD style, const RECT& rect, CWnd* pParentWnd, UINT comboId )
{
	m_duringCreation = true;

	if ( !__super::Create( style, rect, pParentWnd, comboId ) )
		return false;

	InitStockItems();		// post-creation set-up
	m_duringCreation = false;
	return true;
}

template< typename ValueT >
void CStockValuesComboBox< ValueT >::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( !m_duringCreation )
		InitStockItems();			// CComboBox::AddString() fails during creation
}


#endif // StockValuesComboBox_hxx
