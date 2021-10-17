#ifndef CommonDefs_h
#define CommonDefs_h
#pragma once


#pragma warning( disable: 4355 )	// 'this' : used in base member initializer list


#define _INDIRECT_LINE_NAME_( t ) #t
#define _LINE_NAME_( t ) _INDIRECT_LINE_NAME_( t )
#define __LINE__STR__ _LINE_NAME_( __LINE__ )
#define __FILE_LINE__ __FILE__ "(" __LINE__STR__ "): "

// To be used for defining promptable (F4 - Next Error) compile-time user warnings like:
//	#pragma message( __WARN__ "User: some warning message!" )
//
#define __WARN__ __FILE_LINE__ "warning C8989:\n  * "


// debug support
#ifdef ASSERT
	#undef ASSERT
#endif

#ifdef ENSURE
	#undef ENSURE
#endif

#define ASSERT _ASSERTE
#define REQUIRE ASSERT
#define ENSURE ASSERT
#define INVARIANT ASSERT

#define ASSERT_PTR( x ) ASSERT( ( x ) != NULL )
#define ASSERT_NULL( x ) ASSERT( ( x ) == NULL )

#define DEBUG_BREAK ASSERT( false )
#define DEBUG_BREAK_IF( cond ) ASSERT( !cond )


#define abstract			// class not instantiable
#define final				// don't override a method/don't subclass a class
#define persist				// persistent data-member
#define throws_( ... )
#define interface struct

#define COUNT_OF( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

// iterator-like access for STL algorithms
#define END_OF( array ) ( array + COUNT_OF( array ) )

// pass the pair of "array, arrayCount" in functions
#define ARRAY_PAIR( array ) (array), COUNT_OF( (array) )
#define ARRAY_PAIR_V( vect ) &(vect).front(), static_cast< unsigned int >( (vect).size() )


enum
{
	KiloByte = 1024,
	MegaByte = KiloByte * KiloByte,
	GigaByte = KiloByte * KiloByte * KiloByte,
};


enum RecursionDepth { Shallow, Deep };


template< typename Type >
inline Type* safe_ptr( Type* ptr )
{
	ASSERT_PTR( ptr );
	return ptr;
}

template< typename Type >
inline const Type* safe_ptr( const Type* ptr )
{
	ASSERT_PTR( ptr );
	return ptr;
}


template< typename Type, typename BaseType >
inline bool is_a( const BaseType* pObject )
{
	return dynamic_cast< const Type* >( pObject ) != NULL;
}

template< typename ToPtrType, typename FromPtrType >
inline ToPtrType checked_static_cast( FromPtrType fromPtr )
{
	ASSERT( dynamic_cast< ToPtrType >( fromPtr ) == static_cast< ToPtrType >( fromPtr ) );		// checked in debug builds
	return static_cast< ToPtrType >( fromPtr );
}

template< typename ToPtrType, typename FromPtrType >
inline ToPtrType safe_static_cast( FromPtrType fromPtr )
{
	return safe_ptr( checked_static_cast< ToPtrType >( fromPtr ) );
}


template< typename FieldType >
inline bool HasFlag( FieldType field, unsigned int flag )
{
	return ( field & flag ) != 0;
}

template< typename FieldType >
inline bool EqFlag( FieldType field, unsigned int flag )
{
	return flag == ( field & flag );
}

template< typename FieldType >
inline bool EqMaskedValue( FieldType field, unsigned int mask, unsigned int value )
{
	return value == ( field & mask );
}

template< typename FieldType >
inline void ClearFlag( FieldType& rField, unsigned int flag )
{
	rField &= ~flag;
}

template< typename FieldType >
inline FieldType GetMasked( FieldType field, unsigned int mask, bool on )
{
	return on ? ( field | mask ) : ( field & ~mask );
}

template< typename FieldType >
inline void SetFlag( FieldType& rField, unsigned int flag, bool on = true )
{
	if ( on )
		rField |= flag;
	else
		rField &= ~flag;
}

template< typename FieldType >
inline FieldType MakeFlag( FieldType field, unsigned int flag, bool on = true )
{
	SetFlag( field, flag, on );
	return field;
}

template< typename FieldType >
inline void ToggleFlag( FieldType& rField, unsigned int flag )
{
	rField ^= flag;
}

template< typename FieldType >
inline bool ModifyFlags( FieldType& rField, unsigned int clearFlags, unsigned int setFlags )
{
	FieldType oldField = rField;
	rField &= ~clearFlags;
	rField |= setFlags;
	return rField != oldField;
}

template< typename FieldType >
inline bool SetMaskedValue( FieldType& rField, unsigned int mask, unsigned int value )
{
	return ModifyFlags( rField, mask, value & mask );
}


// bit flags are bitfield values based on power of 2 exponent - simplifies bit field enum definitions
#define BIT_FLAG( exponent2 ) ( 1 << (exponent2) )

inline unsigned int ToBitFlag( int exponent2 )
{
	ASSERT( exponent2 < 32 );
	return 1 << exponent2;
}

template< typename FieldType >
inline bool HasBitFlag( FieldType field, int exponent2 )
{
	return HasFlag( field, ToBitFlag( exponent2 ) );
}

template< typename FieldType >
inline void ClearBitFlag( FieldType& rField, int exponent2 )
{
	ClearFlag( rField, ToBitFlag( exponent2 ) );
}

template< typename FieldType >
inline void SetBitFlag( FieldType& rField, int exponent2, bool on = true )
{
	SetFlag( rField, ToBitFlag( exponent2 ), on );
}


namespace utl
{
	enum Ternary { False, True, Default };

	inline bool EvalTernary( Ternary value, bool defaultValue ) { return True == value || ( Default == value && defaultValue ); }
	inline bool SetTernary( Ternary& rValue, bool value ) { Ternary oldValue = rValue; rValue = value ? True : False; return rValue != oldValue; }
	inline Ternary GetNextTernary( Ternary value ) { return Default == value ? False : static_cast<Ternary>( value + 1 ); }
	inline bool ToggleTernary( Ternary& rValue ) { rValue = static_cast<Ternary>( False == rValue ); return True == rValue; }
}


namespace utl
{
	__declspec( selectany ) extern const size_t npos = size_t( -1 );


	template< typename ContainerT >
	inline size_t GetLastPos( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return rItems.size() - 1; }


	template< typename ValueType >
	inline void AssignPtr( ValueType* pField, const ValueType& value )
	{
		if ( pField != NULL )
			*pField = value;
	}

	inline unsigned int GetPlatformBits( void ) { return sizeof( void* ) * 8; }
	inline unsigned int Is32bitPlatform( void ) { return 32 == GetPlatformBits(); }
	inline unsigned int Is64bitPlatform( void ) { return 64 == GetPlatformBits(); }

	template< typename ValueT >
	inline bool ModifyValue( ValueT& rValue, const ValueT& newValue )
	{
		if ( rValue == newValue )
			return false;				// value not changed
		rValue = newValue;
		return true;
	}


	// private copy constructor and copy assignment ensure derived classes cannot be copied.
	//
	class noncopyable
	{
	protected:
		noncopyable( void ) {}
		~noncopyable() {}
	private:  // emphasize the following members are private
		noncopyable( const noncopyable& );
		noncopyable& operator=( const noncopyable& );
	};


	// implemented by managed objects or inherited by managed interfaces
	//
	interface IMemoryManaged
	{
		virtual ~IMemoryManaged() = 0
		{
		}
	};
}


namespace str
{
	template< typename CharT >
	inline bool IsEmpty( const CharT* pText ) { return NULL == pText || 0 == *pText; }

	inline size_t GetLength( const char* pText ) { return strlen( pText ); }


	template< typename CharT >
	size_t& SettleLength( size_t& rCount, const CharT* pText )
	{
		if ( std::string::npos == rCount )
			rCount = str::GetLength( pText );
		else
			REQUIRE( rCount <= str::GetLength( pText ) );

		return rCount;
	}


	inline const char* end( const char* pText ) { return pText + GetLength( pText ); }

	template< typename CharT >
	inline const CharT* FindTokenEnd( const CharT* pText, const CharT delims[] )
	{
		const CharT* pTextEnd = str::end( pText );
		return std::find_first_of( pText, pTextEnd, delims, str::end( delims ) );
	}

	template< typename CharT >
	const CharT* SkipLineEnd( const CharT* pText )		// work for both text or binary mode: skips "\n" or "\r\n"
	{
		ASSERT_PTR( pText );

		if ( '\r' == *pText )
			++pText;

		if ( '\n' == *pText )
			++pText;

		return pText;
	}
}


#endif // CommonDefs_h
