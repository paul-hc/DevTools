#ifndef Application_h
#define Application_h
#pragma once


struct CCmdLineOptions;


namespace app
{
	void RunMain( const CCmdLineOptions& options ) throws_( std::exception, CException* );
};


#endif // Application_h
