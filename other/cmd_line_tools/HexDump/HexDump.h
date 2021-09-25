#ifndef HexDump_h
#define HexDump_h
#pragma once

#include <string>
#include <iosfwd>


namespace io
{
	enum { DefaultRowByteCount = 16 };

	void HexDump( std::ostream& os, const std::string& textPath, size_t rowByteCount = DefaultRowByteCount );
}


#endif // HexDump_h
