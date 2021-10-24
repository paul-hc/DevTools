#ifndef Io_fwd_h
#define Io_fwd_h
#pragma once


namespace fs { class CPath; }


namespace io
{
	enum { FileBlockSize = 16 * KiloByte };		// read in 16KB data blocks at a time; originally 4096-byte (4K) data blocks


	void __declspec(noreturn) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException );
	void __declspec(noreturn) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException );


	template< typename CharT >
	size_t GetStreamSize( std::basic_istream<CharT>& is )
	{
		ASSERT( is.good() );
		typename std::basic_istream<CharT>::pos_type currPos = is.tellg();

		is.seekg( 0, std::ios_base::end );
		size_t streamCount = static_cast<size_t>( is.tellg() );

		is.seekg( currPos );			// restore original reading position
		return streamCount;
	}
}


#endif // Io_fwd_h
