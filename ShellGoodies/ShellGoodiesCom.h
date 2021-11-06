#ifndef ShellGoodiesCom_h
#define ShellGoodiesCom_h
#pragma once

#include "resource.h"


class CShellMenuController;


class ATL_NO_VTABLE CShellGoodiesCom
	: public CComObjectRootEx< CComSingleThreadModel >
	, public CComCoClass< CShellGoodiesCom, &CLSID_ShellGoodiesCom >
	, public ISupportErrorInfo
	, public IShellExtInit
	, public IContextMenu
{
public:
	CShellGoodiesCom( void );
	~CShellGoodiesCom();
private:
	bool IsInit( void ) const { return m_pController.get() != NULL; }
	size_t ExtractDropInfo( IDataObject* pSelFileObjects );
private:
	std::auto_ptr<CShellMenuController> m_pController;	// controller for handling the selected files in Explorer

	// generated COM stuff
public:
	DECLARE_REGISTRY_RESOURCEID( IDR_SHELL_GOODIES_REG )

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP( CShellGoodiesCom )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
		COM_INTERFACE_ENTRY( IShellExtInit )
		COM_INTERFACE_ENTRY( IContextMenu )
	END_COM_MAP()
public:
	// ISupportErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

	// IShellExtInit
	STDMETHOD( Initialize )( LPCITEMIDLIST folderPidl, IDataObject* pSelFileObjects, HKEY hKeyProgId );

	// IContextMenu
	STDMETHOD( QueryContextMenu )( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags );
	STDMETHOD( InvokeCommand )( CMINVOKECOMMANDINFO* pCmi );
	STDMETHOD( GetCommandString )( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax );
};


#endif // ShellGoodiesCom_h
