#ifndef Options_h
#define Options_h
#pragma once


class COptions
{
public:
	COptions( void );

	void ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException );
private:
	const TCHAR* m_pArg;								// current argument parsed
public:
	bool m_helpMode;
};


#endif // Options_h
