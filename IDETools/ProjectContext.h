#ifndef ProjectContext_h
#define ProjectContext_h
#pragma once

#include "utl/UI/InternalChange.h"


class CProjectContext : private CInternalChange
{
public:
	CProjectContext( void );
	virtual ~CProjectContext();

	const std::tstring& GetLocalDirPath( void ) const { return m_localDirPath; }
	const std::tstring& GetLocalCurrentFile( void ) const { return m_localCurrentFile; }
	const std::tstring& GetAssociatedProjectFile( void ) const { return m_associatedProjectFile; }
	const std::tstring& GetProjectActiveConfiguration( void ) const { return m_projectActiveConfiguration; }

	void SetLocalDirPath( const std::tstring& localDirPath );
	void SetLocalCurrentFile( const std::tstring& localCurrentFile );
	void SetAssociatedProjectFile( const std::tstring& associatedProjectFile );
	void SetProjectActiveConfiguration( const std::tstring& projectActiveConfiguration );

	bool FindProjectFile( void );
protected:
	// notifications
	virtual void OnLocalDirPathChanged( void );
	virtual void OnLocalCurrentFileChanged( void );
	virtual void OnAssociatedProjectFileChanged( void );
	virtual void OnProjectActiveConfigurationChanged( void );
protected:
	std::tstring m_localDirPath;
	std::tstring m_localCurrentFile;
	std::tstring m_associatedProjectFile;
	std::tstring m_projectActiveConfiguration;
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


#endif // ProjectContext_h
