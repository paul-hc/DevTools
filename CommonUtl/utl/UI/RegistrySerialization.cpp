
#include "pch.h"
#include "RegistrySerialization.h"
#include "MfcUtilities.h"
#include "utl/AppTools.h"
#include "utl/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace serial
{
	template< typename SerializableT >
	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT SerializableT* pOutSerializable );

	template< typename SerializableT >
	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const SerializableT* pSerializable );
}


namespace reg
{
	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT CObject* pDestObject )
	{
		return serial::LoadProfileBinaryState( pRegSection, pEntry, pDestObject );
	}

	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const CObject* pSrcObject )
	{
		return serial::SaveProfileBinaryState( pRegSection, pEntry, pSrcObject );
	}


	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT serial::ISerializable* pDestObject )
	{
		return serial::LoadProfileBinaryState( pRegSection, pEntry, pDestObject );
	}

	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const serial::ISerializable* pSrcObject )
	{
		return serial::SaveProfileBinaryState( pRegSection, pEntry, pSrcObject );
	}


	bool DeleteProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry )
	{
		reg::CKey key( AfxGetApp()->GetSectionKey( pRegSection ) );

		return key.IsOpen() && key.DeleteValue( pEntry );
	}
}


namespace serial
{
	template< typename SerializableT >
	bool LoadProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, OUT SerializableT* pOutSerializable )
	{
		BYTE* pDataBuffer = nullptr;
		UINT bufferSize;

		if ( !AfxGetApp()->GetProfileBinary( pRegSection, pEntry, &pDataBuffer, &bufferSize ) )
			return false;

		bool succeeded = false;

		try
		{
			CMemFile file( pDataBuffer, bufferSize );
			CArchive archive( &file, CArchive::load );

			pOutSerializable->Serialize( archive );
			succeeded = true;
		}
		catch ( CMemoryException* pExc )
		{
			app::TraceException( pExc );
			pExc->Delete();
		}
		catch ( CArchiveException* pExc )
		{
			app::TraceException( pExc );
			pExc->Delete();
		}

		if ( pDataBuffer != nullptr )
			delete[] pDataBuffer;

		return succeeded;
	}

	template< typename SerializableT >
	bool SaveProfileBinaryState( const TCHAR* pRegSection, const TCHAR* pEntry, const SerializableT* pSerializable )
	{
		bool succeeded = false;

		try
		{
			CMemFile file;

			{
				CArchive memArchive( &file, CArchive::store );

				const_cast<SerializableT*>( pSerializable )->Serialize( memArchive );
				memArchive.Flush();
			}

			size_t bufferSize;
			if ( const BYTE* pData = mfc::GetFileBuffer( &file, &bufferSize ) )
				succeeded = AfxGetApp()->WriteProfileBinary( pRegSection, pEntry, const_cast<BYTE*>( pData ), static_cast<UINT>( bufferSize ) ) != FALSE;

			file.Close();		// free buffer memory
		}
		catch ( CMemoryException* pExc )
		{
			app::TraceException( pExc );
			pExc->Delete();
		}

		return succeeded;
	}
}
