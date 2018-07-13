#ifndef GeneralOptions_h
#define GeneralOptions_h
#pragma once

#include "utl/Subject.h"
#include "FileCommands_fwd.h"


class CReportListControl;


struct CGeneralOptions : public CSubject
{
	CGeneralOptions( void );
	~CGeneralOptions();

	static CGeneralOptions& Instance( void );	// shared instance

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	bool operator==( const CGeneralOptions& right ) const;
	bool operator!=( const CGeneralOptions& right ) const { return !operator==( right ); }

	void ApplyToListCtrl( CReportListControl* pListCtrl ) const;
public:
	// file lists thumbs
	int m_smallIconDim, m_largeIconDim;
	bool m_useListThumbs;
	bool m_useListDoubleBuffer;

	// Undo/Redo
	bool m_undoLogPersist;
	cmd::FileFormat m_undoLogFormat;
	bool m_undoEditingCmds;
};


#endif // GeneralOptions_h
