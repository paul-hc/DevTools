#ifndef CmdLineOptions_h
#define CmdLineOptions_h
#pragma once

#include "utl/Encoding.h"
#include "utl/Path.h"
#include "CmdLineOptions_fwd.h"


class CRuntimeException;
enum GuidesProfileType;


struct CCmdLineOptions
{
	CCmdLineOptions( void );
	~CCmdLineOptions();

	bool HasOptionFlag( app::TOption flag ) const { return HasFlag( m_optionFlags, flag ); }

	void ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException );
private:
	void PostProcessArguments( void ) throws_( CRuntimeException );

	enum CaseCvt { AsIs, UpperCase, LowerCase };

	static bool ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt = AsIs );

	void ThrowInvalidArgument( void ) throws_( CRuntimeException );
private:
	const TCHAR* m_pArg;				// current argument parsed
public:
	app::TOption m_optionFlags;
	fs::CPath m_dirPath;
	GuidesProfileType m_guidesProfileType;
	size_t m_maxDepthLevel;
	size_t m_maxDirFiles;
	fs::Encoding m_fileEncoding;
};


#endif // CmdLineOptions_h
