#ifndef GeneralOptions_h
#define GeneralOptions_h
#pragma once

#include "utl/Subject.h"
#include "utl/Image_fwd.h"


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
public:
	IconStdSize m_smallIconStdSize;
	IconStdSize m_largeIconStdSize;
};


#endif // GeneralOptions_h
