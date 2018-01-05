#ifndef ProjectContext_h
#define ProjectContext_h
#pragma once

#include "utl/InternalChange.h"


class CProjectContext : private CInternalChange
{
public:
	CProjectContext( void );
	virtual ~CProjectContext();

	const std::tstring& GetLocalDirPath( void ) const { return m_localDirPath; }
	const std::tstring& GetLocalCurrentFile( void ) const { return m_localCurrentFile; }
	const std::tstring& GetAssociatedProjectFile( void ) const { return m_associatedProjectFile; }
	const std::tstring& GetProjectActiveConfiguration( void ) const { return m_projectActiveConfiguration; }
	const std::tstring& GetProjectAdditionalIncludePath( void ) const { return m_projectAdditionalIncludePath; }

	void SetLocalDirPath( const std::tstring& localDirPath );
	void SetLocalCurrentFile( const std::tstring& localCurrentFile );
	void SetAssociatedProjectFile( const std::tstring& associatedProjectFile );
	void SetProjectActiveConfiguration( const std::tstring& projectActiveConfiguration );
	void SetProjectAdditionalIncludePath( const std::tstring& projectAdditionalIncludePath );

	bool FindProjectFile( void );

	static std::tstring ExtractAdditionalIncludePath( const TCHAR* dspFullPath, const TCHAR* activeConfiguration = NULL );
protected:
	// notifications
	virtual void OnLocalDirPathChanged( void );
	virtual void OnLocalCurrentFileChanged( void );
	virtual void OnAssociatedProjectFileChanged( void );
	virtual void OnProjectActiveConfigurationChanged( void );
	virtual void OnProjectAdditionalIncludePathChanged( void );
protected:
	std::tstring m_localDirPath;
	std::tstring m_localCurrentFile;
	std::tstring m_associatedProjectFile;
	std::tstring m_projectActiveConfiguration;
	std::tstring m_projectAdditionalIncludePath;
};


// inline code

inline void CProjectContext::SetLocalCurrentFile( const std::tstring& localCurrentFile )
{
	m_localCurrentFile = localCurrentFile;
	OnLocalCurrentFileChanged();
}

inline void CProjectContext::SetAssociatedProjectFile( const std::tstring& associatedProjectFile )
{
	m_associatedProjectFile = associatedProjectFile;
	OnAssociatedProjectFileChanged();
}

inline void CProjectContext::SetProjectActiveConfiguration( const std::tstring& projectActiveConfiguration )
{
	m_projectActiveConfiguration = projectActiveConfiguration;
	OnProjectActiveConfigurationChanged();
}

inline void CProjectContext::SetProjectAdditionalIncludePath( const std::tstring& projectAdditionalIncludePath )
{
	m_projectAdditionalIncludePath = projectAdditionalIncludePath;
	OnProjectAdditionalIncludePathChanged();
}


#endif // ProjectContext_h
