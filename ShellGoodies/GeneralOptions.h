#ifndef GeneralOptions_h
#define GeneralOptions_h
#pragma once

#include "utl/Subject.h"
#include "AppCommands.h"


class CReportListControl;


struct CGeneralOptions : public TSubject
{
	CGeneralOptions( void );
	~CGeneralOptions();

	static CGeneralOptions& Instance( void );	// shared instance

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;
	void PostApply( void ) const;

	bool operator==( const CGeneralOptions& right ) const;
	bool operator!=( const CGeneralOptions& right ) const { return !operator==( right ); }

	void ApplyToListCtrl( CReportListControl* pListCtrl ) const;
public:
	// file lists
	int m_smallIconDim, m_largeIconDim;
	bool m_useListThumbs;
	bool m_useListDoubleBuffer;
	bool m_highlightTextDiffsFrame;

	// Undo/Redo
	bool m_undoLogPersist;
	cmd::FileFormat m_undoLogFormat;
	bool m_undoEditingCmds;

	// Filename text processing
	bool m_trimFname;
	bool m_normalizeWhitespace;			// ensure single whitespaces
};


#endif // GeneralOptions_h
