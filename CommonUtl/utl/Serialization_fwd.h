#ifndef Serialization_fwd_h
#define Serialization_fwd_h
#pragma once


class CArchive;


namespace portable
{
	// serializable numeric types that are portable across 32 bit/64 bit (Win32/x64)

	typedef unsigned int size_t;
    typedef unsigned __int64 DWORD_PTR;
    typedef unsigned __int64 UINT_PTR;


	/*	Note: with archive insertors/extractors, DON'T use:
			size_t, DWORD_PTR, UINT_PTR

	Storage:
		Type				32 bit		64 bit
		----				------		------
		sizeof( bool )		1 bytes		1 bytes
		sizeof( char )		1 bytes		1 bytes
		sizeof( short )		2 bytes		2 bytes
		sizeof( int )		4 bytes		4 bytes
		sizeof( long )		4 bytes		4 bytes
		sizeof( __int64 )	8 bytes		8 bytes
		sizeof( long long )	8 bytes		8 bytes
		sizeof( size_t )	4 bytes		8 bytes		AVOID!
		sizeof( DWORD )		4 bytes		4 bytes
		sizeof( DWORD_PTR )	4 bytes		8 bytes		AVOID!
		sizeof( float )		4 bytes		4 bytes
		sizeof( double )	8 bytes		8 bytes
	*/
}


namespace serial
{
	interface ISerializable		// equivalent with CObject as base class, but implemented by non MFC dynamic objects
	{
		virtual void Serialize( CArchive& archive ) = 0;
	};
}


namespace serial
{
	interface IStreamable
	{
		virtual void Save( CArchive& archive ) throws_( CException* ) = 0;
		virtual void Load( CArchive& archive ) throws_( CException* ) = 0;
	};
}


#endif // Serialization_fwd_h
