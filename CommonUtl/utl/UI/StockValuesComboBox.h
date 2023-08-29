#ifndef StockValuesComboBox_h
#define StockValuesComboBox_h
#pragma once

#include "ui_fwd.h"
#include "DataAdapters.h"
#include "BaseStockContentCtrl.h"


// edits a formatted predefined value set, augmented with custom values entered by user

class CStockValuesComboBox : public CBaseStockContentCtrl<CComboBox>
{
public:
	CStockValuesComboBox( const ui::IStockTags* pStockTags );

	const ui::IStockTags* GetTags( void ) const { return m_pStockTags; }
	void SetTags( const ui::IStockTags* pStockTags );

	// input/output
	template< typename ValueT >
	bool OutputValue( ValueT value );

	template< typename ValueT >
	bool InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors = true ) const;		// ui::BySel for CBN_SELCHANGE, ui::ByEdit for CBN_EDITCHANGE

	template< typename ValueT >
	void DDX_Value( CDataExchange* pDX, ValueT& rValue, int comboId );
protected:
	// base overrides
	virtual void InitStockContent( void ) override;

	template< typename ValueT >
	typename const ui::CStockTags<ValueT>* GetTagsAs( void ) const { return checked_static_cast< const ui::CStockTags<ValueT>* >( m_pStockTags ); }

	bool OutputTag( const std::tstring& tag );
	void OnInputError( void ) const;
private:
	const ui::IStockTags* m_pStockTags;
};


// edits a timespan duration field (miliseconds) displayed as seconds (double)
class CDurationComboBox : public CStockValuesComboBox
{
public:
	CDurationComboBox( const ui::IStockTags* pStockTags = ui::CDurationSecondsStockTags::Instance() );

	bool OutputMiliseconds( UINT durationMs );
	bool InputMiliseconds( OUT UINT* pDurationMs, ui::ComboField byField, bool showErrors = false ) const;

	void DDX_Miliseconds( CDataExchange* pDX, IN OUT UINT& rDurationMs, int comboId );
};


// edits a zoom percentage field
class CZoomComboBox : public CStockValuesComboBox
{
public:
	CZoomComboBox( const ui::IStockTags* pStockTags = ui::CZoomStockTags::Instance() );
};


namespace ui
{
	// WndUtils.h forward declarations

	std::tstring GetComboSelText( const CComboBox& rCombo, ui::ComboField byField /*= ui::BySel*/ );
}


#include "Dialog_fwd.h"


// CStockValuesComboBox template code

template< typename ValueT >
bool CStockValuesComboBox::OutputValue( ValueT value )
{
	if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
		return OutputTag( pStockTags->FormatValue( value ) );

	ASSERT( false );
	return false;
}

template< typename ValueT >
bool CStockValuesComboBox::InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors /*= true*/ ) const
{
	if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
	{
		std::tstring currentTag = ui::GetComboSelText( *this, byField );

		if ( currentTag.empty() )
			return false;			// transient empty text, e.g. when the combo list is still dropped down, etc

		if ( pStockTags->ParseValue( pOutValue, currentTag ) )
			if ( pStockTags->IsValidValue( *pOutValue ) )
				return true;

		OnInputError();

		if ( showErrors )
			return ui::ShowInputError( const_cast<CStockValuesComboBox*>( this ), pStockTags->FormatValidationError(), MB_ICONERROR );		// invalid input
	}
	else
		ASSERT( false );

	return false;
}

template< typename ValueT >
void CStockValuesComboBox::DDX_Value( CDataExchange* pDX, ValueT& rValue, int comboId )
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


#endif // StockValuesComboBox_h
