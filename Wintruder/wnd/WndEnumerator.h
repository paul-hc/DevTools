#ifndef WndEnumerator_h
#define WndEnumerator_h
#pragma once


abstract class CWndEnumBase
{
protected:
	CWndEnumBase( void );
public:
	void Build( HWND hRootWnd );
	void BuildChildren( HWND hWnd );

	bool IsValidMatch( HWND hWnd ) const;
protected:
	virtual void AddWndItem( HWND hWnd ) = 0;
private:
	static BOOL CALLBACK EnumWindowProc( HWND hTopLevel, CWndEnumBase* pEnumerator );
	static BOOL CALLBACK EnumChildWindowProc( HWND hWnd, CWndEnumBase* pEnumerator );
private:
	DWORD m_appProcessId;
};


class CWndEnumerator : public CWndEnumBase
{
public:
	CWndEnumerator( void ) {}

	const std::vector<HWND>& GetWindows( void ) const { return m_windows; }
protected:
	virtual void AddWndItem( HWND hWnd );
private:
	std::vector<HWND> m_windows;
};


namespace wnd
{
	void QueryAncestorBranchPath( std::vector<HWND>& rBranchPath, HWND hWnd, HWND hWndAncestor = ::GetDesktopWindow() );		// bottom-up ancestor path order from [hWnd, ..., hWndAncestor]
}


#endif // WndEnumerator_h
