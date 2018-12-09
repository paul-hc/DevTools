
#include "stdafx.h"
#include "Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	// CRegKey (ATL class) helpers

	bool QueryValue( std::tstring& rValue, const CRegKey& key, const TCHAR* pValueName )
	{
		TCHAR buffer[ MAX_PATH * 2 ];
		ULONG charCount;
		if ( ERROR_SUCCESS == const_cast< CRegKey& >( key ).QueryStringValue( pValueName, buffer, &charCount ) )
		{
			rValue = buffer;
			return true;
		}
		return false;
	}
}


namespace reg
{
	// obsolete stuff: should be based on using CRegKey ATL class

	void QuerySubKeyNames( std::vector< std::tstring >& rSubKeyNames, const reg::CKey& key )
	{
		if ( !key.IsValid() )
			return;

		reg::CKeyIterator itSubKey( &key );
		if ( 0 == itSubKey.GetCount() )
			return;

		for ( ; itSubKey.IsValid(); ++itSubKey )
			rSubKeyNames.push_back( itSubKey.GetName() );
	}
}


namespace reg
{
	CKey::CKey( HKEY hParentKey, const TCHAR* pKeyName, bool forceCreate /*= false*/ )
		: m_hKey( NULL )
	{
		if ( ( forceCreate ? ::RegCreateKey( hParentKey, pKeyName, &m_hKey )
						   : ::RegOpenKey( hParentKey, pKeyName, &m_hKey ) ) != ERROR_SUCCESS )
			m_hKey = NULL;
	}

	HKEY CKey::Detach( void )
	{
		HKEY hKeyOrg = m_hKey;
		m_hKey = NULL;
		return hKeyOrg;
	}

	// extracts the root key handle from string key path and the rest of the key sub-path
	HKEY CKey::ExtractRootKey( std::tstring& rSubKeysPath, const std::tstring& srcKeyPath )
	{
		HKEY hRootKey = NULL;

		rSubKeysPath.clear();
		if ( !srcKeyPath.empty() )
		{
			size_t rootSepPos = srcKeyPath.find( '\\' );
			std::tstring rootKeyName( rootSepPos != std::tstring::npos ? srcKeyPath.substr( 0, rootSepPos ) : srcKeyPath );

			if ( rootSepPos != -1 )
			{
				rootKeyName = srcKeyPath.substr( 0, rootSepPos );
				rSubKeysPath = srcKeyPath.substr( rootSepPos + 1 );
			}
			else
				rootKeyName = srcKeyPath;

			// check for known root keys
			if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_CLASSES_ROOT") ) )
				hRootKey = HKEY_CLASSES_ROOT;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_CURRENT_USER") ) )
				hRootKey = HKEY_CURRENT_USER;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_LOCAL_MACHINE") ) )
				hRootKey = HKEY_LOCAL_MACHINE;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_USERS") ) )
				hRootKey = HKEY_USERS;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_PERFORMANCE_DATA") ) )
				hRootKey = HKEY_PERFORMANCE_DATA;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_CURRENT_CONFIG") ) )
				hRootKey = HKEY_CURRENT_CONFIG;
			else if ( str::Equals< str::IgnoreCase >( rootKeyName.c_str(), _T("HKEY_DYN_DATA") ) )
				hRootKey = HKEY_DYN_DATA;
		}
		return hRootKey;
	}

	// opens or creates a nested sub-key of 'hParentKey' specified by pSubKeyPath
	HKEY CKey::OpenKeySubPath( HKEY hParentKey, const TCHAR* pSubKeyPath, bool forceCreate /*= false*/ )
	{
		std::tstring subKeyPath( pSubKeyPath );
		HKEY m_hKey = NULL;

		if ( hParentKey != NULL )
		{
			TCHAR* pKeyName = _tcstok( (TCHAR*)subKeyPath.c_str(), _T("\\") );

			// Iterate for each sub-key name:
			while ( pKeyName != NULL && hParentKey != NULL )
			{
				if ( ( forceCreate ? ::RegCreateKey( hParentKey, pKeyName, &m_hKey )
								   : ::RegOpenKey( hParentKey, pKeyName, &m_hKey ) ) != ERROR_SUCCESS )
					m_hKey = NULL;
				// Close the previous parent key:
				::RegCloseKey( hParentKey );
				hParentKey = m_hKey;
				// Get next sub-key name:
				pKeyName = _tcstok( NULL, _T("\\") );
			}
		}
		return m_hKey;
	}

	HKEY CKey::OpenKeyFullPath( const TCHAR* pKeyFullPath, bool forceCreate /*= false*/ )
	{
		// opens or creates a nested full key path specified by pKeyFullPath
		std::tstring subKeyPath;
		HKEY hParentKey = ExtractRootKey( subKeyPath, pKeyFullPath );

		return CKey::OpenKeySubPath( hParentKey, subKeyPath.c_str(), forceCreate );
	}

	bool CKey::Close( void )
	{
		if ( !IsValid() )
			return false;
		::RegCloseKey( m_hKey );
		m_hKey = NULL;
		return true;
	}

	void CKey::RemoveAllSubKeys( void )
	{
		// reverse order to be able to remove sub-keys
		for ( CKeyIterator itSubKey( this, SB_Last ); itSubKey.IsValid(); --itSubKey )
		{
			CKey subKey( m_hKey, itSubKey.GetName() );
			subKey.RemoveAllSubKeys();
			subKey.Close();
			RemoveSubKey( itSubKey.GetName() );
		}
	}

	DWORD CKey::GetValueType( const TCHAR* pValueName, DWORD* pBufferSize /*= NULL*/ ) const
	{
		DWORD type;

		// first query for desired buffer size
		if ( ::RegQueryValueEx( m_hKey, pValueName, NULL, &type, NULL, pBufferSize ) != ERROR_SUCCESS )
			return REG_NONE;
		return type;
	}

	bool CKey::GetValue( CValue& rValue, const TCHAR* pValueName ) const
	{
		return rValue.Read( *this, pValueName );
	}

	bool CKey::SetValue( const TCHAR* pValueName, const CValue& value )
	{
		return value.Write( *this, pValueName );
	}

	bool CKey::RemoveValue( const TCHAR* pValueName )
	{
		ASSERT( IsValid() );
		return ERROR_SUCCESS == ::RegDeleteValue( m_hKey, pValueName );		// if pValueName is NULL/empty -> the default value is reset
	}

	void CKey::RemoveAllValues( void )
	{
		// in reverse order to be able to remove values
		for ( CValueIterator itValue( this, SB_Last ); itValue.IsValid(); --itValue )
			RemoveValue( itValue.GetName() );
	}

	std::tstring CKey::ReadString( const TCHAR* pValueName, const TCHAR* pDefaultText /*= _T("")*/ )
	{
		std::tstring text( pDefaultText );

		if ( IsValid() )
			switch ( GetValueType( pValueName ) )
			{
				case REG_SZ:				// Unicode nul terminated string
				case REG_EXPAND_SZ:			// Unicode nul terminated string
				case REG_MULTI_SZ:			// Multiple Unicode strings
				case REG_BINARY:			// Free form binary
				{
					CValue value;
					if ( GetValue( value, pValueName ) )
						text = value.GetString();
					break;
				}
				default:
					break;					// Value type mismatch !!!
			}

		return text;
	}

	long CKey::ReadNumber( const TCHAR* pValueName, long defaultNumber /*= 0L*/ )
	{
		long numberValue = defaultNumber;

		if ( IsValid() )
		{
			DWORD numType = GetValueType( pValueName );

			switch ( numType )
			{
				case REG_DWORD:					// 32-bit number (alias REG_DWORD_LITTLE_ENDIAN)
				case REG_DWORD_BIG_ENDIAN:		// 32-bit number
				{
					CValue value;
					if ( GetValue( value, pValueName ) )
					{
						numberValue = value.GetDword();

						if ( numType == REG_DWORD_BIG_ENDIAN )
							numberValue = MAKELONG( HIWORD( numberValue ), LOWORD( numberValue ) );	// swap low and high words for big endian numeric representation
					}
					break;
				}
				default:
					break;					// Value type mismatch !!!
			}
		}
		return numberValue;
	}

	bool CKey::WriteString( const TCHAR* pValueName, const TCHAR* pValue )
	{
		if ( !IsValid() )
			return false;

		CValue stringValue;
		stringValue.SetString( pValue );
		return SetValue( pValueName, stringValue );
	}

	bool CKey::WriteNumber( const TCHAR* pValueName, long value )
	{
		if ( !IsValid() )
			return false;

		CValue numValue;
		numValue.SetDword( value );
		return SetValue( pValueName, numValue );
	}


	// CValue implementation

	CValue::CValue( void )
		: m_type( REG_NONE )
		, m_pBuffer( NULL )
		, m_size( 0 )
		, m_autoDelete( false )
	{
	}

	CValue::CValue( const CValue& src )
		: m_type( REG_NONE )
		, m_pBuffer( NULL )
		, m_size( 0 )
		, m_autoDelete( false )
	{
		operator=( src );
	}

	CValue::~CValue()
	{
		Destroy();
	}

	CValue& CValue::operator=( const CValue& src )
	{
		Destroy();
		m_type = src.m_type;
		m_pBuffer = src.m_pBuffer;
		m_size = src.m_size;
		m_autoDelete = src.m_autoDelete;
		const_cast< CValue& >( src ).Detach();
		return *this;
	}

	void CValue::AttachBuffer( void* extDestBuffer, DWORD extBufferSize )
	{
		ASSERT( extDestBuffer != NULL && extBufferSize > 0 );
		Destroy();
		m_type = REG_NONE;
		m_pBuffer = (BYTE*)extDestBuffer;
		m_size = extBufferSize;
	}

	// srcBuffer may be null, in which case nothing is copied to allocate buffer
	BYTE* CValue::AllocCopy( const void* srcBuffer, DWORD srcBufferSize, DWORD srcType /*= REG_BINARY*/ )
	{
		ASSERT( srcBufferSize > 0 );
		Destroy();
		m_autoDelete = true;

		m_type = srcType;
		m_pBuffer = new BYTE[ m_size = srcBufferSize ];
		if ( srcBuffer != NULL )
			memcpy( m_pBuffer, srcBuffer, m_size );
		return m_pBuffer;
	}

	bool CValue::Destroy( void )
	{
		if ( !IsValid() )
			return false;
		if ( m_autoDelete )
			delete m_pBuffer;
		Detach();
		return true;
	}

	void CValue::Detach( void )
	{
		m_type = REG_NONE;
		m_pBuffer = NULL;
		m_size = 0;
		m_autoDelete = false;
	}

	bool CValue::ensureBuffer( DWORD requiredSize )
	{
		ASSERT( requiredSize > 0 );
		if ( requiredSize <= m_size )
			return false;		// No need to realocate.
		else
		{
			Destroy();
			m_pBuffer = new BYTE[ m_size = requiredSize ];
			m_autoDelete = true;
		}
		return true;
	}

	bool CValue::Read( const CKey& key, const TCHAR* pValueName )
	{
		ASSERT( key.IsValid() );

		DWORD requiredSize = 0;

		// first query for desired buffer size
		if ( ::RegQueryValueEx( key.Get(), pValueName, NULL, NULL, NULL, &requiredSize ) != ERROR_SUCCESS )
			return false;
		ensureBuffer( requiredSize );
		VERIFY( ERROR_SUCCESS == ::RegQueryValueEx( key.Get(), pValueName, NULL, &m_type, m_pBuffer, &requiredSize ) );
		ASSERT( IsValid() );
		return true;
	}

	bool CValue::Write( CKey& key, const TCHAR* pValueName ) const
	{
		ASSERT( IsValid() && key.IsValid() );
		return ERROR_SUCCESS == ::RegSetValueEx( key.Get(), pValueName, 0, m_type, m_pBuffer, m_size );
	}

	void CValue::Assign( DWORD srcType, const void* srcBuffer, DWORD srcBufferSize )
	{
		ASSERT( !m_autoDelete );
		m_type = srcType;
		m_pBuffer = (BYTE*)srcBuffer;
		m_size = srcBufferSize;
	}

	void CValue::SetDword( DWORD dword )
	{
		Assign( REG_DWORD, (const BYTE*)&m_quickBuffer, sizeof( DWORD ) );
		*(DWORD*)m_pBuffer = dword;
	}


	// CKeyIterator implementation

	CKeyIterator::CKeyIterator( const CKey* pParentKey, SeekBound bound /*= SB_First*/ )
		: m_pParentKey( pParentKey )
		, m_keyInfo( m_pParentKey->Get() )
		, m_pSubKeyName( new TCHAR[ m_keyInfo.m_subKeyNameMaxLen + 1 ] )
		, m_pos( bound == SB_First ? 0 : ( m_keyInfo.m_subKeyCount - 1 ) )
		, m_lastResult( ERROR_SUCCESS )
	{
		ASSERT_PTR( m_pParentKey );
		SeekToPos();
	}

	CKeyIterator::~CKeyIterator()
	{
		delete m_pSubKeyName;
	}

	bool CKeyIterator::IsValid( void ) const
	{
		return m_pParentKey->IsValid() &&
			   m_pos >= 0 && m_pos < (int)m_keyInfo.m_subKeyCount &&
			   ERROR_SUCCESS == m_lastResult;
	}

	bool CKeyIterator::SeekToPos( void )
	{
		if ( !IsValid() )
			return false;

		DWORD subKeyNameLen = m_keyInfo.m_subKeyNameMaxLen + 1;

		m_lastResult = ::RegEnumKeyEx( m_pParentKey->Get(), m_pos, m_pSubKeyName, &subKeyNameLen, NULL, NULL, NULL, NULL );
		ASSERT( IsValid() && subKeyNameLen <= m_keyInfo.m_subKeyNameMaxLen );
		return true;
	}


	// CValueIterator implementation

	CValueIterator::CValueIterator( const CKey* pParentKey, SeekBound bound /*= SB_First*/ )
		: m_pParentKey( pParentKey )
		, m_keyInfo( m_pParentKey->Get() )
		, m_pValueName( new TCHAR[ m_keyInfo.m_valueNameMaxLen + 1 ] )
		, m_valueType( REG_NONE )
		, m_valueBuffSize( 0 )
		, m_pos( bound == SB_First ? 0 : ( m_keyInfo.m_valueCount - 1 ) )
		, m_lastResult( ERROR_SUCCESS )
	{
		ASSERT_PTR( m_pParentKey );
		SeekToPos();
	}

	CValueIterator::~CValueIterator()
	{
		delete m_pValueName;
	}

	bool CValueIterator::IsValid( void ) const
	{
		return m_pParentKey->IsValid() &&
			   m_pos >= 0 && m_pos < (int)m_keyInfo.m_valueCount &&
			   ERROR_SUCCESS == m_lastResult;
	}

	CValueIterator& CValueIterator::Restart( SeekBound bound /*= SB_First*/ )
	{
		m_pos = bound == SB_First ? 0 : ( m_keyInfo.m_valueCount - 1 );
		SeekToPos();
		return *this;
	}

	bool CValueIterator::SeekToPos( void )
	{
		if ( !IsValid() )
			return false;

		DWORD valueNameLen = m_keyInfo.m_valueNameMaxLen + 1;

		m_lastResult = ::RegEnumValue( m_pParentKey->Get(), m_pos, m_pValueName, &valueNameLen, NULL, &m_valueType, NULL, &m_valueBuffSize );
		ASSERT( IsValid() && valueNameLen <= m_keyInfo.m_valueNameMaxLen );
		return true;
	}

} //namespace reg
