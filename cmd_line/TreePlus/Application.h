#ifndef Application_h
#define Application_h
#pragma once

#include <iosfwd>


struct CCmdLineOptions;


namespace app
{
	void RunMain( std::wstringstream& os, const CCmdLineOptions& options ) throws_( std::exception, CException* );
};


#endif // Application_h
