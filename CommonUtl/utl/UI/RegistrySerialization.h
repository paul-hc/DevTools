#ifndef RegistrySerialization_h
#define RegistrySerialization_h
#pragma once

#include "utl/Serialization_fwd.h"


namespace reg
{
	// registry serialization of "value objects" (not-dynamically created creatable) that are:
	//	(1) derived from CObject
	//	(2) implement serial::ISerializable interface

	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT CObject* pDestObject );
	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const CObject* pSrcObject );

	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT serial::ISerializable* pDestObject );
	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const serial::ISerializable* pSrcObject );

	bool DeleteProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry );
}


#endif // RegistrySerialization_h
