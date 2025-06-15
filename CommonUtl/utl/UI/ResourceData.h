#ifndef ResourceData_h
#define ResourceData_h
#pragma once

#include <ole2.h>


class CResourceData
{
public:
	CResourceData( const TCHAR* pResName = nullptr, const TCHAR* pResType = nullptr );
	~CResourceData();

	bool LoadResource( const TCHAR* pResName, const TCHAR* pResType );
	bool LoadResource( UINT resId, const TCHAR* pResType ) { return LoadResource( MAKEINTRESOURCE( resId ), pResType ); }
	void Clear( void );

	bool IsValid( void ) const { return m_pResource != nullptr; }
	DWORD GetSize( void ) const { return m_pResource != nullptr ? ::SizeofResource( m_hInst, m_hResource ) : 0; }

	template< typename ResourceType >
	ResourceType* GetResource( void ) const { return static_cast<ResourceType*>( m_pResource ); }

	CComPtr<IStream> CreateStreamCopy( void ) const;		// delete on Release()
private:
	HINSTANCE m_hInst;
	HRSRC m_hResource;
	HGLOBAL m_hGlobal;
	void* m_pResource;
};


#endif // ResourceData_h
