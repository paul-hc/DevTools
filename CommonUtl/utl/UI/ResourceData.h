#ifndef ResourceData_h
#define ResourceData_h
#pragma once

#include <ole2.h>


class CResourceData
{
public:
	CResourceData( const TCHAR* pResId, const TCHAR* pResType );
	~CResourceData();

	bool IsValid( void ) const { return m_pResource != nullptr; }
	DWORD GetSize( void ) const { return m_pResource != nullptr ? ::SizeofResource( m_hInst, m_hResource ) : 0; }

	template< typename ResourceType >
	const ResourceType* GetResource( void ) const { return static_cast<const ResourceType*>( m_pResource ); }

	CComPtr<IStream> CreateStreamCopy( void ) const;		// delete on Release()
private:
	HINSTANCE m_hInst;
	HRSRC m_hResource;
	HGLOBAL m_hGlobal;
	void* m_pResource;
};


#endif // ResourceData_h
