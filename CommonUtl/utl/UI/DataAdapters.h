#ifndef DataAdapters_h
#define DataAdapters_h
#pragma once

#include "DataAdapters_fwd.h"


namespace ui
{
	interface IStockTags			// implemented by objects that manage an editable value (typically numeric), with a data set of stock values (combo-box editable)
	{
		virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const = 0;
	};


	template< typename ValueT >
	interface IValueAdapter			// formats and parses numeric values to/from string
	{
		virtual std::tstring FormatValue( ValueT value ) const = 0;
		virtual bool ParseValue( OUT ValueT* pOutValue, const std::tstring& text ) const = 0;
	};


	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueAdapter<ValueT>* pAdapter, ui::TValueLimitFlags limitFlags, const Range<ValueT>& limits );
}


namespace ui
{
	template< typename ValueT >
	class CStockTags : public IStockTags		// stores an editable value, as well as the stock values (for combo-box items)
	{
	public:
		CStockTags( const IValueAdapter<ValueT>* pAdapter, const TCHAR* pStockTags );						// string list stock values separated by '|'
		CStockTags( const IValueAdapter<ValueT>* pAdapter, const ValueT* pStockTags, size_t stockCount );	// array stock values

		const std::vector<ValueT>& GetStockValues( void ) const { return m_stockValues; }

		const Range<ValueT>& GetLimits( void ) const { return m_limits; }
		void SetLimits( const Range<ValueT>& limits, ui::TValueLimitFlags limitFlags ) { m_limits = limits; m_limitFlags = limitFlags; }

		// value input/output
		std::tstring FormatValue( ValueT value ) const { return m_pAdapter->FormatValue( value ); }
		bool ParseValue( OUT ValueT* pOutValue, const std::tstring& tag ) const { ASSERT_PTR( pOutValue ); return m_pAdapter->ParseValue( pOutValue, tag ); }

		bool IsValidValue( ValueT value ) const;
		std::tstring FormatValidationError( void ) const;

		// IStockTags interface
		virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const implement;
	private:
		void SplitStockTags( const TCHAR* pStockTags );
		void InitLimits( void );
	private:
		const IValueAdapter<ValueT>* m_pAdapter;
		std::vector<ValueT> m_stockValues;

		ui::TValueLimitFlags m_limitFlags;
		Range<ValueT> m_limits;
	};
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
		virtual std::tstring FormatValue( NumT value ) const implement { return num::FormatNumber( value, m_loc ); }
		virtual bool ParseValue( OUT NumT* pOutValue, const std::tstring& text ) const implement { return num::ParseNumber( *pOutValue, text, nullptr, m_loc ); }
	protected:
		const std::locale& m_loc;
	};


	template< typename NumT >
	class CPercentageAdapter : public CNumericAdapter<NumT>
	{
	public:
		CPercentageAdapter( const TCHAR* pSuffix = _T(" %") )
			: CNumericAdapter<NumT>()
			, m_pSuffix( pSuffix )
		{
		}

		// base overrides
		virtual std::tstring FormatValue( NumT value ) const override { return num::FormatNumber( value, this->m_loc ) + m_pSuffix; }
	private:
		const TCHAR* m_pSuffix;
	};


	class CPositivePercentageAdapter : public CPercentageAdapter<UINT>
	{
		CPositivePercentageAdapter( void ) {}
	public:
		static const CPositivePercentageAdapter* Instance( void );
	};
}


namespace ui
{
	// special purpose stock tags

	class CZoomStockTags : public CStockTags<UINT>
	{
		CZoomStockTags( void );

		enum ZoomLimits { MinZoomPct = 1, MaxZoomPct = 10000 };		// access via GetLimits()
	public:
		static const CZoomStockTags* Instance( void );
	};
}


namespace ui
{
	template< typename ValueT >
	std::tstring FormatValidationMessage( const IValueAdapter<ValueT>* pAdapter, ui::TValueLimitFlags limitFlags, const Range<ValueT>& limits )
	{
		ASSERT_PTR( pAdapter );

		if ( HasFlag( limitFlags, ui::LimitMinValue ) && HasFlag( limitFlags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must be between %s and %s!"),
								pAdapter->FormatValue( limits.m_start ).c_str(),
								pAdapter->FormatValue( limits.m_end ).c_str() );
		else if ( HasFlag( limitFlags, ui::LimitMinValue ) )
			return str::Format( _T("Value must not be less than %s!"), pAdapter->FormatValue( limits.m_start ).c_str() );
		else if ( HasFlag( limitFlags, ui::LimitMaxValue ) )
			return str::Format( _T("Value must not be greater than %s!"), pAdapter->FormatValue( limits.m_end ).c_str() );

		return _T("You must input a valid value!");
	}


	// CStockTags template code

	template< typename ValueT >
	CStockTags<ValueT>::CStockTags( const IValueAdapter<ValueT>* pAdapter, const TCHAR* pStockTags )
		: m_pAdapter( pAdapter )
		, m_limitFlags( 0 )
	{
		SplitStockTags( pStockTags );
		InitLimits();
	}

	template< typename ValueT >
	CStockTags<ValueT>::CStockTags( const IValueAdapter<ValueT>* pAdapter, const ValueT* pStockTags, size_t stockCount )
		: m_pAdapter( pAdapter )
		, m_stockValues( pStockTags, pStockTags + stockCount )
		, m_limitFlags( 0 )
	{
		InitLimits();
	}

	template< typename ValueT >
	void CStockTags<ValueT>::QueryStockTags( OUT std::vector<std::tstring>& rTags ) const implement
	{
		rTags.reserve( m_stockValues.size() );
		for ( typename std::vector<ValueT>::const_iterator itValue = m_stockValues.begin(); itValue != m_stockValues.end(); ++itValue )
			rTags.push_back( m_pAdapter->FormatValue( *itValue ) );
	}

	template< typename ValueT >
	bool CStockTags<ValueT>::IsValidValue( ValueT value ) const
	{
		if ( !m_stockValues.empty() )
		{
			ASSERT( !HasFlag( m_limitFlags, ui::LimitMinValue | ui::LimitMaxValue ) || m_limits.IsNonEmpty() );		// valid range defined?

			if ( HasFlag( m_limitFlags, ui::LimitMinValue ) )
				if ( value < m_limits.m_start )
					return false;		// value too small

			if ( HasFlag( m_limitFlags, ui::LimitMaxValue ) )
				if ( value > m_limits.m_end )
					return false;		// value too large
		}

		return true;
	}

	template< typename ValueT >
	inline std::tstring CStockTags<ValueT>::FormatValidationError( void ) const
	{
		return ui::FormatValidationMessage( m_pAdapter, m_limitFlags, m_limits );
	}

	template< typename ValueT >
	void CStockTags<ValueT>::SplitStockTags( const TCHAR* pStockTags )
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
	void CStockTags<ValueT>::InitLimits( void )
	{
		if ( !m_stockValues.empty() )
		{
			SetLimits( Range<ValueT>( *std::min_element( m_stockValues.begin(), m_stockValues.end() ),
									  *std::max_element( m_stockValues.begin(), m_stockValues.end() ) ),
					   ui::LimitMinValue | ui::LimitMaxValue );
		}
		else
			m_limits = num::PositiveRange<ValueT>();
	}
}


#endif // DataAdapters_h
