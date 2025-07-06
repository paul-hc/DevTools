#ifndef FlagRepository_h
#define FlagRepository_h
#pragma once

#include "FlagStore.h"


class CStyleRepository
{
	CStyleRepository( void );
	~CStyleRepository();
public:
	static CStyleRepository& Instance( void );

	const CFlagStore* GetGeneralStore( bool isChild ) const { return isChild ? &m_generalChildStore : &m_generalTopLevelStore; }

	const std::vector<CFlagStore*>& GetSpecificStores( void ) const { return m_specificStores; }
	const CFlagStore* FindSpecificStore( const std::tstring& wndClass ) const;

	std::tstring& StreamStyle( std::tstring& rOutput, DWORD style, const std::tstring& wndClass, const TCHAR* pSep = stream::flagSep ) const;
private:
	void AddFlagStore( const TCHAR* pWndClassAliases, CFlagInfo pFlagInfos[], unsigned int count );

	// field access
	static DWORD GetWindowStyle( HWND hSrc );
	static DWORD SetWindowStyle( HWND hDest, DWORD style );
private:
	CFlagStore m_generalTopLevelStore;
	CFlagStore m_generalChildStore;
	std::vector<CFlagStore*> m_specificStores;
};


class CStyleExRepository
{
	CStyleExRepository( void );
	~CStyleExRepository();
public:
	static CStyleExRepository& Instance( void );

	const std::vector<CFlagStore*>& GetStores( void ) const { return m_stores; }
	const CFlagStore* FindStore( const std::tstring& wndClass = std::tstring() ) const;

	std::tstring& StreamStyleEx( std::tstring& rOutput, DWORD styleEx, const std::tstring& wndClass = std::tstring(), const TCHAR* pSep = stream::flagSep ) const;
private:
	// field access
	static DWORD GetWindowStyleEx( HWND hSrc );
	static DWORD SetWindowStyleEx( HWND hDest, DWORD styleEx );
	static DWORD GetListViewStyleEx( HWND hSrc );
	static DWORD SetListViewStyleEx( HWND hDest, DWORD styleExClass );
	static DWORD GetTreeViewStyleEx( HWND hSrc );
	static DWORD SetTreeViewStyleEx( HWND hDest, DWORD styleExClass );
	static DWORD GetTabCtrlStyleEx( HWND hSrc );
	static DWORD SetTabCtrlStyleEx( HWND hDest, DWORD styleExClass );
	static DWORD GetComboExStyleEx( HWND hSrc );
	static DWORD SetComboExStyleEx( HWND hDest, DWORD styleExClass );
	static DWORD GetToolbarStyleEx( HWND hSrc );
	static DWORD SetToolbarStyleEx( HWND hDest, DWORD styleExClass );
private:
	std::vector<CFlagStore*> m_stores;
};


#endif // FlagRepository_h
