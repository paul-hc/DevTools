#ifndef ThemeStore_h
#define ThemeStore_h
#pragma once

#include "utl/ComparePredicates.h"
#include "utl/ContainerUtilities.h"
#include "utl/ISubject.h"


class CVisualTheme;


enum Relevance { HighRelevance, MediumRelevance, ObscureRelevance, NotImplemented = ObscureRelevance };


interface IThemeNode : public utl::ISubject
{
	enum ThemeNode { Class = 1, Part, State };

	virtual ThemeNode GetThemeNode( void ) const = 0;
	virtual Relevance GetRelevance( void ) const = 0;
};


class CBaseNode : public IThemeNode
{
protected:
	CBaseNode( Relevance relevance ) : m_relevance( relevance ) {}
public:
	virtual Relevance GetRelevance( void ) const { return m_relevance; }
	void SetRelevance( Relevance relevance ) { m_relevance = relevance; }
private:
	Relevance m_relevance;
};


struct CThemeState : public CBaseNode
{
	CThemeState( int stateId, const std::wstring& stateName, Relevance relevance ) : CBaseNode( relevance ), m_stateId( stateId ), m_stateName( stateName ) {}

	virtual ThemeNode GetThemeNode( void ) const { return State; }
	virtual std::tstring GetCode( void ) const { return m_stateName; }
public:
	int m_stateId;
	std::wstring m_stateName;
};


struct CThemePart : public CBaseNode
{
	CThemePart( int partId, const std::wstring& partName, Relevance relevance ) : CBaseNode( relevance ), m_partId( partId ), m_partName( partName ) {}

	virtual ThemeNode GetThemeNode( void ) const { return Part; }
	virtual std::tstring GetCode( void ) const { return m_partName; }
	CThemePart* AddState( int stateId, const std::wstring& stateName, Relevance relevance = HighRelevance );
	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );
public:
	int m_partId;
	std::wstring m_partName;
	std::vector< CThemeState > m_states;
};


struct CThemeClass : public CBaseNode
{
	CThemeClass( const std::wstring& className, Relevance relevance ) : CBaseNode( relevance ), m_className( className ) {}
	~CThemeClass() { utl::ClearOwningContainer( m_parts ); }

	virtual ThemeNode GetThemeNode( void ) const { return Class; }
	virtual std::tstring GetCode( void ) const { return m_className; }
	CThemePart* AddPart( int partId, const std::wstring& partName, Relevance relevance = HighRelevance );
	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );
public:
	std::wstring m_className;
	std::vector< CThemePart* > m_parts;
};


struct CThemeStore
{
	CThemeStore( void ) { RegisterStandardClasses(); }
	~CThemeStore() { utl::ClearOwningContainer( m_classes ); }

	CThemeClass* FindClass( const wchar_t* pClassName ) const;
	bool SetupNotImplementedThemes( void );
private:
	CThemeClass* AddClass( const std::wstring& className, Relevance relevance = HighRelevance );
	void RegisterStandardClasses( void );
public:
	std::vector< CThemeClass* > m_classes;
};


namespace func
{
	struct AsRelevance
	{
		Relevance operator()( const IThemeNode* pThemeNode ) const { return pThemeNode->GetRelevance(); }
		Relevance operator()( const utl::ISubject* pSubject ) const { return static_cast< const IThemeNode* >( pSubject )->GetRelevance(); }
	};
}

namespace pred
{
	typedef CompareScalarAdapterPtr< func::AsRelevance > CompareRelevance;
}


#endif // ThemeStore_h
