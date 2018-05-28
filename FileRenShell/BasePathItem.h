#ifndef BasePathItem_h
#define BasePathItem_h
#pragma once

#include "utl/Subject.h"
#include "utl/Path.h"


namespace fmt { enum PathFormat; }


abstract class CBasePathItem : public CSubject
{
protected:
	CBasePathItem( const fs::CPath& keyPath, fmt::PathFormat fmtDisplayPath );
public:
	virtual ~CBasePathItem();

	const fs::CPath& GetKeyPath( void ) const { return m_keyPath; }

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;
private:
	fs::CPath m_keyPath;
	const std::tstring m_displayPath;
};


#endif // BasePathItem_h
