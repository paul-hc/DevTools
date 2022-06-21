#ifndef GeneralOptions_h
#define GeneralOptions_h
#pragma once

#include "utl/Subject.h"
#include "AppCommands_fwd.h"


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
	persist int m_smallIconDim, m_largeIconDim;
	persist bool m_useListThumbs;
	persist bool m_useListDoubleBuffer;
	persist bool m_highlightTextDiffsFrame;

	// Undo/Redo
	cmd::FileFormat m_undoLogFormat;			// disabled persistence since removed cmd::TextFormat
	persist bool m_undoLogPersist;
	persist bool m_undoEditingCmds;

	// Filename text processing
	persist bool m_trimFname;
	persist bool m_normalizeWhitespace;			// ensure single whitespaces
};


#endif // GeneralOptions_h
