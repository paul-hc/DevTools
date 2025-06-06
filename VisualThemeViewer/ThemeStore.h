#ifndef ThemeStore_h
#define ThemeStore_h
#pragma once

#include "utl/ComparePredicates.h"
#include "utl/EnumTags.h"
#include "utl/Subject.h"
#include "utl/UI/ThemeItem.h"


class CVisualTheme;


enum Relevance { HighRelevance, MediumRelevance, ObscureRelevance, NotImplemented = ObscureRelevance };
const CEnumTags& GetTags_Relevance( void );


enum Flag
{
	TextFlag				= BIT_FLAG( 0 ),
	SquareContentFlag		= BIT_FLAG( 1 ),
	ShrinkFitContentFlag	= BIT_FLAG( 2 ),
	PreviewFlag				= BIT_FLAG( 3 ),
	PreviewShallowFlag		= BIT_FLAG( 4 ),
	PreviewFillBkFlag		= BIT_FLAG( 5 )							// fills with COLOR_BTNFACE for themes that alpha-blend with background
};


interface IThemeNode;


struct CThemeItemNode : public CThemeItem
{
	CThemeItemNode( const wchar_t* pThemeClass, int partId, int stateId, const IThemeNode* pDeepNode ) : CThemeItem( pThemeClass, partId, stateId ), m_pDeepNode( pDeepNode ) {}
public:
	const IThemeNode* m_pDeepNode;		// for inheriting flags: the deepest node from which the theme item originates
};


interface IThemeNode : public utl::ISubject
{
	enum NodeType { Class = 1, Part, State };

	virtual NodeType GetNodeType( void ) const = 0;
	virtual Relevance GetRelevance( void ) const = 0;
	virtual int GetFlags( void ) const = 0;

	virtual IThemeNode* GetParentNode( void ) const = 0;
	virtual CThemeItemNode MakeThemeItem( void ) const = 0;			// for previewing any node
};


abstract class CBaseNode : public CSubjectImpl<IThemeNode>
{
protected:
	CBaseNode( Relevance relevance, int flags ) : m_relevance( relevance ), m_pParentNode( nullptr ), m_flags( flags ) {}
public:
	virtual Relevance GetRelevance( void ) const { return m_relevance; }
	virtual int GetFlags( void ) const { return m_flags; }
	virtual IThemeNode* GetParentNode( void ) const { return m_pParentNode; }

	void SetRelevance( Relevance relevance ) { m_relevance = relevance; }
	void SetFlags( int flags ) { m_flags = flags; }
	void SetParentNode( IThemeNode* pParentNode ) { m_pParentNode = pParentNode; }

	template< typename NodeT >
	NodeT* GetParentAs( void ) const { return const_cast<NodeT*>( checked_static_cast<const NodeT*>( GetParentNode() ) ); }

	bool ModifyFlags( unsigned int clearFlags, unsigned int setFlags ) { return ::ModifyFlags( m_flags, clearFlags, setFlags ); }
private:
	Relevance m_relevance;
	IThemeNode* m_pParentNode;
	int m_flags;
};


struct CThemeState : public CBaseNode
{
	CThemeState( int stateId, const std::wstring& stateName, Relevance relevance, int flags )
		: CBaseNode( relevance, flags ), m_stateId( stateId ), m_stateName( stateName ) {}

	virtual const std::tstring& GetCode( void ) const { return m_stateName; }
	virtual NodeType GetNodeType( void ) const { return State; }
	virtual CThemeItemNode MakeThemeItem( void ) const;
public:
	int m_stateId;
	std::wstring m_stateName;
};


struct CThemePart : public CBaseNode
{
	CThemePart( int partId, const std::wstring& partName, Relevance relevance, int flags )
		: CBaseNode( relevance, flags ), m_partId( partId ), m_partName( partName ), m_pPreviewState( nullptr ) {}

	~CThemePart();

	virtual const std::tstring& GetCode( void ) const { return m_partName; }
	virtual NodeType GetNodeType( void ) const { return Part; }
	virtual CThemeItemNode MakeThemeItem( void ) const;

	CThemePart* AddState( int stateId, const std::wstring& stateName, int flags = 0, Relevance relevance = HighRelevance );
	void SetDeepFlags( unsigned int addFlags );

	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );

	const CThemeState* GetPreviewState( void ) const;
	void SetPreviewState( const CThemeState* pPreviewState, RecursionDepth depth );
public:
	int m_partId;
	std::wstring m_partName;
	std::vector<CThemeState*> m_states;
private:
	mutable const CThemeState* m_pPreviewState;		// self-encapsulated through GetPreviewState()
};


struct CThemeClass : public CBaseNode
{
	CThemeClass( const std::wstring& className, Relevance relevance ) : CBaseNode( relevance, 0 ), m_className( className ), m_pPreviewPart( nullptr ) {}
	~CThemeClass();

	virtual const std::tstring& GetCode( void ) const { return m_className; }
	virtual NodeType GetNodeType( void ) const { return Class; }
	virtual CThemeItemNode MakeThemeItem( void ) const;

	CThemePart* AddPart( int partId, const std::wstring& partName, int flags = 0, Relevance relevance = HighRelevance );
	void SetDeepFlags( unsigned int addFlags );
	IThemeNode* FindNode( const std::wstring& code ) const;

	bool SetupNotImplemented( CVisualTheme& rTheme, HDC hDC );

	const CThemePart* GetPreviewPart( void ) const;
	void SetPreviewPart( const CThemePart* pPreviewPart ) { ASSERT_PTR( pPreviewPart ); m_pPreviewPart = pPreviewPart; }
public:
	std::wstring m_className;
	std::vector<CThemePart*> m_parts;
private:
	mutable const CThemePart* m_pPreviewPart;		// self-encapsulated through GetPreviewPart()
};


struct CThemeStore
{
	CThemeStore( void ) { RegisterStandardClasses(); }
	~CThemeStore();

	CThemeClass* FindClass( const std::wstring& className ) const;
	IThemeNode* FindNode( const std::wstring& code ) const;
	bool SetupNotImplementedThemes( void );

	size_t GetTotalCount( void ) const;
private:
	CThemeClass* AddClass( const std::wstring& className, Relevance relevance = HighRelevance );
	void RegisterStandardClasses( void );
public:
	std::vector<CThemeClass*> m_classes;
};


namespace func
{
	struct AsRelevance
	{
		Relevance operator()( const IThemeNode* pThemeNode ) const { return pThemeNode->GetRelevance(); }
		Relevance operator()( const utl::ISubject* pSubject ) const { return static_cast<const IThemeNode*>( pSubject )->GetRelevance(); }
	};
}

namespace pred
{
	typedef CompareScalarAdapterPtr<func::AsRelevance> TCompareRelevance;
}


#endif // ThemeStore_h
