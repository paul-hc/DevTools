#ifndef TouchItem_h
#define TouchItem_h
#pragma once

#include "utl/ISubject.h"
#include "utl/FileState.h"


class CTouchItem : public utl::ISubject
{
	typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;		// ptr stored in m_fileListCtrl
public:
	CTouchItem( TFileStatePair* pStatePair, bool showFullPath );
	virtual ~CTouchItem();

	const fs::CPath& GetKeyPath( void ) const { return GetSrcState().m_fullPath; }
	const fs::CFileState& GetSrcState( void ) const { return m_pStatePair->first; }
	const fs::CFileState& GetDestState( void ) const { return m_pStatePair->second; }

	bool IsModified( void ) const;

	// utl::ISubject interface
	virtual std::tstring GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	struct ToKeyPath
	{
		const fs::CPath& operator()( const fs::CPath& path ) const { return path; }
		const fs::CPath& operator()( const CTouchItem* pItem ) const { return pItem->GetKeyPath(); }
	};
private:
	TFileStatePair* m_pStatePair;
	const std::tstring m_displayPath;
};


#endif // TouchItem_h
