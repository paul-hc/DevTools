#ifndef CommandSvcHandler_h
#define CommandSvcHandler_h
#pragma once

#include "utl/ICommand.h"
#include "utl/CommandModel.h"


namespace ui
{
	// Light-weight command service that handles command execution, ID_EDIT_UNDO and ID_EDIT_REDO.
	//	Suitable for local undo/redo implementation.

	class CCommandSvcHandler
		: public CCmdTarget
		, public utl::ICommandExecutor
	{
	public:
		CCommandSvcHandler( size_t maxSize = 20 );
		virtual ~CCommandSvcHandler();

		bool IsDirty( void ) const { return m_dirty; }
		const CCommandModel* GetCmdModel( void ) const { return &m_cmdModel; }
		CCommandModel* RefCmdModel( void ) { return &m_cmdModel; }

		// utl::ICommandExecutor interface
		virtual bool Execute( utl::ICommand* pCmd ) implement;

		// persistence
		bool LoadState( const TCHAR* pRegSection, const TCHAR* pEntry );
		bool SaveState( const TCHAR* pRegSection, const TCHAR* pEntry ) const;
		bool DeleteStateFromRegistry( const TCHAR* pRegSection, const TCHAR* pEntry );

		// base overrides
		virtual void Serialize( CArchive& archive ) overrides(CObject);
	private:
		CCommandModel m_cmdModel;
		size_t m_maxSize;
		bool m_dirty;
	protected:
		afx_msg void On_EditUndo( void );
		afx_msg void OnUpdate_EditUndo( CCmdUI* pCmdUI );
		afx_msg void On_EditRedo( void );
		afx_msg void OnUpdate_EditRedo( CCmdUI* pCmdUI );

		DECLARE_MESSAGE_MAP()
	};
}


#endif // CommandSvcHandler_h
