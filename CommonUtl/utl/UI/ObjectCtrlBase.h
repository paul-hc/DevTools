#ifndef ObjectCtrlBase_h
#define ObjectCtrlBase_h
#pragma once

#include "ISubject.h"


namespace ui
{
	interface ISubjectAdapter
	{
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const = 0;
	};


	class CCodeAdapter : public ui::ISubjectAdapter
	{
	public:
		static ui::ISubjectAdapter* Instance( void );

		// ui::ISubjectAdapter interface
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;
	};


	class CDisplayCodeAdapter : public ui::CCodeAdapter
	{
	public:
		static ui::ISubjectAdapter* Instance( void );

		// ui::ISubjectAdapter interface
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;
	};
}


#include "AccelTable.h"
#include "CmdIdStore.h"
#include "InternalChange.h"


namespace fs { class CPath; }
class CShellContextMenuHost;


abstract class CObjectCtrlBase : public CInternalChange
{
protected:
	CObjectCtrlBase( CWnd* pCtrl, UINT ctrlAccelId = 0 );
	~CObjectCtrlBase();
public:
	ui::ISubjectAdapter* GetSubjectAdapter( void ) const { return m_pSubjectAdapter; }
	void SetSubjectAdapter( ui::ISubjectAdapter* pSubjectAdapter );

	std::tstring FormatCode( const utl::ISubject* pSubject ) const { ASSERT_PTR( m_pSubjectAdapter ); return m_pSubjectAdapter->FormatCode( pSubject ); }

	CAccelTable& GetCtrlAccel( void ) { return m_ctrlAccel; }
	void SetTrackMenuTarget( CWnd* pTrackMenuTarget ) { m_pTrackMenuTarget = pTrackMenuTarget; }

	virtual bool IsInternalCmdId( int cmdId ) const;
protected:
	bool HandleCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );		// must be called from control class' OnCmdMsg() method override
	bool TranslateMessage( MSG* pMsg );

	// shell context menu hosting/tracking
public:
	enum ShellContextMenuStyle { NoShellMenu, ExplorerSubMenu, ShellMenuFirst, ShellMenuLast };

	bool UseShellContextMenu( void ) const { return m_shCtxStyle != NoShellMenu; }
	void SetShellContextMenuStyle( ShellContextMenuStyle shCtxStyle, UINT shCtxQueryFlags = UINT_MAX );

	bool IsShellMenuCmd( int cmdId ) const;
protected:
	CMenu* MakeContextMenuHost( CMenu* pSrcPopupMenu, const std::vector< fs::CPath >& filePaths );
	bool DoTrackContextMenu( CMenu* pPopupMenu, const CPoint& screenPos );
	void ResetShellContextMenu( void );

	bool ShellInvokeDefaultVerb( const std::vector< fs::CPath >& filePaths );
	bool ShellInvokeProperties( const std::vector< fs::CPath >& filePaths );
private:
	ui::ISubjectAdapter* m_pSubjectAdapter;			// by default ui::CDisplayCodeAdapter
protected:
	CWnd* m_pCtrl;
	CWnd* m_pTrackMenuTarget;						// receiver of commands when tracking the context menu
	CAccelTable m_ctrlAccel;
	ui::CCmdIdStore m_internalCmdIds;				// contains only standard commands that the control handles itself (vs custom commands handled by parent)

	// shell context menu hosting/tracking
	ShellContextMenuStyle m_shCtxStyle;
	UINT m_shCtxQueryFlags;
	std::auto_ptr< CShellContextMenuHost > m_pShellMenuHost;
};


#endif // ObjectCtrlBase_h
