#ifndef VersionInfo_h
#define VersionInfo_h
#pragma once


class CVersionInfo
{
public:
	CVersionInfo( UINT versionInfoId = VS_VERSION_INFO );
	~CVersionInfo();

	bool IsValid( void ) const { return !m_versionInfo.empty(); }
	const void* GetPtr( void ) const { ASSERT( !m_versionInfo.empty() ); return &m_versionInfo.front(); }
	const VS_FIXEDFILEINFO& GetFileInfo( void ) const { return m_fileInfo; }

	std::tstring ExpandValues( const TCHAR* pSource ) const;
	std::tstring FormatProductVersion( void ) const;
	std::tstring FormatFileVersion( void ) const;
	std::tstring FormatComments( void ) const;

	std::tstring GetValue( const TCHAR* pKeyName ) const;
	std::tstring FormatValue( const std::tstring& keyName ) const;

	std::tstring GetProductName( void ) const { return GetValue( _T("ProductName") ); }
	std::tstring GetProductVersion( void ) const { return GetValue( _T("ProductVersion") ); }
	std::tstring GetFileVersion( void ) const { return GetValue( _T("FileVersion") ); }
	std::tstring GetFileDescription( void ) const { return GetValue( _T("FileDescription") ); }
	std::tstring GetInternalName( void ) const { return GetValue( _T("InternalName") ); }
	std::tstring GetSpecialBuild( void ) const { return GetValue( _T("SpecialBuild") ); }
	std::tstring GetPrivateBuild( void ) const { return GetValue( _T("PrivateBuild") ); }
	std::tstring GetCopyright( void ) const { return GetValue( _T("LegalCopyright") ); }
	std::tstring GetTrademarks( void ) const { return GetValue( _T("LegalTrademarks") ); }
	std::tstring GetComments( void ) const { return GetValue( _T("Comments") ); }
	std::tstring GetCompanyName( void ) const { return GetValue( _T("CompanyName") ); }
private:
	std::tstring FormatVersion( DWORD ms, DWORD ls ) const;
private:
	enum { AnsiCodePage = 1252 };

	struct CLanguage
	{
		CLanguage( void ) : m_langId( 0 ), m_codePage( AnsiCodePage ) {}		// default ANSI code page
	public:
		WORD m_langId;			// language ID
		WORD m_codePage;		// character set (code page)
	};

	std::vector< BYTE > m_versionInfo;		// a copy of the resource, required by VerQueryValue()
	VS_FIXEDFILEINFO m_fileInfo;
	CLanguage m_translation;
public:
	static const std::tstring s_bulletPrefix;
};


#endif // VersionInfo_h
