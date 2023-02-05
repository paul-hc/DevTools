#ifndef CommonDefs_h
#define CommonDefs_h
#pragma once

#include <crtdbg.h>

#ifdef _DEBUG
	#undef _ASSERT_EXPR

	// We output the message as string, not as a format.
	// This way we avoid assertion firing when printing '%' characters, mistaken for invalid printf format sequence.
	//
	#define _ASSERT_EXPR(expr, msg) \
			(void) ((!!(expr)) || \
					(1 != _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%s", msg)) || \
					(_CrtDbgBreak(), 0))
#endif //_DEBUG


#pragma warning( disable: 4355 )	// 'this' : used in base member initializer list


#define _INDIRECT_LINE_NAME_( t ) #t
#define _LINE_NAME_( t ) _INDIRECT_LINE_NAME_( t )
#define __LINE__STR__ _LINE_NAME_( __LINE__ )
#define __FILE_LINE__ __FILE__ "(" __LINE__STR__ "): "

// To be used for defining promptable (F4 - Next Error) compile-time user warnings like:
//	#pragma message( __WARN__ "User: some warning message!" )
//
#define __WARN__ __FILE_LINE__ "warning C9999:\n  * "


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
#define override			// method suffix to indicate a base override - note: replaces the CLR override keyword
#define final				// don't override a method/don't subclass a class
#define persist				// persistent data-member
#define throws_( ... )


#ifndef _HAS_CXX17
	#define nullptr 0
#endif //_HAS_CXX17


#define COUNT_OF( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

// iterator-like access for STL algorithms
#define END_OF( array ) ( array + COUNT_OF( array ) )

// pass the pair of "array, arrayCount" in functions
#define ARRAY_PAIR( array ) (array), COUNT_OF( (array) )
#define ARRAY_PAIR_V( vect ) &(vect).front(), static_cast<unsigned int>( (vect).size() )

#include <unknwn.h>

#ifdef _DEBUG
	#define HR_AUDIT( expr ) utl::Audit( (expr), (#expr) )
	#define HR_OK( expr ) utl::Check( (expr), (#expr) )

	namespace str
	{
		// forward declarations
		std::string AsNarrow( const std::tstring& text );
		std::tstring GetTypeName( const type_info& info );
		std::tstring Format( const TCHAR* pFormat, ... );
	}

	namespace dbg
	{
		inline int GetRefCount( IUnknown* pInterface )
		{
			if ( NULL == pInterface )
				return 0;

			pInterface->AddRef();
			ULONG refCount = pInterface->Release();
			return static_cast<int>( refCount );
		}

		template< typename InterfaceT >
		inline std::tstring FormatRefCount( InterfaceT* pInterface )
		{
			return str::Format( _T("<%s> ptr=0x%08x ref_count=%d"), str::GetTypeName( typeid( pInterface ) ).c_str(), pInterface, GetRefCount( pInterface ) );
		}
	}

	template< typename InterfaceT >
	void TRACE_COM_ITF( InterfaceT* pInterface, const char* pSuffix ) { TRACE( "@ TRACE_COM_ITF%s  -  %s\n", str::AsNarrow( dbg::FormatRefCount( pInterface ) ).c_str(), pSuffix ); }

	template< typename CComPtr_T >
	void TRACE_COM_PTR( const CComPtr_T& ptr, const char* pSuffix ) { TRACE_COM_ITF( ptr.p, pSuffix ); }

#else

	#define HR_AUDIT( expr ) utl::Audit( (expr), NULL )
	#define HR_OK( expr ) utl::Check( (expr), NULL )

	#define TRACE_COM_ITF( pInterface, pSuffix ) __noop
	#define TRACE_COM_PTR( ptr, pSuffix ) __noop

#endif


#define HR_VERIFY( expr )	VERIFY( HR_OK( (expr) ) )


namespace utl
{
	HRESULT Audit( HRESULT hResult, const char* pFuncName );
	inline bool Check( HRESULT hResult, const char* pFuncName ) { return SUCCEEDED( Audit( hResult, pFuncName ) ); }
}


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
	return dynamic_cast<const Type*>( pObject ) != NULL;
}

template< typename ToPtrType, typename FromPtrType >
inline ToPtrType checked_static_cast( FromPtrType fromPtr )
{
	ASSERT( dynamic_cast<ToPtrType>( fromPtr ) == static_cast<ToPtrType>( fromPtr ) );		// checked in debug builds
	return static_cast<ToPtrType>( fromPtr );
}

template< typename ToPtrType, typename FromPtrType >
inline ToPtrType safe_static_cast( FromPtrType fromPtr )
{
	return safe_ptr( checked_static_cast<ToPtrType>( fromPtr ) );
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
	__declspec( selectany ) extern const size_t npos = std::tstring::npos;


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


#include "Compare_fwd.h"


#endif // CommonDefs_h
