#ifndef StockValuesComboBox_hxx
#define StockValuesComboBox_hxx

#include "Dialog_fwd.h"
#include "WndUtils.h"


namespace ui
{
	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueSetAdapter<ValueT>* pAdapter, ui::TValueLimitFlags flags, const Range<ValueT>& validRange )
	{
		ASSERT_PTR( pAdapter );

		if ( HasFlag( flags, ui::LimitMinValue ) && HasFlag( flags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must be between %s and %s!"),
				pAdapter->OutputValue( validRange.m_start ).c_str(),
				pAdapter->OutputValue( validRange.m_end ).c_str() );
		else if ( HasFlag( flags, ui::LimitMinValue ) )
			return str::Format( _T("Value must not be less than %s!"), pAdapter->OutputValue( validRange.m_start ).c_str() );
		else if ( HasFlag( flags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must not be greater than %s!"), pAdapter->OutputValue( validRange.m_end ).c_str() );

		return _T("You must input a valid value!");
	}
}


// CStockValuesComboBox template code

template< typename ValueT >
CStockValuesComboBox<ValueT>::CStockValuesComboBox( const ui::IValueSetAdapter<ValueT>* pStockAdapter, ui::TValueLimitFlags flags /*= 0*/ )
	: CBaseStockContentCtrl<CComboBox>()
	, m_pStockAdapter( nullptr )
	, m_flags( flags )
{
	SetAdapter( pStockAdapter );
}

template< typename ValueT >
inline void CStockValuesComboBox<ValueT>::SetAdapter( const ui::IValueSetAdapter<ValueT>* pStockAdapter )
{
	m_pStockAdapter = pStockAdapter;
	ASSERT_PTR( m_pStockAdapter );

	const std::vector<ValueT>& stockValues = m_pStockAdapter->GetStockValues();
	if ( !stockValues.empty() )
	{
		m_validRange.m_start = *std::min_element( stockValues.begin(), stockValues.end() );
		m_validRange.m_end = *std::max_element( stockValues.begin(), stockValues.end() );
	}

	if ( m_hWnd != nullptr )
		InitStockContent();
}

template< typename ValueT >
inline bool CStockValuesComboBox<ValueT>::OutputValue( ValueT value )
{
	bool changed = ui::SetComboEditText( *this, m_pStockAdapter->OutputValue( value ), str::IgnoreCase ).first;

	if ( ui::OwnsFocus( m_hWnd ) )
		SetEditSel( 0, -1 );
	return changed;
}

template< typename ValueT >
inline bool CStockValuesComboBox<ValueT>::InputValue( ValueT* pOutValue, ui::ComboField byField, bool showErrors /*= false*/ ) const
{
	if ( m_pStockAdapter->ParseValue( pOutValue, ui::GetComboSelText( *this, byField ) ) )
		if ( IsValidValue( *pOutValue ) )
			return true;

	ui::PostCommandToParent( m_hWnd, ui::CN_INPUTERROR );		// give parent a chance to restore previous valid value (posted since notifications are disabled during DDX)

	if ( showErrors )
		return ui::ShowInputError( (CWnd*)this, FormatValidationError() );		// invalid delay

	return false;				// invalid input
}

template< typename ValueT >
bool CStockValuesComboBox<ValueT>::IsValidValue( ValueT value ) const
{
	const std::vector<ValueT>& stockValues = m_pStockAdapter->GetStockValues();

	if ( !stockValues.empty() )
	{
		ASSERT( !HasFlag( m_flags, ui::LimitMinValue | ui::LimitMaxValue ) || m_validRange.IsNonEmpty() );		// valid range defined?

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
void CStockValuesComboBox<ValueT>::InitStockContent( void )
{
	ui::WriteComboItemValues( *this, m_pStockAdapter->GetStockValues(), OutputFormatter( m_pStockAdapter ) );
}

template< typename ValueT >
inline std::tstring CStockValuesComboBox<ValueT>::FormatValidationError( void ) const
{
	return ui::FormatValidationMessage( m_pStockAdapter, m_flags, m_validRange );
}

template< typename ValueT >
void CStockValuesComboBox<ValueT>::DDX_Value( CDataExchange* pDX, ValueT& rValue, int comboId )
{
	DDX_Control( pDX, comboId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputValue( rValue );
	else
	{
		if ( !InputValue( &rValue, ui::ByEdit, true ) )
			pDX->Fail();
	}
}


#endif // StockValuesComboBox_hxx
