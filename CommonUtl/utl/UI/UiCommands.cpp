
#include "pch.h"
#include "UiCommands.h"
#include "PopupMenus_fwd.h"
#include "utl/EnumTags.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_UiCommandType( void )
	{
		static CEnumTags s_tags( -1, SetColor );

		if ( s_tags.IsEmpty() )
		{
			s_tags.AddTagPair( _T("Set Color"), _T("SET_COLOR") );
		}

		return s_tags;
	}
}


// CSetColorCmd implementation

IMPLEMENT_SERIAL( CSetColorCmd, CObject, VERSIONABLE_SCHEMA | 1 );

CSetColorCmd::CSetColorCmd( ui::IColorEditorHost* pEditorHost /*= nullptr*/ , COLORREF color /*= CLR_DEFAULT*/ )
	: CCommand( cmd::SetColor, nullptr )
	, m_color( color )
	, m_oldColor( CLR_DEFAULT )
	, m_pEditorHost( pEditorHost )
{
}

void CSetColorCmd::Serialize( CArchive& archive )
{
	CCommand::Serialize( archive );

	archive & m_color;
	archive & m_oldColor;
}

bool CSetColorCmd::Execute( void ) implement
{
	ASSERT_PTR( m_pEditorHost );

	m_oldColor = m_pEditorHost->GetColor();

	if ( CLR_DEFAULT == m_color )
		return false;

	m_pEditorHost->SetColor( m_color, true );
	return true;
}

bool CSetColorCmd::Unexecute( void ) implement
{
	if ( CLR_DEFAULT == m_oldColor )
		return false;

	m_pEditorHost->SetColor( m_oldColor, true );
	return true;
}
