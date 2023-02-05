#ifndef Registry_h
#define Registry_h
#pragma once

#include <atlbase.h>
#include "Registry_fwd.h"


namespace reg
{
	bool WriteStringValue( HKEY hParentKey, const TKeyPath& keySubPath, const TCHAR* pValueName, const std::tstring& text );

	bool DeleteKey( HKEY hParentKey, const TKeyPath& keySubPath, RecursionDepth depth = Deep );		// deletes the rightmost key (identified by 'filename')


	class CKey;


	bool OpenKey( CKey* pKey, const TCHAR* pKeyFullPath, REGSAM samDesired = KEY_READ | KEY_WRITE );
	bool CreateKey( CKey* pKey, const TCHAR* pKeyFullPath );


	class CKey : private utl::noncopyable
	{
	public:
		explicit CKey( HKEY hKey = NULL ) : m_key( hKey ) {}
		CKey( CKey& rSrcMove ) : m_key( rSrcMove.m_key ) {}								// move constructor
		~CKey() { Close(); }

		CKey& operator=( CKey& rSrcMove ) { m_key = rSrcMove.m_key; return *this; }		// move assignment

		bool Open( HKEY hParentKey, const TCHAR* pSubKeyPath, REGSAM samDesired = KEY_READ | KEY_WRITE ) throw()
		{
			return ERROR_SUCCESS == m_key.Open( hParentKey, pSubKeyPath, samDesired );
		}

		bool Open( HKEY hParentKey, const TKeyPath& subKeyPath, REGSAM samDesired = KEY_READ | KEY_WRITE ) throw()
		{
			return ERROR_SUCCESS == m_key.Open( hParentKey, subKeyPath.GetPtr(), samDesired );
		}

		bool Create( HKEY hParentKey, const TCHAR* pSubKeyPath, LPTSTR pClass = REG_NONE,
					 DWORD dwOptions = REG_OPTION_NON_VOLATILE, REGSAM samDesired = KEY_READ | KEY_WRITE, SECURITY_ATTRIBUTES* pSecAttr = NULL, DWORD* pDisposition = NULL ) throw()
		{
			return ERROR_SUCCESS == m_key.Create( hParentKey, pSubKeyPath, pClass, dwOptions, samDesired, pSecAttr, pDisposition );
		}

		bool Create( HKEY hParentKey, const TKeyPath& subKeyPath, LPTSTR pClass = REG_NONE,
					 DWORD dwOptions = REG_OPTION_NON_VOLATILE, REGSAM samDesired = KEY_READ | KEY_WRITE, SECURITY_ATTRIBUTES* pSecAttr = NULL, DWORD* pDisposition = NULL ) throw()
		{
			return ERROR_SUCCESS == m_key.Create( hParentKey, subKeyPath.GetPtr(), pClass, dwOptions, samDesired, pSecAttr, pDisposition );
		}

		HKEY Get( void ) const { return m_key.m_hKey; }
		CRegKey& GetKey( void ) { return m_key; }

		bool IsOpen( void ) const { return Get() != NULL; }

		bool Close( void ) { return ERROR_SUCCESS == m_key.Close(); }
		void Reset( HKEY hKey = NULL ) { Close(); m_key.Attach( hKey ); }
		HKEY Detach( void ) { return m_key.Detach(); }

		bool Flush( void ) { return ERROR_SUCCESS == m_key.Flush(); }		// flush the key's data to disk

		static bool ParseFullPath( HKEY& rhHive, TKeyPath& rSubPath, const TCHAR* pKeyFullPath );		// full path includes registry hive (the root)

		void DeleteAll( void ) { DeleteAllValues(); DeleteAllSubKeys(); }

		// SUB-KEYS
		bool HasSubKey( const TCHAR* pSubKeyName ) const;
		void QuerySubKeyNames( std::vector<std::tstring>& rSubKeyNames ) const;
		bool DeleteSubKey( const TCHAR* pSubKey, RecursionDepth depth = Shallow ) { return ERROR_SUCCESS == ( Shallow == depth ? m_key.DeleteSubKey( pSubKey ) : m_key.RecurseDeleteKey( pSubKey ) ); }
		void DeleteAllSubKeys( void );

		// VALUES
		bool HasValue( const TCHAR* pValueName ) const { return GetValueType( pValueName ) != REG_NONE; }
		void QueryValueNames( std::vector<std::tstring>& rValueNames ) const;

		bool DeleteValue( const TCHAR* pValueName ) { return ERROR_SUCCESS == m_key.DeleteValue( pValueName ); }
		void DeleteAllValues( void );

		std::pair<DWORD, size_t> GetValueInfo( const TCHAR* pValueName ) const;		// <Type, BufferSize>
		DWORD GetValueType( const TCHAR* pValueName ) const { return GetValueInfo( pValueName ).first; }
		size_t GetValueBufferSize( const TCHAR* pValueName ) const { return GetValueInfo( pValueName ).second; }

		// string
		bool WriteStringValue( const TCHAR* pValueName, const std::tstring& text );
		bool QueryStringValue( const TCHAR* pValueName, std::tstring& rText ) const;
		std::tstring ReadStringValue( const TCHAR* pValueName, const std::tstring& defaultText = str::GetEmpty() ) const { std::tstring text; return QueryStringValue( pValueName, text ) ? text : defaultText; }

		// multi-string
		template< typename StringyT >
		bool WriteMultiString( const TCHAR* pValueName, const std::vector<StringyT>& values );

		template< typename StringyT >
		bool QueryMultiString( const TCHAR* pValueName, std::vector<StringyT>& rValues ) const;

		// number
		template< typename NumericT >
		bool WriteNumberValue( const TCHAR* pValueName, NumericT value );

		template< typename NumericT >
		bool QueryNumberValue( const TCHAR* pValueName, NumericT& rNumber ) const;

		template< typename NumericT >
		NumericT ReadNumberValue( const TCHAR* pValueName, NumericT defaultNumber = NumericT() ) const { NumericT number; return QueryNumberValue( pValueName, number ) ? number : defaultNumber; }

		// GUID (persisted as string)
		bool WriteGuidValue( const TCHAR* pValueName, const GUID& value ) { return ERROR_SUCCESS == m_key.SetGUIDValue( pValueName, value ); }
		bool QueryGuidValue( const TCHAR* pValueName, GUID& rValue ) const { return ERROR_SUCCESS == m_key.QueryGUIDValue( pValueName, rValue ); }

		// binary structure
		template< typename ValueT >
		bool WriteBinaryValue( const TCHAR* pValueName, const ValueT& value );

		template< typename ValueT >
		bool QueryBinaryValue( const TCHAR* pValueName, ValueT* pValue ) const;

		// binary buffer
		template< typename ValueT >
		bool WriteBinaryBuffer( const TCHAR* pValueName, const std::vector<ValueT>& bufferValue );

		template< typename ValueT >
		bool QueryBinaryBuffer( const TCHAR* pValueName, std::vector<ValueT>& rBufferValue ) const;
	private:
		mutable CRegKey m_key;			// friendly for Query... methods (declared non-const in CRegKey class)
	};


	struct CKeyInfo
	{
		CKeyInfo( void ) { Clear(); }
		CKeyInfo( HKEY hKey ) { Build( hKey ); }
		CKeyInfo( const CKey& key )
		{
			if ( key.IsOpen() )
				Build( key.Get() );
			else
				Clear();
		}

		void Clear( void );
		bool Build( HKEY hKey );
	public:
		DWORD m_subKeyCount;
		DWORD m_subKeyNameMaxLen;
		DWORD m_valueCount;
		DWORD m_valueNameMaxLen;
		DWORD m_valueBufferMaxLen;
		CTime m_lastWriteTime;
	};
}


#include "Algorithms.h"
#include "StringUtilities.h"


namespace reg
{
	// CKey template code

	template< typename StringyT >
	bool CKey::WriteMultiString( const TCHAR* pValueName, const std::vector<StringyT>& values )
	{
		ASSERT( IsOpen() );

		std::vector<TCHAR> msData;		// multi-string data: an array of zero-terminated strings, terminated by 2 zero characters
		str::QuickTokenize( msData, str::JoinLines( values, _T("|") ).c_str(), _T("|") );
		return ERROR_SUCCESS == m_key.SetMultiStringValue( pValueName, &msData.front() );
	}

	template< typename StringyT >
	bool CKey::QueryMultiString( const TCHAR* pValueName, std::vector<StringyT>& rValues ) const
	{
		ASSERT( IsOpen() );

		std::vector<TCHAR> buffer( GetValueBufferSize( pValueName ) );
		if ( buffer.empty() )
			return false;

		ULONG count = static_cast<ULONG>( buffer.size() );
		LONG result = m_key.QueryMultiStringValue( pValueName, &buffer.front(), &count );
		ASSERT( result != ERROR_MORE_DATA );		// GetValueBufferSize() sized the buffer properly?
		if ( result != ERROR_SUCCESS )
			return false;

		rValues.clear();
		for ( const TCHAR* pItem = &buffer.front(); *pItem != _T('\0'); pItem += str::GetLength( pItem ) + 1 )
			rValues.push_back( StringyT( pItem ) );
		return true;
	}


	template< typename NumericT >
	bool CKey::WriteNumberValue( const TCHAR* pValueName, NumericT value )
	{
		ASSERT( IsOpen() );
		if ( sizeof( ULONGLONG ) == sizeof( NumericT ) )
			return ERROR_SUCCESS == m_key.SetQWORDValue( pValueName, static_cast<ULONGLONG>( value ) );
		else
			return ERROR_SUCCESS == m_key.SetDWORDValue( pValueName, static_cast<DWORD>( value ) );
	}

	template< typename NumericT >
	bool CKey::QueryNumberValue( const TCHAR* pValueName, NumericT& rNumber ) const
	{
		ASSERT( IsOpen() );
		if ( sizeof( ULONGLONG ) == sizeof( NumericT ) )
		{
			ULONGLONG value;
			if ( m_key.QueryQWORDValue( pValueName, value ) != ERROR_SUCCESS )
				return false;

			rNumber = static_cast<NumericT>( value );
		}
		else
		{
			DWORD value;
			if ( m_key.QueryDWORDValue( pValueName, value ) != ERROR_SUCCESS )
				return false;

			rNumber = static_cast<NumericT>( value );
		}
		return true;
	}


	template< typename ValueT >
	bool CKey::WriteBinaryValue( const TCHAR* pValueName, const ValueT& value )
	{
		ASSERT( IsOpen() );
		return ERROR_SUCCESS == m_key.SetBinaryValue( pValueName, &value, static_cast<ULONG>( sizeof( value ) ) );
	}

	template< typename ValueT >
	bool CKey::QueryBinaryValue( const TCHAR* pValueName, ValueT* pValue ) const
	{
		ASSERT( IsOpen() );
		ASSERT_PTR( pValue );

		size_t byteSize = GetValueBufferSize( pValueName );
		if ( 0 == byteSize || byteSize != sizeof( ValueT ) )
			return false;				// bad size or it doesn't line up with ValueT's storage size

		ULONG count = static_cast<ULONG>( byteSize );
		return ERROR_SUCCESS == m_key.QueryBinaryValue( pValueName, pValue, &count );
	}


	template< typename ValueT >
	bool CKey::WriteBinaryBuffer( const TCHAR* pValueName, const std::vector<ValueT>& bufferValue )
	{
		return ERROR_SUCCESS == m_key.SetBinaryValue( pValueName, &bufferValue.front(), static_cast<ULONG>( utl::ByteSize( bufferValue ) ) );
	}

	template< typename ValueT >
	bool CKey::QueryBinaryBuffer( const TCHAR* pValueName, std::vector<ValueT>& rBufferValue ) const
	{
		size_t byteSize = GetValueBufferSize( pValueName );
		if ( 0 == byteSize )
			return false;
		if ( ( byteSize % sizeof( ValueT ) ) != 0 )
			return false;				// byte size doesn't line up with ValueT's storage size

		rBufferValue.resize( byteSize / sizeof( ValueT ) );

		ULONG count = static_cast<ULONG>( byteSize );
		return ERROR_SUCCESS == m_key.QueryBinaryValue( pValueName, &rBufferValue.front(), &count );
	}
}


#endif // Registry_h
