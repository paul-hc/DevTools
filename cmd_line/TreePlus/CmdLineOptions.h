#ifndef CmdLineOptions_h
#define CmdLineOptions_h
#pragma once

#include "utl/Encoding.h"
#include "utl/Path.h"
#include "CmdLineOptions_fwd.h"


class CRuntimeException;
class CTable;
enum GuidesProfileType;


struct CCmdLineOptions
{
	CCmdLineOptions( void );
	~CCmdLineOptions();

	bool HasOptionFlag( app::Option flag ) const { return m_optionFlags.Has( flag ); }

	void ParseCommandLine( int argc, const TCHAR* const argv[] ) throws_( CRuntimeException );

	const CTable* GetTable( void ) const { return m_pTable.get(); }
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
	std::auto_ptr<CTable> m_pTable;
	GuidesProfileType m_guidesProfileType;
	size_t m_maxDepthLevel;
	size_t m_maxDirFiles;
	fs::Encoding m_fileEncoding;
	fs::CPath m_outputFilePath;
};


#endif // CmdLineOptions_h
