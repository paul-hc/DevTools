#ifndef FlagStore_h
#define FlagStore_h
#pragma once

#include "utl/UI/WndUtils.h"
#include "WindowClass.h"


namespace stream
{
	extern const TCHAR flagSep[];
	extern const TCHAR maskSep[];
}


struct CFlagGroup;


struct CFlagInfo
{
	enum Type { Editable, ReadOnly, Separator };

	bool IsBitFlag( void ) const { return m_value != 0 && m_mask == 0; }
	bool IsValue( void ) const { return m_mask != 0; }
	bool IsReadOnly( void ) const { return ReadOnly == m_type; }
	bool IsSeparator( void ) const { return Separator == m_type; }
	bool IsExclusiveWith( const CFlagInfo& mutual ) const { return m_mask == mutual.m_mask && m_value != mutual.m_value; }

	bool IsOn( DWORD flags ) const;
	bool SetTo( DWORD* pFlags, bool on = true ) const;
	void Toggle( DWORD* pFlags ) const { SetTo( pFlags, !IsOn( *pFlags ) ); }

	const std::tstring& GetName( void ) const { LazyInit(); return m_name; }
	const std::tstring& GetAliases( void ) const { LazyInit(); return m_aliases; }

	bool operator==( const CFlagInfo& cmp ) const { return m_mask == cmp.m_mask && m_value == cmp.m_value; }
	bool operator!=( const CFlagInfo& cmp ) const { return !operator==( cmp ); }

	bool StreamFlagName( std::tstring& rOutput, DWORD flags, const TCHAR* pSep, bool withAliases = true ) const;		// conditional if this is on in flags
private:
	void LazyInit( void ) const;
public:
	DWORD m_value;
	DWORD m_mask;
	Type m_type;
	const TCHAR* m_pRawTag;

	CFlagGroup* m_pGroup;

//[private:]
	mutable std::tstring m_name, m_aliases;		// lazy initialization
};


struct CFlagGroup
{
	CFlagGroup( void ) : m_pSeparator( nullptr ) {}
	CFlagGroup( CFlagInfo* pSeparator ) : m_pSeparator( pSeparator ) { m_pSeparator->m_pGroup = this; }

	DWORD GetMask( void ) const { return m_flagInfos.front()->m_mask; }

	const std::tstring& GetName( const std::tstring& anonymousName ) const
	{
		return m_pSeparator != nullptr && !m_pSeparator->GetName().empty() ? m_pSeparator->GetName() : anonymousName;
	}

	const std::vector< const CFlagInfo* >& GetFlags( void ) const { return m_flagInfos; }

	bool CanGroup( const CFlagInfo* pFlagInfo ) const;
	void AddToGroup( CFlagInfo* pFlagInfo );

	const CFlagInfo* FindOnFlag( DWORD flags ) const;

	bool IsValueGroup( void ) const { return m_flagInfos.front()->IsValue(); }
	const CFlagInfo* FindOnValue( DWORD flags ) const { ASSERT( IsValueGroup() ); return FindOnFlag( flags ); }

	bool IsReadOnlyGroup( void ) const;
public:
	CFlagInfo* m_pSeparator;
private:
	std::vector< const CFlagInfo* > m_flagInfos;
};


typedef	DWORD (*TGetWindowFieldFunc)( HWND hSrc );
typedef	DWORD (*TSetWindowFieldFunc)( HWND hDest, DWORD flags );


struct CFlagStore
{
	CFlagStore( const TCHAR* pWndClassAliases, CFlagInfo flagInfos[], unsigned int count, TGetWindowFieldFunc pGetFunc, TSetWindowFieldFunc pSetFunc );
	~CFlagStore();

	const std::tstring& GetWndClass( void ) const { return m_wndClasses.front(); }
	DWORD GetMask( void ) const { return m_mask; }
	DWORD GetReadOnlyMask( void ) const { return m_mask & ~m_editableMask; }
	bool HasGroups( void ) const { return !m_groups.empty(); }

	bool SameFieldWith( const CFlagStore& right ) const { return m_pGetFunc == right.m_pGetFunc; }

	void StreamFormatFlags( std::tstring& rOutput, DWORD flags, const TCHAR* pSep = stream::flagSep ) const;
	void StreamFormatMask( std::tstring& rOutput, const TCHAR* pSep = stream::maskSep ) const;
	static void StreamMask( std::tstring& rOutput, DWORD mask, const TCHAR* pSep = stream::maskSep );

	int CheckFlagsTransition( DWORD newFlags, DWORD oldFlags ) const;

	DWORD GetWindowField( HWND hSrc ) const
	{
		ASSERT_PTR( m_pGetFunc );
		return m_pGetFunc( hSrc );
	}

	DWORD SetWindowField( HWND hDest, DWORD flags ) const
	{
		ASSERT_PTR( m_pSetFunc );
		return m_pSetFunc( hDest, flags );
	}
private:
	bool NeedsGroups( const CFlagInfo flagInfos[], unsigned int count ) const;
public:
	std::vector< std::tstring > m_wndClasses;			// class aliases: family of window classes that share the same flag definitions
	std::vector< CFlagInfo* > m_flagInfos;
	std::vector< CFlagGroup* > m_groups;
private:
	DWORD m_mask;
	DWORD m_editableMask;
	TGetWindowFieldFunc m_pGetFunc;
	TSetWindowFieldFunc m_pSetFunc;
};


#endif // FlagStore_h
