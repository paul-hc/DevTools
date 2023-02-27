#ifndef TextFileIo_fwd_h
#define TextFileIo_fwd_h
#pragma once


namespace io
{
	template< typename StringT >
	interface ILineParserCallback
	{
		virtual bool OnParseLine( const StringT& line, unsigned int lineNo ) = 0;		// return false to stop parsing

		virtual void OnBeginParsing( void ) {}
		virtual void OnEndParsing( void ) {}
	};
}


#endif // TextFileIo_fwd_h
