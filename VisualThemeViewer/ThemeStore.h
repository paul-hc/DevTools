#ifndef ThemeStore_h
#define ThemeStore_h
#pragma once

#include "utl/ComparePredicates.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/Subject.h"
#include "utl/ThemeItem.h"


class CVisualTheme;


enum Relevance { HighRelevance, MediumRelevance, ObscureRelevance, NotImplemented = ObscureRelevance };
const CEnumTags& GetTags_Relevance( void );


interface IThemeNode : public utl::ISubject
{
	enum NodeType { Class = 1, Part, State };

	virtual NodeType GetNodeType( void ) const = 0;
	virtual Relevance GetRelevance( void ) const = 0;

	virtual const IThemeNode* GetParentNode( void ) const = 0;
	virtual CThemeItem MakeThemeItem( void ) const = 0;			// for previewing any node
};


abstract class CBaseNode : public CSubjectImpl< IThemeNode >
{
protected:
	CBaseNode( Relevance relevance ) : m_relevance( relevance ), m_pParentNode( NULL ) {}
public:
	virtual Relevance GetRelevance( void ) const { return m_relevance; }
	virtual const IThemeNode* GetParentNode( void ) const { return m_pParentNode; }

	void SetRelevance( Relevance relevance ) { m_relevance = relevance; }
	void SetParentNode( IThemeNode* pParentNode ) { m_pParentNode = pParentNode; }
private:
	Relevance m_relevance;
	IThemeNode* m_pParentNode;
};


struct CThemeState : public CBaseNode
{
	CThemeState( int stateId, const std::wstring& stateName, Relevance relevance ) : CBaseNode( relevance ), m_stateId( stateId ), m_stateName( stateName ) {}

	virtual NodeType GetNodeType( void ) const { return State; }
	virtual const std::tstring& GetCode( void ) const { return m_stateName; }
	virtual CThemeItem MakeThemeItem( void ) const;
public:
	int m_stateId;
	std::wstring m_stateName;
};


struct CThemePart : public CBaseNode
{
	CThemePart( int partId, const std::wstring& partName, Relevance relevance ) : CBaseNode( relevance ), m_partId( partId ), m_partName( partName ) {}
	~CThemePart() { utl::ClearOwningContainer( m_states ); }

	virtual NodeType GetNodeType( void ) const { return Part; }
	virtual const std::tstring& GetCode( void ) const { return m_partName; }
	virtual CThemeItem MakeThemeItem( void ) const;

	CThemePart* AddState( int stateId, const std::wstring& stateName, Relevance relevance = HighRelevance );
	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );
public:
	int m_partId;
	std::wstring m_partName;
	std::vector< CThemeState* > m_states;
private:
	CThemeState* m_pPreviewState;
};


struct CThemeClass : public CBaseNode
{
	CThemeClass( const std::wstring& className, Relevance relevance ) : CBaseNode( relevance ), m_className( className ) {}
	~CThemeClass() { utl::ClearOwningContainer( m_parts ); }

	virtual NodeType GetNodeType( void ) const { return Class; }
	virtual const std::tstring& GetCode( void ) const { return m_className; }
	virtual CThemeItem MakeThemeItem( void ) const;

	CThemePart* AddPart( int partId, const std::wstring& partName, Relevance relevance = HighRelevance );

	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );
public:
	std::wstring m_className;
	std::vector< CThemePart* > m_parts;
	CThemePart* m_pPreviewPart;
	CThemeState* m_pPreviewState;
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
