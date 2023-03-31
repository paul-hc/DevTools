#ifndef StockValuesComboBox_h
#define StockValuesComboBox_h
#pragma once

#include "utl/Range.h"
#include "ui_fwd.h"


namespace ui
{
	enum ValueSetFlags
	{
		LimitMinValue	= BIT_FLAG( 0 ),
		LimitMaxValue	= BIT_FLAG( 1 )
	};

	typedef int TValueSetFlags;


	template< typename ValueT >
	interface IValueSetAdapter
	{
		typedef ValueT TValue;

		virtual const std::vector<ValueT>& GetStockValues( void ) const = 0;
		virtual std::tstring OutputValue( const ValueT& value ) const = 0;
		virtual bool ParseValue( ValueT* pOutValue, const std::tstring& text ) const = 0;
	};


	template< typename ValueT, typename DisplayValueT = ValueT >
	interface IDisplayValueSetAdapter : public IValueSetAdapter<ValueT>
	{
		typedef DisplayValueT TDisplayValue;

		// display type conversion
		virtual DisplayValueT ToDisplayValue( const ValueT& value ) const = 0;
		virtual ValueT FromDisplayValue( const DisplayValueT& displayValue ) const = 0;
	};


	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueSetAdapter<ValueT>* pAdapter, ui::TValueSetFlags flags, const Range<ValueT>& validRange );


	// timespan duration field (miliseconds) displayed as seconds (double)
	class CDurationInSecondsAdapter : public ui::IDisplayValueSetAdapter<UINT, double>		// UINT stands for __time64_t (timespan in miliseconds)
	{
	protected:
		CDurationInSecondsAdapter( void );
	public:
		typedef ui::IDisplayValueSetAdapter<UINT, double> IAdapterBase;

		static IAdapterBase* Instance( void );

		// IValueSetAdapter<UINT> interface
		virtual const std::vector<UINT>& GetStockValues( void ) const;
		virtual std::tstring OutputValue( const UINT& miliseconds ) const;
		virtual bool ParseValue( UINT* pOutMiliseconds, const std::tstring& text ) const;

		// IDisplayValueSetAdapter<UINT, double> interface
		virtual double ToDisplayValue( const UINT& miliseconds ) const;
		virtual UINT FromDisplayValue( const double& seconds ) const;
	private:
		std::vector<UINT> m_stockMiliseconds;
	public:
		static const UINT s_defaultMiliseconds[];
	};


	// percentage field (no specified value set)
	abstract class CPercentageAdapterBase : public ui::IValueSetAdapter<UINT>
	{
	public:
		typedef ui::IValueSetAdapter<UINT> IAdapterBase;

		// IValueSetAdapter<UINT> interface
		virtual std::tstring OutputValue( const UINT& zoomPct ) const;
		virtual bool ParseValue( UINT* pOutZoomPct, const std::tstring& text ) const;
	};


	// zoom percentage field (value set defined in ui::CStdZoom::m_zoomPcts)
	class CZoomPercentageAdapter : public CPercentageAdapterBase
	{
	public:
		static IAdapterBase* Instance( void );

		// IValueSetAdapter<UINT> interface
		virtual const std::vector<UINT>& GetStockValues( void ) const;
	};
}


#include "BaseStockContentCtrl.h"


// edits a formatted predefined value set augmented with custom values
template< typename ValueT >
class CStockValuesComboBox : public CBaseStockContentCtrl<CComboBox>
{
public:
	CStockValuesComboBox( const ui::IValueSetAdapter<ValueT>* pStockAdapter, ui::TValueSetFlags flags = 0 );

	const ui::IValueSetAdapter<ValueT>* GetAdapter( void ) const { return m_pStockAdapter; }
	void SetAdapter( const ui::IValueSetAdapter<ValueT>* pStockAdapter );

	ui::TValueSetFlags GetFlags( void ) const { return m_flags; }
	void SetFlags( ui::TValueSetFlags flags ) { m_flags = flags ; }

	const Range<ValueT>& GetValidRange( void ) const { return m_validRange; }
	void SetValidRange( const Range<ValueT>& validRange ) { m_validRange = validRange; }

	// input/output
	bool OutputValue( ValueT value );
	bool InputValue( ValueT* pOutValue, ui::ComboField byField, bool showErrors = false ) const;

	bool IsValidValue( ValueT value ) const;

	void DDX_Value( CDataExchange* pDX, ValueT& rValue, int comboId );
	std::tstring FormatValidationError( void ) const;
protected:
	// base overrides
	virtual void InitStockContent( void );

	struct OutputFormatter
	{
		OutputFormatter( const ui::IValueSetAdapter<ValueT>* pStockAdapter ) : m_pStockAdapter( pStockAdapter ) { ASSERT_PTR( m_pStockAdapter ); }

		std::tstring operator()( ValueT value ) const { return m_pStockAdapter->OutputValue( value ); }
	private:
		const ui::IValueSetAdapter<ValueT>* m_pStockAdapter;
	};

	// generated stuff
private:
	const ui::IValueSetAdapter<ValueT>* m_pStockAdapter;
	ui::TValueSetFlags m_flags;
	Range<ValueT> m_validRange;
};


// edits a timespan duration field (miliseconds) displayed as seconds (double)
class CDurationComboBox : public CStockValuesComboBox<UINT>
{
public:
	CDurationComboBox( ui::TValueSetFlags flags = ui::LimitMinValue, const ui::IValueSetAdapter<UINT>* pStockAdapter = ui::CDurationInSecondsAdapter::Instance() );
};


// edits a zoom percentage field
class CZoomComboBox : public CStockValuesComboBox<UINT>
{
public:
	CZoomComboBox( ui::TValueSetFlags flags = ui::LimitMinValue | ui::LimitMaxValue,
				   const ui::IValueSetAdapter<UINT>* pStockAdapter = ui::CZoomPercentageAdapter::Instance() );
};


#endif // StockValuesComboBox_h
