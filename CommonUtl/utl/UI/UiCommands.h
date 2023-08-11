#ifndef UiCommands_h
#define UiCommands_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace ui { interface IColorEditorHost; }


namespace cmd
{
	enum UiCommandType			// persistent commands - don't modify their values, so that it won't break serialization
	{
		SetColor = 50
	};

	const CEnumTags& GetTags_UiCommandType( void );
}


class CSetColorCmd : public CObject
	, public CCommand
{
	DECLARE_SERIAL( CSetColorCmd )
public:
	CSetColorCmd( ui::IColorEditorHost* pEditorHost = nullptr, COLORREF color = CLR_DEFAULT );

	COLORREF GetColor( void ) const { return m_color; }
	COLORREF GetOldColor( void ) const { return m_oldColor; }

	void SetHost( ui::IColorEditorHost* pEditorHost ) { ASSERT( nullptr == m_pEditorHost && pEditorHost != nullptr ); m_pEditorHost = pEditorHost; }

	// base overrides
	virtual void Serialize( CArchive& archive );

	virtual bool Execute( void ) implement;
	virtual bool Unexecute( void ) implement;
private:
	persist COLORREF m_color;
	persist COLORREF m_oldColor;

	ui::IColorEditorHost* m_pEditorHost;	// transient, must be re-parented on de-serialization
};


#endif // UiCommands_h
