#ifndef GeneralOptions_h
#define GeneralOptions_h
#pragma once

#include "utl/Subject.h"


class CReportListControl;


struct CGeneralOptions : public CSubject
{
	CGeneralOptions( void );
	~CGeneralOptions();

	static CGeneralOptions& Instance( void );	// shared instance

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	bool operator==( const CGeneralOptions& right ) const;
	bool operator!=( const CGeneralOptions& right ) const { return !operator==( right ); }

	void ApplyToListCtrl( CReportListControl* pListCtrl ) const;
public:
	bool m_useListThumbs;
	int m_smallIconDim, m_largeIconDim;
};


#endif // GeneralOptions_h
