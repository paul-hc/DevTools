#ifndef DataAdapters_h
#define DataAdapters_h
#pragma once

#include "DataAdapters_fwd.h"


namespace ui
{
	interface IValueEditorHost
	{
		virtual void OutputValue( void ) = 0;
		virtual void OnInputError( void ) = 0;
	};


	interface IValueTags			// implemented by objects that manage an editable value (typically numeric), with a data set of stock values (combo-box editable)
	{
		virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const = 0;
		virtual std::tstring GetTag( void ) const = 0;				// for output formatting
		virtual bool StoreTag( const std::tstring& tag ) = 0;		// for input parsing

		virtual void StoreEditorHost( ui::IValueEditorHost* pEditorHost ) = 0;
	};


	template< typename ValueT >
	interface IValueAdapter			// formats and parses numeric values to/from string
	{
		virtual std::tstring FormatValue( const ValueT& value ) const = 0;
		virtual bool ParseValue( OUT ValueT* pOutValue, const std::tstring& text ) const = 0;
	};


	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueAdapter<ValueT>* pAdapter, ui::TStockValueFlags flags, const Range<ValueT>& validRange );
}


// WndUtils.h forward declarations

namespace ui
{
	bool ShowInputError( CWnd* pCtrl, const std::tstring& message, UINT iconFlag /*= MB_ICONERROR*/ );
}

// StringUtilities.h forward declarations

namespace num
{
	const std::locale& GetEmptyLocale( void );

	template< typename ValueT >
	std::tstring FormatNumber( ValueT value, const std::locale& loc /*= GetEmptyLocale()*/ );

	template< typename ValueT >
	bool ParseNumber( OUT ValueT& rNumber, const std::tstring& text, size_t* pSkipLength /*= nullptr*/, const std::locale& loc /*= GetEmptyLocale()*/ );

	template< typename ValueT >
	ValueT MinValue( void );

	template< typename ValueT >
	ValueT MaxValue( void );

	template< typename ValueT >		// [0, MAX]
	Range<ValueT> PositiveRange( void );
}


namespace ui
{
	template< typename NumT >
	class CNumericAdapter : public ui::IValueAdapter<NumT>
	{
	public:
		CNumericAdapter( const std::locale& loc = num::GetEmptyLocale() ) : m_loc( loc ) {}

		// ui::IValueAdapter<NumT> interface
		virtual std::tstring FormatValue( const NumT& value ) const implement { return num::FormatNumber( value, m_loc ); }
		virtual bool ParseValue( OUT NumT* pOutValue, const std::tstring& text ) const implement { return num::ParseNumber( *pOutValue, text, nullptr, m_loc ); }
	protected:
		const std::locale& m_loc;
	};


	template< typename NumT >
	class CPercentageAdapter : public CNumericAdapter<NumT>
	{
	public:
		CPercentageAdapter( const TCHAR* pSuffix = _T(" %") )
			: CNumericAdapter()
			, m_pSuffix( pSuffix )
		{
		}

		// base overrides
		virtual std::tstring FormatValue( const NumT& value ) const override { return num::FormatNumber( value, this->m_loc ) + m_pSuffix; }
	private:
		const TCHAR* m_pSuffix;
	};


	class CPositivePercentageAdapter : public CPercentageAdapter<UINT>
	{
		CPositivePercentageAdapter( void );
	public:
		static const CPositivePercentageAdapter* Instance( void );
	};
}


namespace ui
{
	template< typename ValueT >
	class CStockValues : public IValueTags		// stores an editable value, as well as the stock values (for combo-box items)
	{
	public:
		CStockValues( IValueAdapter<ValueT> pAdapter, ValueT value, const TCHAR* pStockTags );							// string list stock values separated by '|'
		CStockValues( IValueAdapter<ValueT> pAdapter, ValueT value, const ValueT* pStockValues, size_t stockCount );	// array stock values

		ValueT GetValue( void ) const { return m_value; }
		void SetValue( ValueT value );

		ui::TStockValueFlags GetFlags( void ) const { return m_flags; }
		void SetFlags( ui::TStockValueFlags flags ) { m_flags = flags ; }

		const Range<ValueT>& GetValidRange( void ) const { return m_validRange; }
		void SetValidRange( const Range<ValueT>& validRange ) { m_validRange = validRange; }

		bool IsValidValue( ValueT value ) const;

		// IValueTags interface
		virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const implement;
		virtual std::tstring GetTag( void ) const implement { return m_pAdapter->FormatValue( m_value ); }
		virtual bool StoreTag( const std::tstring& tag ) implement;
		virtual void StoreEditorHost( ui::IValueEditorHost* pEditorHost ) implement { m_pEditorHost = pEditorHost; }

		void SplitStockTags( const TCHAR* pStockTags );
	private:
		void InitValidRange( void );
		std::tstring FormatValidationError( void ) const;
	private:
		IValueAdapter<ValueT> m_pAdapter;
		ValueT m_value;
		std::vector<ValueT> m_stockValues;

		ui::TStockValueFlags m_flags;
		Range<ValueT> m_validRange;

		ui::IValueEditorHost* m_pEditorHost;
	};
}


namespace ui
{
	class CZoomStockValues
	{
	public:
		CZoomStockValues()
		{
		}
	private:
	};

}


namespace ui
{
	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueAdapter<ValueT>* pAdapter, ui::TStockValueFlags flags, const Range<ValueT>& validRange )
	{
		ASSERT_PTR( pAdapter );

		if ( HasFlag( flags, ui::LimitMinValue ) && HasFlag( flags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must be between %s and %s!"),
								pAdapter->FormatValue( validRange.m_start ).c_str(),
								pAdapter->FormatValue( validRange.m_end ).c_str() );
		else if ( HasFlag( flags, ui::LimitMinValue ) )
			return str::Format( _T("Value must not be less than %s!"), pAdapter->FormatValue( validRange.m_start ).c_str() );
		else if ( HasFlag( flags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must not be greater than %s!"), pAdapter->FormatValue( validRange.m_end ).c_str() );

		return _T("You must input a valid value!");
	}


	// CStockValues template code

	template< typename ValueT >
	CStockValues<ValueT>::CStockValues( IValueAdapter<ValueT> pAdapter, ValueT value, const TCHAR* pStockTags )
		: m_pAdapter( pAdapter )
		, m_value( value )
		, m_flags( ui::LimitMinValue | ui::LimitMaxValue )
		, m_pEditorHost( nullptr )
	{
		SplitStockTags( pStockTags );
		InitValidRange();
	}

	template< typename ValueT >
	CStockValues<ValueT>::CStockValues( IValueAdapter<ValueT> pAdapter, ValueT value, const ValueT* pStockValues, size_t stockCount )
		: m_pAdapter( pAdapter )
		, m_value( value )
		, m_stockValues( pStockValues, pStockValues + stockCount )
		, m_flags( ui::LimitMinValue | ui::LimitMaxValue )
		, m_pEditorHost( nullptr )
	{
		InitValidRange();
	}

	template< typename ValueT >
	void CStockValues<ValueT>::SetValue( ValueT value )
	{
		m_value = value;

		if ( m_pEditorHost != nullptr )
			m_pEditorHost->OutputValue();		// the host will call GetValue() to update the control or button state
	}

	template< typename ValueT >
	void CStockValues<ValueT>::QueryStockTags( OUT std::vector<std::tstring>& rTags ) const implement
	{
		rTags.reserve( m_stockValues.size() );
		for ( typename std::vector<ValueT>::const_iterator itValue = m_stockValues.begin(); itValue != m_stockValues.end(); ++itValue )
			rTags.push_back( m_pAdapter->FormatValue( *itValue ) );
	}

	template<typename ValueT>
	bool CStockValues<ValueT>::StoreTag( const std::tstring& tag )
	{
		if ( m_pAdapter->ParseValue( &m_value, tag ) )
			if ( IsValidValue( m_value ) )
				return true;

		if ( m_pEditorHost != nullptr )
			m_pEditorHost->OnInputError();		// give owner a chance to restore previous valid value

		//if ( showErrors )
		return ui::ShowInputError( (CWnd*)this, FormatValidationError(), MB_ICONERROR );		// invalid input
	}

	template< typename ValueT >
	void CStockValues<ValueT>::SplitStockTags( const TCHAR* pStockTags )
	{
		std::vector<std::tstring> stockTags;
		str::Split( stockTags, pStockTags, _T("|") );

		m_stockValues.reserve( stockTags.size() );
		for ( std::vector<std::tstring>::const_iterator itTag = stockTags.begin(); itTag != stockTags.end(); ++itTag )
		{
			ValueT value;

			if ( m_pAdapter->ParseValue( &value, *itTag ) )
				m_stockValues.push_back( value );
			else
				ASSERT( false );		// parsing error in pStockTags?
		}
	}

	template< typename ValueT >
	void CStockValues<ValueT>::InitValidRange( void )
	{
		if ( !m_stockValues.empty() )
		{
			m_validRange.m_start = *std::min_element( m_stockValues.begin(), m_stockValues.end() );
			m_validRange.m_end = *std::max_element( m_stockValues.begin(), m_stockValues.end() );
		}
		else
			m_validRange = num::PositiveRange();
	}

	template< typename ValueT >
	inline std::tstring CStockValues<ValueT>::FormatValidationError( void ) const
	{
		return ui::FormatValidationMessage( m_pAdapter, m_flags, m_validRange );
	}

	template< typename ValueT >
	bool CStockValues<ValueT>::IsValidValue( ValueT value ) const
	{
		if ( !m_stockValues.empty() )
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
}


#endif // DataAdapters_h
