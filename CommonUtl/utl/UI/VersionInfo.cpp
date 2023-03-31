
#include "pch.h"
#include "VersionInfo.h"
#include "StringParsing.h"
#include "TimeUtils.h"
#include <sstream>

#pragma comment( lib, "version.lib" )			// link to version.dll API

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const std::tstring CVersionInfo::s_bulletPrefix = _T("\x25CF ");

CVersionInfo::CVersionInfo( UINT versionInfoId /*= VS_VERSION_INFO*/ )
{
	ZeroMemory( &m_fileInfo, sizeof( m_fileInfo ) );

	HINSTANCE hResInst = AfxGetResourceHandle();
	if ( HRSRC hResInfo = FindResource( hResInst, MAKEINTRESOURCE( versionInfoId ), RT_VERSION ) )
	{
		DWORD dwSize = ::SizeofResource( hResInst, hResInfo );
		if ( HGLOBAL hResData = ::LoadResource( hResInst, hResInfo ) )
		{
			// VerQueryValue() requires a copy of the resource
			if ( BYTE* pRes = (BYTE*)::LockResource( hResData ) )
				m_versionInfo.assign( pRes, pRes + dwSize );

			FreeResource( hResData );
		}
	}

	if ( IsValid() )
	{
		// get file info
		UINT size;
		void* ptr;
		::VerQueryValue( GetPtr(), _T("\\"), &ptr, &size );
		m_fileInfo = *(VS_FIXEDFILEINFO*)ptr;

		// get translation info
		if ( ::VerQueryValue( GetPtr(), _T("\\VarFileInfo\\Translation"), &ptr, &size ) && size >= sizeof( CLanguage ) )
			m_translation = *(CLanguage*)ptr;
	}
}

CVersionInfo::~CVersionInfo()
{
}

std::tstring CVersionInfo::GetValue( const TCHAR* pKeyName ) const
{
	ASSERT_PTR( pKeyName );
	std::tstring value;
	if ( IsValid() )
	{
		std::tstring subBlock = str::Format( _T("\\StringFileInfo\\%04x%04x\\%s"), (int)m_translation.m_langId, (int)m_translation.m_codePage, pKeyName );
		void* ptr;
		UINT size = 0;
		if ( ::VerQueryValue( GetPtr(), const_cast<TCHAR*>( subBlock.c_str() ), &ptr, &size ) )
			value = (const TCHAR*)ptr;
	}

	return value;
}

std::tstring CVersionInfo::FormatValue( const std::tstring& keyName ) const
{
	if ( keyName == _T("ProductVersion") )
		return FormatProductVersion();
	else if ( keyName == _T("FileVersion") )
		return FormatFileVersion();
	else if ( keyName == _T("Comments") )
		return FormatComments();
	else if ( keyName == _T("BuildDate") )
		return FormatBuildDate();
	else if ( keyName == _T("BuildTime") )
		return FormatBuildTime();

	return GetValue( keyName.c_str() );
}

std::tstring CVersionInfo::FormatVersion( DWORD ms, DWORD ls ) const
{
	return str::Format( _T("%d.%d  %c  revision %d, build %d"),
		HIWORD( ms ), LOWORD( ms ),
		str::Conditional( L'\x25cf', '-' ),
		HIWORD( ls ), LOWORD( ls ) );
}

std::tstring CVersionInfo::FormatProductVersion( void ) const
{
	return IsValid()
		? FormatVersion( m_fileInfo.dwProductVersionMS, m_fileInfo.dwProductVersionLS )
		: std::tstring();
}

std::tstring CVersionInfo::FormatFileVersion( void ) const
{
	return IsValid()
		? FormatVersion( m_fileInfo.dwFileVersionMS, m_fileInfo.dwFileVersionLS )
		: std::tstring();
}

std::tstring CVersionInfo::FormatComments( void ) const
{
	std::vector<std::tstring> items;
	str::Split( items, GetComments().c_str(), _T("|") );

	for ( std::vector<std::tstring>::iterator itItem = items.begin(); itItem != items.end(); ++itItem )
	{
		str::Trim( *itItem );

		if ( !itItem->empty() )
			*itItem = s_bulletPrefix + *itItem;
	}

	return str::Join( items, _T("\r\n") );
}

std::tstring CVersionInfo::FormatBuildDate( void ) const
{
	CTime timestamp = time_utl::ParseStdTimestamp( GetBuildTimestamp() );
	std::tstring buildDate;

	if ( time_utl::IsValid( timestamp ) )
		buildDate = time_utl::FormatTimestamp( timestamp, _T("%d %b %Y") );

	return buildDate;
}

std::tstring CVersionInfo::FormatBuildTime( void ) const
{
	CTime timestamp = time_utl::ParseStdTimestamp( GetBuildTimestamp() );
	std::tstring buildTime;

	if ( time_utl::IsValid( timestamp ) )
		buildTime = time_utl::FormatTimestamp( timestamp, _T("%#H:%M:%S") );

	return buildTime;
}


namespace func
{
	struct EvalVersionValue
	{
		EvalVersionValue( const CVersionInfo* pVersionInfo ) : m_pVersionInfo( pVersionInfo ) { ASSERT_PTR( m_pVersionInfo ); }

		std::tstring operator()( const std::tstring& keyName ) const
		{
			return m_pVersionInfo->FormatValue( keyName.c_str() );
		}
	private:
		const CVersionInfo* m_pVersionInfo;
	};
}


std::tstring CVersionInfo::ExpandValues( const TCHAR* pSource ) const
{
	return str::ExpandKeysToValues( pSource, _T("["), _T("]"), func::EvalVersionValue( this ) );
}
