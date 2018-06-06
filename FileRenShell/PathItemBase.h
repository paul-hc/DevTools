#ifndef PathItemBase_h
#define PathItemBase_h
#pragma once

#include "utl/Subject.h"
#include "utl/Path.h"


abstract class CPathItemBase : public CSubject
{
protected:
	CPathItemBase( const fs::CPath& keyPath );
public:
	virtual ~CPathItemBase();

	const fs::CPath& GetKeyPath( void ) const { return m_keyPath; }
	void StripDisplayCode( const fs::CPath& commonParentPath );

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	struct ToKeyPath
	{
		const fs::CPath& operator()( const fs::CPath& path ) const { return path; }
		const fs::CPath& operator()( const CPathItemBase* pItem ) const { return pItem->GetKeyPath(); }
	};
private:
	fs::CPath m_keyPath;
	std::tstring m_displayPath;
};


namespace func
{
	struct ResetItem
	{
		template< typename ItemType >
		void operator()( ItemType* pItem )
		{
			pItem->Reset();
		}
	};


	struct StripDisplayCode
	{
		StripDisplayCode( const fs::CPath& commonParentPath ) : m_commonParentPath( commonParentPath ) {}

		void operator()( CPathItemBase* pItem ) const
		{
			pItem->StripDisplayCode( m_commonParentPath );
		}
	private:
		const fs::CPath& m_commonParentPath;			// to be stripped
	};
}


#endif // PathItemBase_h
