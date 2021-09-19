#ifndef CmdLineOptions_h
#define CmdLineOptions_h
#pragma once

#include <vector>
#include "utl/Path.h"
#include "CmdLineOptions_fwd.h"


class CRuntimeException;


struct CCmdLineOptions
{
	CCmdLineOptions( void );
	~CCmdLineOptions();

	void ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException );
private:
	void PostProcessArguments( void ) throws_( CRuntimeException );
	void ParseBuildTimestamp( const std::tstring& value ) throws_( CRuntimeException );

	enum CaseCvt { AsIs, UpperCase, LowerCase };

	static bool ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt = AsIs );

	void ThrowInvalidArgument( void ) throws_( CRuntimeException );
private:
	const TCHAR* m_pArg;								// current argument parsed
public:
	app::TOption m_optionFlags;
	fs::CPath m_targetRcPath;
	CTime m_buildTimestamp;
};


#endif // CmdLineOptions_h
