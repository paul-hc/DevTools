
#include "StdAfx.h"
#include "RegistrySection.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAppRegistrySection implementation

const std::tstring& CAppRegistrySection::GetSectionName( void ) const
{
	return m_section;
}

int CAppRegistrySection::GetIntParameter( const TCHAR entryName[], int defaultValue ) const
{
	return m_pApp->GetProfileInt( m_section.c_str(), entryName, defaultValue );
}

std::tstring CAppRegistrySection::GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue /*= NULL*/ ) const
{
	return m_pApp->GetProfileString( m_section.c_str(), entryName, pDefaultValue ).GetString();
}

bool CAppRegistrySection::SaveParameter( const TCHAR entryName[], int value ) const
{
	return m_pApp->WriteProfileInt( m_section.c_str(), entryName, value ) != FALSE;
}

bool CAppRegistrySection::SaveParameter( const TCHAR entryName[], const std::tstring& value ) const
{
	return m_pApp->WriteProfileString( m_section.c_str(), entryName, value.c_str() ) != FALSE;
}


// CRegistryEntry implementation

struct CRegistryEntry
{
	CRegistryEntry::CRegistryEntry( void ) : m_type( REG_NONE ) {}

	explicit CRegistryEntry::CRegistryEntry( DWORD value )
		: m_type( REG_DWORD )
		, m_valueBuffer( sizeof( DWORD ) )
	{
		ValueAsDWORD() = value;
	}

	explicit CRegistryEntry::CRegistryEntry( const std::tstring& value )
		: m_type( REG_SZ )
	{
		typedef const BYTE* Iterator;
		Iterator itStart = reinterpret_cast< const BYTE* >( value.c_str() ), itEnd = itStart + utl::ByteSize( value ) + 1;

		m_valueBuffer.assign( itStart, itEnd );
	}

	CRegistryEntry::CRegistryEntry( const BYTE* pValue, size_t size )
		: m_type( REG_BINARY )
		, m_valueBuffer( pValue, pValue + size )
	{
		ASSERT_PTR( pValue );
	}

	bool IsNone( void ) const { return REG_NONE == m_type && m_valueBuffer.empty(); }
	DWORD GetSize( void ) const { return static_cast< DWORD >( m_valueBuffer.size() ); }
	void Clear( void ) { m_type = REG_NONE; m_valueBuffer.clear(); }

	const TCHAR* ValueAsString( void ) { ASSERT( REG_SZ == m_type ); return reinterpret_cast< const TCHAR* >( &m_valueBuffer.front() ); }
	DWORD& ValueAsDWORD( void ) { ASSERT( REG_DWORD == m_type ); return *reinterpret_cast< DWORD* >( &m_valueBuffer.front() ); }
public:
	DWORD m_type;
	std::vector< BYTE > m_valueBuffer;
};


const HKEY CRegistrySection::s_hives[] = { HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CURRENT_CONFIG };

CRegistrySection::CRegistrySection( Hive hive, const std::tstring& section )
	: m_hKey( s_hives[ hive ] )
	, m_section( section )
{
}


CRegistrySection::CRegistrySection( HKEY hKey, const std::tstring& section )
	: m_hKey( hKey )
	, m_section( section )
{
}

const std::tstring& CRegistrySection::GetSectionName( void ) const
{
	return m_section;
}

int CRegistrySection::GetIntParameter( const TCHAR entryName[], int defaultValue ) const
{
	CRegistryEntry entry;
	if ( GetParameter( entryName, entry ) )
		if ( REG_SZ == entry.m_type )
			TRACE( _T(" * Ignoring registry entry %s=%s - it was saved as REG_SZ, but now has changed to REG_DWORD\n"), entryName, entry.ValueAsString() );
		else
		{
			ASSERT( entry.m_type == REG_DWORD || entry.m_type == REG_DWORD_LITTLE_ENDIAN || entry.m_type == REG_DWORD_BIG_ENDIAN );
			return entry.ValueAsDWORD();
		}

	return defaultValue;
}


std::tstring CRegistrySection::GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue/* = NULL*/ ) const
{
	CRegistryEntry entry;
	if ( GetParameter( entryName, entry ) )
		if ( REG_DWORD == entry.m_type )
			TRACE( _T(" * Ignoring registry entry %s=%d - it was saved as REG_DWORD, but now has changed to REG_SZ\n"), entryName, entry.ValueAsDWORD() );
		else
		{
			ASSERT( entry.m_type == REG_SZ || entry.m_type == REG_EXPAND_SZ || entry.m_type == REG_LINK );
			return entry.ValueAsString();
		}

	return pDefaultValue != NULL ? pDefaultValue : std::tstring();
}

bool CRegistrySection::GetParameter( const TCHAR entryName[], CRegistryEntry& rEntry ) const
{
	rEntry.Clear();

	HKEY hSubKey;
	if ( ::RegOpenKeyEx( m_hKey, m_section.c_str(), 0, KEY_READ, &hSubKey ) != ERROR_SUCCESS )
		return false;

	DWORD valueSize;
	DWORD valueType;

	if ( ::RegQueryValueEx( hSubKey, entryName, 0, &valueType, 0, &valueSize ) != ERROR_SUCCESS )
	{
		::RegCloseKey( hSubKey );
		return false;
	}

	std::vector< BYTE > valueBuffer( valueSize );
	LONG result = ::RegQueryValueEx( hSubKey, entryName, 0, &valueType, &valueBuffer.front(), &valueSize );

	::RegCloseKey( hSubKey );

	if ( result != ERROR_SUCCESS )
		return false;

	rEntry.m_type = valueType;
	rEntry.m_valueBuffer.swap( valueBuffer );

	ENSURE( !rEntry.IsNone() );
	return true;
}

bool CRegistrySection::SaveParameter( const TCHAR entryName[], int value ) const
{
	CRegistryEntry Data( value );
	return SaveParameter( entryName, Data );
}


bool CRegistrySection::SaveParameter( const TCHAR entryName[], const std::tstring& value ) const
{
	CRegistryEntry Data( value );
	return SaveParameter( entryName, Data );
}

bool CRegistrySection::SaveParameter( const TCHAR entryName[], const CRegistryEntry& entry ) const
{
	HKEY hSubKey;
	if ( ::RegCreateKeyEx( m_hKey, m_section.c_str(), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hSubKey, NULL ) != ERROR_SUCCESS )
		return false;

	LONG result = ::RegSetValueEx( hSubKey, entryName, 0, entry.m_type, &entry.m_valueBuffer.front(), entry.GetSize() );
	::RegCloseKey( hSubKey );
	return ERROR_SUCCESS == result;
}

bool CRegistrySection::DeleteParameter( const TCHAR entryName[] ) const
{
	ASSERT( !str::IsEmpty( entryName ) );

	HKEY hSubKey;

	if ( ::RegOpenKeyEx( m_hKey, m_section.c_str(), 0, KEY_WRITE | KEY_READ, &hSubKey ) != ERROR_SUCCESS )
		return false;

	LONG result = RegDeleteValue( hSubKey, entryName );
	::RegCloseKey( hSubKey );
	return ERROR_SUCCESS == result;
}

bool CRegistrySection::DeleteSection( void ) const
{
	HKEY hSubKey;

	if ( ::RegOpenKeyEx( m_hKey, m_section.c_str(), 0, KEY_WRITE | KEY_READ, &hSubKey ) != ERROR_SUCCESS )
		return false;

	DWORD numSubKeys = 0;
	do
	{
		// first get an info about this subkey ...
		DWORD subKeyMaxLength;
		if ( ::RegQueryInfoKey( hSubKey, 0, 0, 0, &numSubKeys, &subKeyMaxLength, 0, 0, 0, 0, 0, 0 ) != ERROR_SUCCESS )
		{
			::RegCloseKey( hSubKey );
			return false;
		}

		if( numSubKeys > 0 )
		{
			DWORD subKeyNameLength = subKeyMaxLength + 1;
			std::vector< TCHAR > subKeyNameBuffer( subKeyNameLength );

			if ( ::RegEnumKey( hSubKey, 0, &subKeyNameBuffer.front(), subKeyNameLength ) != ERROR_SUCCESS )
			{
				::RegCloseKey( hSubKey );
				return false;
			}

			if ( !CRegistrySection( hSubKey, &subKeyNameBuffer.front() ).DeleteSection() )
			{
				::RegCloseKey( hSubKey );
				return false;
			}
		}
	} while( numSubKeys > 0 );

	::RegCloseKey( hSubKey );

	return ERROR_SUCCESS == ::RegDeleteKey( m_hKey, m_section.c_str() );
}
