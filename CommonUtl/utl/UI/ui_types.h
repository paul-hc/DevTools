#ifndef ui_types_h
#define ui_types_h
#pragma once


typedef int TPercent;				// [-100, 100]
typedef unsigned int TUPercent;		// [0, 100]
typedef int TValueOrPct;			// if negative is a percentage, otherwise a pozitive value

typedef double TFactor;				// from 0.0 to 1.0


namespace ui
{
	inline bool IsPercentage_0_100( TUPercent percentage ) { return percentage >= 0 && percentage <= 100; }
	inline bool IsPercentage_m100_p100( TPercent percentage ) { return percentage >= -100 && percentage <= 100; }

	inline TPercent GetPercentageOf( int value, int maxValue ) { return MulDiv( 100, value, maxValue ); }
	inline int ScaleValue( int value, TPercent percentage ) { return MulDiv( value, percentage, 100 ); }
	inline int AdjustValueBy( int value, TPercent byPercentage ) { return ScaleValue( value, 100 + byPercentage ); }


	// factor utils

	inline bool IsPercentage( TValueOrPct valueOrNegativePct ) { return valueOrNegativePct < 0; }
	inline int EvalValueOrPercentage( TValueOrPct valueOrNegativePct, int extent ) { return IsPercentage( valueOrNegativePct ) ? MulDiv( extent, -valueOrNegativePct, 100 ) : valueOrNegativePct; }


	// factor utils

	inline bool IsFactor_0_1( TFactor factor ) { return factor >= 0.0 && factor <= 1.0; }
	inline bool IsFactor_m1_p1( TFactor factor ) { return factor >= -1.0 && factor <= 1.0; }
	inline TFactor PercentToFactor( TPercent pct ) { ASSERT( IsPercentage_m100_p100( pct ) ); return static_cast<double>( pct ) / 100.0; }
	inline TPercent FactorToPercent( TFactor factor ) { ASSERT( IsFactor_m1_p1( factor ) ); return static_cast<TPercent>( factor * 100.0 ); }

	template< typename ValueT >
	inline ValueT AdjustValueByFactor( ValueT value, TFactor factor ) { return static_cast<ValueT>( value * ( 1.0 + factor ) ); }


	// Encalpsulates a raw value that can be either a value or a percentage (of en extent).

	union CValuePct
	{
		CValuePct( void ) { m_valuePair.m_value = m_valuePair.m_percentage = SHRT_MAX; }
		explicit CValuePct( int rawValue ) : m_rawValue( rawValue ) {}

		static CValuePct MakeValue( int value ) { CValuePct valPct; valPct.SetValue( value ); return valPct; }
		static CValuePct MakePercentage( TPercent percentage ) { CValuePct valPct; valPct.SetPercentage( percentage ); return valPct; }

		bool IsValid( void ) const { return m_valuePair.IsValid(); }

		int GetRaw( void ) const { return m_rawValue; }
		void SetRaw( int rawValue ) { m_rawValue = rawValue; }

		bool HasValue( void ) const { return CPair::IsValidField( m_valuePair.m_value ); }
		bool HasPercentage( void ) const { return CPair::IsValidField( m_valuePair.m_percentage ); }

		int GetValue( void ) const { ASSERT( HasValue() ); return m_valuePair.m_value; }
		void SetValue( int value ) { m_valuePair.SetValue( value ); }

		TPercent GetPercentage( void ) const { ASSERT( HasPercentage() ); return m_valuePair.m_percentage; }
		void SetPercentage( TPercent percentage ) { m_valuePair.SetPercentage( percentage ); }

		int EvalValue( int extent ) const;
		double EvalValue( double extent ) const;
	private:
		struct CPair
		{
			bool IsValid( void ) const { return IsValidField( m_value ) || IsValidField( m_percentage ); }
			static bool IsValidField( short field ) { return field != SHRT_MAX; }

			void SetValue( int value ) { m_value = static_cast<short>( value ); m_percentage = SHRT_MAX; }
			void SetPercentage( TPercent percentage ) { m_percentage = static_cast<short>( percentage ); m_value = SHRT_MAX; }
		public:
			short m_value;
			short m_percentage;
		};
	private:
		int m_rawValue;
		CPair m_valuePair;
	};
}


#endif // ui_types_h
