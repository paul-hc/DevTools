
#include "pch.h"
#include "Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	bool WriteStringValue( HKEY hParentKey, const TKeyPath& keySubPath, const TCHAR* pValueName, const std::tstring& text )
	{
		CKey key;
		return
			key.Create( hParentKey, keySubPath ) &&
			key.WriteStringValue( pValueName, text );
	}


	bool DeleteKey( HKEY hParentKey, const TKeyPath& keySubPath, RecursionDepth depth /*= Deep*/ )
	{
		ASSERT( !keySubPath.GetParentPath().IsEmpty() );		// must have a parent path to delete its sub-key

		reg::CKey parentKey;
		return
			parentKey.Open( hParentKey, keySubPath.GetParentPath() ) &&
			parentKey.DeleteSubKey( keySubPath.GetFilenamePtr(), depth );
	}

	bool OpenKey( CKey* pKey, const TCHAR* pKeyFullPath, REGSAM samDesired /*= KEY_READ | KEY_WRITE*/ )
	{
		ASSERT_PTR( pKey );
		HKEY hHive;
		TKeyPath subPath;

		if ( CKey::ParseFullPath( hHive, subPath, pKeyFullPath ) )
			return pKey->Open( hHive, subPath, samDesired );

		pKey->Reset();
		return false;
	}

	bool CreateKey( CKey* pKey, const TCHAR* pKeyFullPath )
	{
		ASSERT_PTR( pKey );
		HKEY hHive;
		TKeyPath subPath;

		if ( CKey::ParseFullPath( hHive, subPath, pKeyFullPath ) )
			return pKey->Create( hHive, subPath );

		pKey->Reset();
		return false;
	}



	// CKey implementation

	bool CKey::ParseFullPath( HKEY& rhHive, TKeyPath& rSubPath, const TCHAR* pKeyFullPath )
	{
		ASSERT( !str::IsEmpty( pKeyFullPath ) );
		rhHive = nullptr;
		rSubPath.Clear();

		size_t sepPos = str::Find<str::Case>( pKeyFullPath, _T('\\') );
		if ( sepPos != utl::npos )
		{
			rSubPath = TKeyPath( pKeyFullPath + sepPos + 1 );

			if ( str::EqualsN( pKeyFullPath, _T("HKEY_CLASSES_ROOT"), sepPos ) )
				rhHive = HKEY_CLASSES_ROOT;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_CURRENT_USER"), sepPos ) )
				rhHive = HKEY_CURRENT_USER;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_LOCAL_MACHINE"), sepPos ) )
				rhHive = HKEY_LOCAL_MACHINE;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_USERS"), sepPos ) )
				rhHive = HKEY_USERS;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_PERFORMANCE_DATA"), sepPos ) )
				rhHive = HKEY_PERFORMANCE_DATA;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_CURRENT_CONFIG"), sepPos ) )
				rhHive = HKEY_CURRENT_CONFIG;
			else if ( str::EqualsN( pKeyFullPath, _T("HKEY_DYN_DATA"), sepPos ) )
				rhHive = HKEY_DYN_DATA;
			else
			{
				rSubPath.Clear();
				return false;
			}

			return true;
		}
		return false;
	}

	bool CKey::HasSubKey( const TCHAR* pSubKeyName ) const
	{
		CKey subKey;
		return subKey.Open( Get(), pSubKeyName, KEY_READ );
	}

	void CKey::QuerySubKeyNames( std::vector<std::tstring>& rSubKeyNames ) const
	{
		ASSERT( IsOpen() );
		CKeyInfo info( Get() );

		rSubKeyNames.clear();
		rSubKeyNames.reserve( info.m_subKeyCount );

		std::vector<TCHAR> buffer( info.m_subKeyNameMaxLen + 1 );

		for ( DWORD i = 0; i != info.m_subKeyCount; ++i )
		{
			DWORD nameLen = static_cast<DWORD>( buffer.size() );

			if ( ERROR_SUCCESS == m_key.EnumKey( i, &buffer.front(), &nameLen ) )
				rSubKeyNames.push_back( &buffer.front() );
		}
	}

	void CKey::DeleteAllSubKeys( void )
	{
		ASSERT( IsOpen() );
		std::vector<std::tstring> subKeyNames;
		QueryValueNames( subKeyNames );

		for ( std::vector<std::tstring>::const_iterator itSubKeyName = subKeyNames.begin(); itSubKeyName != subKeyNames.end(); ++itSubKeyName )
			DeleteSubKey( itSubKeyName->c_str(), Deep );
	}

	void CKey::QueryValueNames( std::vector<std::tstring>& rValueNames ) const
	{
		ASSERT( IsOpen() );
		CKeyInfo info( Get() );

		rValueNames.clear();
		rValueNames.reserve( info.m_valueCount );

		std::vector<TCHAR> buffer( info.m_valueNameMaxLen + 1 );

		for ( DWORD i = 0; i != info.m_valueCount; ++i )
		{
			DWORD nameLen = static_cast<DWORD>( buffer.size() );

			if ( ERROR_SUCCESS == ::RegEnumValue( Get(), i, &buffer.front(), &nameLen, nullptr, nullptr, nullptr, nullptr ) )
				rValueNames.push_back( &buffer.front() );
		}
	}

	void CKey::DeleteAllValues( void )
	{
		ASSERT( IsOpen() );
		std::vector<std::tstring> valueNames;
		QueryValueNames( valueNames );

		for ( std::vector<std::tstring>::const_iterator itValueName = valueNames.begin(); itValueName != valueNames.end(); ++itValueName )
			DeleteValue( itValueName->c_str() );
	}

	std::pair<DWORD, size_t> CKey::GetValueInfo( const TCHAR* pValueName ) const
	{
		DWORD type, bufferSize;
		if ( ERROR_SUCCESS == ::RegQueryValueEx( Get(), pValueName, nullptr, &type, nullptr, &bufferSize ) )
			return std::pair<DWORD, size_t>( type, static_cast<size_t>( bufferSize ) );

		return std::pair<DWORD, size_t>( REG_NONE, 0 );
	}

	bool CKey::WriteStringValue( const TCHAR* pValueName, const std::tstring& text )
	{
		ASSERT( IsOpen() );
		return ERROR_SUCCESS == m_key.SetStringValue( pValueName, text.c_str() );
	}

	bool CKey::QueryStringValue( const TCHAR* pValueName, std::tstring& rText ) const
	{
		ASSERT( IsOpen() );

		std::vector<TCHAR> buffer( GetValueBufferSize( pValueName ) );
		if ( buffer.empty() )
			return false;

		ULONG count = static_cast<ULONG>( buffer.size() );
		LONG result = m_key.QueryStringValue( pValueName, &buffer.front(), &count );
		ASSERT( result != ERROR_MORE_DATA );		// GetValueBufferSize() sized the buffer properly?

		if ( result != ERROR_SUCCESS )
			return false;

		rText = &buffer.front();
		return true;
	}


	// CKeyInfo implementation

	void CKeyInfo::Clear( void )
	{
		m_subKeyCount = m_subKeyNameMaxLen = m_valueCount = m_valueNameMaxLen = m_valueBufferMaxLen = 0;
		m_lastWriteTime = CTime();
	}

	bool CKeyInfo::Build( HKEY hKey )
	{
		ASSERT_PTR( hKey );

		FILETIME lastWriteTime;
		if ( ERROR_SUCCESS == ::RegQueryInfoKey( hKey, nullptr, nullptr, nullptr, &m_subKeyCount, &m_subKeyNameMaxLen, nullptr,
												 &m_valueCount, &m_valueNameMaxLen, &m_valueBufferMaxLen, nullptr, &lastWriteTime ) )
		{
			m_lastWriteTime = CTime( lastWriteTime );
			return true;
		}
		return false;
	}
}
