#ifndef Endianness_h
#define Endianness_h
#pragma once


namespace endian
{
	// manage endian-ness conversions for text encodings

	namespace impl
	{
		template< size_t size >
		struct ToBytesSwapped
		{
			template< typename NumericT >
			NumericT operator()( NumericT value ) const
			{
				throw std::out_of_range( str::Format( "Unknown numeric data size: %d", size ) );
			}
		};

		template<>
		struct ToBytesSwapped< 1 >
		{
			template< typename NumericT >
			NumericT operator()( NumericT value ) const
			{
				return value;
			}
		};

		template<>
		struct ToBytesSwapped< 2 >
		{
			template< typename NumericT >
			NumericT operator()( NumericT value ) const
			{
				return
					( (value >> 8) & 0xFF ) |
					( (value & 0xFF) << 8 );
			}
		};

		template<>
		struct ToBytesSwapped< 4 >
		{
			template< typename NumericT >
			NumericT operator()( NumericT value ) const
			{
				return
					( (value & 0xFF000000) >> 24 ) |
					( (value & 0x00FF0000) >>  8 ) |
					( (value & 0x0000FF00) <<  8 ) |
					( (value & 0x000000FF) << 24 );
			}

			template<>
			float operator()( float value ) const
			{
				UINT integer = operator()( reinterpret_cast< const UINT& >( value ) );
				return reinterpret_cast< const float& >( integer );
			}
		};

		template<>
		struct ToBytesSwapped< 8 >
		{
			template< typename NumericT >
			NumericT operator()( NumericT value ) const
			{
				return
					( (value & 0xFF00000000000000ull) >> 56 ) |
					( (value & 0x00FF000000000000ull) >> 40 ) |
					( (value & 0x0000FF0000000000ull) >> 24 ) |
					( (value & 0x000000FF00000000ull) >> 8  ) |
					( (value & 0x00000000FF000000ull) << 8  ) |
					( (value & 0x0000000000FF0000ull) << 24 ) |
					( (value & 0x000000000000FF00ull) << 40 ) |
					( (value & 0x00000000000000FFull) << 56 );
			}

			template<>
			double operator()( double value ) const
			{
				ULONGLONG integer = operator()( reinterpret_cast< const ULONGLONG& >( value ) );
				return reinterpret_cast< const double& >( integer );
			}
		};

	} //namespace impl


	enum Endianness
	{
		Little,
		Big,
		Network = Big
	};


	template< Endianness from, Endianness to >
	struct GetBytesSwapped
	{
		template< class NumericT >
		NumericT operator()( NumericT value ) const
		{
			return impl::ToBytesSwapped< sizeof( NumericT ) >()( value );
		}
	};

	// specialisations when attempting to swap to the same endianess

	template<>
	struct GetBytesSwapped< Little, Little >
	{
		template< class NumericT >
		NumericT operator()( NumericT value ) const { return value; }
	};

	template<>
	struct GetBytesSwapped< Big, Big >
	{
		template< class NumericT >
		NumericT operator()( NumericT value ) const { return value; }
	};

} //namespace endian


namespace func
{
	template< endian::Endianness from, endian::Endianness to >
	struct SwapBytes		// manage endian-ness conversions, e.g. for text encoding
	{
		template< typename NumericT >
		void operator()( NumericT& rNumber ) const
		{
			rNumber = ::endian::GetBytesSwapped< from, to >()( rNumber );
		}
	};
}


namespace endian
{
	template< endian::Endianness from, endian::Endianness to, typename CharT >
	void SwapBytes( std::basic_string< CharT >& rText )
	{
		std::for_each( rText.begin(), rText.end(), func::SwapBytes<from, to>() );
	}

} //namespace endian


#endif // Endianness_h
