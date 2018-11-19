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


#define abstract
#define persist
#define throws_( ... )
#define COUNT_OF( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

// iterator-like access for STL algorithms
#define END_OF( array ) ( array + COUNT_OF( array ) )

// pass the pair of "array, arrayCount" in functions
#define ARRAY_PAIR( array ) (array), COUNT_OF( (array) )


#ifdef _DEBUG
	#define HR_AUDIT( expr ) utl::Audit( (expr), (#expr) )
	#define HR_OK( expr ) utl::Check( (expr), (#expr) )

	namespace dbg
	{
		template< typename Type >
		inline std::tstring FormatRefCount( Type* pointer )
		{
			ULONG refCount = 0;
			if ( pointer != NULL )
			{
				pointer->AddRef();
				refCount = pointer->Release();
			}
			return str::Format( _T("<%s> ptr=0x%08x ref_count=%d"), str::GetTypeName( typeid( pointer ) ).c_str(), pointer, refCount );
		}
	}

	template< typename Type >
	void TRACE_ITF( Type* pointer ) { TRACE( _T("@ TRACE_ITF%s\n"), dbg::FormatRefCount( pointer ).c_str() ); }

	template< typename ComPtrType >
	void TRACE_COM_PTR( const ComPtrType& ptr ) { TRACE( _T("@ TRACE_COM_PTR%s\n"), dbg::FormatRefCount( ptr.p ).c_str() ); }

#else

	#define HR_AUDIT( expr ) utl::Audit( (expr), NULL )
	#define HR_OK( expr ) utl::Check( (expr), NULL )

	#define TRACE_ITF( pointer ) __noop
	#define TRACE_COM_PTR( ptr ) __noop

#endif


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
inline void SetFlag( FieldType& rField, unsigned int flag, bool on = true )
{
	if ( on )
		rField |= flag;
	else
		rField &= ~flag;
}

template< typename FieldType >
inline void ToggleFlag( FieldType& rField, unsigned int flag )
{
	rField ^= flag;
}

template< typename FieldType >
inline bool ModifyFlag( FieldType& rField, unsigned int clearFlags, unsigned int setFlags )
{
	FieldType oldField = rField;
	rField &= ~clearFlags;
	rField |= setFlags;
	return rField != oldField;
}

template< typename FieldType >
inline bool SetMaskedValue( FieldType& rField, unsigned int mask, unsigned int value )
{
	return ModifyFlag( rField, mask, value & mask );
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
	__declspec( selectany ) extern const size_t npos = std::tstring::npos;


	template< typename ValueType >
	inline void AssignPtr( ValueType* pField, const ValueType& value )
	{
		if ( pField != NULL )
			*pField = value;
	}

	inline unsigned int GetPlatformBits( void ) { return sizeof( void* ) * 8; }
	inline unsigned int Is32bitPlatform( void ) { return 32 == GetPlatformBits(); }
	inline unsigned int Is64bitPlatform( void ) { return 64 == GetPlatformBits(); }

	template< typename Type >
	inline bool ModifyValue( Type& rValue, const Type& newValue )
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
}


// forward declarations
void AFXAPI AfxSetWindowText( HWND hWndCtrl, LPCTSTR lpszNew );		// <afxpriv.h>
void AFXAPI AfxCancelModes( HWND hWndRcvr );						// <src/mfc/afximpl.h>
BOOL AFXAPI AfxFullPath( _Pre_notnull_ _Post_z_ LPTSTR lpszPathOut, LPCTSTR lpszFileIn );		// <src/mfc/afximpl.h>


#include "Compare_fwd.h"
#include "StringBase.h"
#include "StdColors.h"


#endif // CommonDefs_h
