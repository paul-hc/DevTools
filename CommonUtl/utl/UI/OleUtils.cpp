
#include "pch.h"
#include "OleUtils.h"
#include "OleDataSource.h"
#include "Clipboard.h"
#include "MfcUtilities.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ole_utl
{
	bool IsFormatRegistered( CLIPFORMAT cfFormat, const TCHAR* pFormat )
	{
		bool bRet = false;
		if ( cfFormat >= 0xC000 )
		{
			TCHAR name[ 128 ];
			if ( ::GetClipboardFormatName( cfFormat, name, sizeof( name ) / sizeof( name[ 0 ] ) ) )
				bRet = ( 0 == _tcsicmp( pFormat, name ) );
		}
		return bRet;
	}

	void CacheGlobalData( COleDataSource* pDataSource, CLIPFORMAT clipFormat, HGLOBAL hGlobal )
	{
		ASSERT( pDataSource );
		if ( hGlobal != nullptr )
			pDataSource->CacheGlobalData( clipFormat, hGlobal );
	}

	bool ExtractData( FORMATETC* pOutFormatEtc, STGMEDIUM* pOutStgMedium, IDataObject* pDataObject, const TCHAR* pFormat )
	{
		// to access global data objects
		ASSERT( pOutFormatEtc );
		ASSERT( pOutStgMedium );
		ASSERT( pDataObject );
		ASSERT( !str::IsEmpty( pFormat ) );

		bool succeeded = false;
		pOutFormatEtc->cfFormat = ole_utl::RegisterFormat( pFormat );
		pOutFormatEtc->ptd = nullptr;
		pOutFormatEtc->dwAspect = DVASPECT_CONTENT;
		pOutFormatEtc->lindex = -1;
		pOutFormatEtc->tymed = TYMED_HGLOBAL;

		if ( HR_OK( pDataObject->QueryGetData( pOutFormatEtc ) ) )
			if ( HR_OK( pDataObject->GetData( pOutFormatEtc, pOutStgMedium ) ) )
			{
				succeeded = TYMED_HGLOBAL == pOutStgMedium->tymed;
				if ( !succeeded )
					::ReleaseStgMedium( pOutStgMedium );
			}

		return succeeded;
	}

	DWORD GetValueDWord( IDataObject* pDataObject, const TCHAR* pFormat )
	{
		DWORD value = 0;
		FORMATETC formatEtc;
		STGMEDIUM stgMedium;
		if ( ExtractData( &formatEtc, &stgMedium, pDataObject, pFormat ) )
		{
			ASSERT( ::GlobalSize( stgMedium.hGlobal ) >= sizeof( DWORD ) );
			value = *static_cast<DWORD*>( ::GlobalLock( stgMedium.hGlobal ) );
			::GlobalUnlock( stgMedium.hGlobal );
			::ReleaseStgMedium( &stgMedium );
		}
		return value;
	}

	bool SetValueDWord( IDataObject* pDataObject, DWORD value, const TCHAR* pFormat, bool force /*= true*/ )
	{
		// create global DWORD data object or set value for existing object
		bool succeeded = false;
		FORMATETC formatEtc;
		STGMEDIUM stgMedium;

		// check if object exists already
		if ( ExtractData( &formatEtc, &stgMedium, pDataObject, pFormat ) )
		{
			DWORD* pValue = static_cast<DWORD*>( ::GlobalLock( stgMedium.hGlobal ) );
			succeeded = ( *pValue != value );
			*pValue = value;
			::GlobalUnlock( stgMedium.hGlobal );
			if ( succeeded )
				succeeded = HR_OK( pDataObject->SetData( &formatEtc, &stgMedium, TRUE ) );
			if ( !succeeded )									// not changed or setting failed
				::ReleaseStgMedium( &stgMedium );
		}
		else if ( value || force )
		{
			stgMedium.hGlobal = ::GlobalAlloc( GMEM_MOVEABLE, sizeof( DWORD ) );
			if ( stgMedium.hGlobal )
			{
				DWORD* pValue = static_cast<DWORD*>( ::GlobalLock( stgMedium.hGlobal ) );
				*pValue = value;
				::GlobalUnlock( stgMedium.hGlobal );
				stgMedium.tymed = TYMED_HGLOBAL;
				stgMedium.pUnkForRelease = nullptr;
				succeeded = HR_OK( pDataObject->SetData( &formatEtc, &stgMedium, TRUE ) );
				if ( !succeeded )
					::GlobalFree( stgMedium.hGlobal );
			}
		}
		return succeeded;
	}

	void CacheTextData( COleDataSource* pDataSource, const std::tstring& text )
	{
		CacheGlobalData( pDataSource, CF_TEXT, utl::CopyToGlobalData( CStringA( text.c_str() ).GetString(), text.length() + 1, GMEM_MOVEABLE | GMEM_SHARE ) );
		CacheGlobalData( pDataSource, CF_UNICODETEXT, utl::CopyToGlobalData( CStringW( text.c_str() ).GetString(), text.length() + 1, GMEM_MOVEABLE | GMEM_SHARE ) );
	}


	// dragging utils

	DROPIMAGETYPE DropImageTypeFromEffect( DROPEFFECT effect )
	{
		effect &= ~DROPEFFECT_SCROLL;

		// drop handlers may return effects with multiple bits set: must prioritize
		if ( DROPEFFECT_NONE == effect )
			return DROPIMAGE_NONE;
		else if ( HasFlag( effect, DROPEFFECT_MOVE ) )
			return DROPIMAGE_MOVE;
		else if ( HasFlag( effect, DROPEFFECT_COPY ) )
			return DROPIMAGE_COPY;
		else if ( HasFlag( effect, DROPEFFECT_LINK ) )
			return DROPIMAGE_LINK;

		return DROPIMAGE_INVALID;
	}

} //namespace ole_utl


namespace ole
{
	namespace impl
	{
		class CStdDataSourceFactory : public IDataSourceFactory
		{
		public:
			CStdDataSourceFactory( void ) {}

			virtual ole::CDataSource* NewDataSource( void );
		};

		ole::CDataSource* CStdDataSourceFactory::NewDataSource( void )
		{
			return new ole::CDataSource;
		}
	}


	IDataSourceFactory* GetStdDataSourceFactory( void )
	{
		static impl::CStdDataSourceFactory stdFactory;
		return &stdFactory;
	}


	// CTransferBlob implementation

	HGLOBAL CTransferBlob::MakeGlobalData( void ) const throws_()
	{
		try
		{
			utl::CGlobalMemFile destMemFile;
			CArchive archive( &destMemFile, CArchive::store );
			archive.m_bForceFlat = FALSE;

			const_cast<CTransferBlob*>( this )->Save( archive );
			archive.Close();		// flushes the data so that we can copy the buffer

			return destMemFile.MakeGlobalData( GMEM_MOVEABLE );			// allocate and copy buffer contents
		}
		catch ( CException* pExc )
		{	pExc;
			TRACE( _T("* CTransferBlob::CacheTo() exception: %s"), mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
		}
		return nullptr;
	}

	bool CTransferBlob::ReadFromGlobalData( HGLOBAL hGlobal ) throws_()
	{
		try
		{
			utl::CGlobalMemFile srcMemFile( hGlobal );
			CArchive archive( &srcMemFile, CArchive::load );
			archive.m_bForceFlat = FALSE;

			Load( archive );
			return true;
		}
		catch ( CException* pExc )
		{	pExc;
			TRACE( _T("* CTransferBlob::ExtractFrom() exception: %s"), mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
			return false;
		}
	}

	bool CTransferBlob::CacheTo( COleDataSource* pDataSource ) const throws_()
	{
		ASSERT_PTR( pDataSource );

		if ( HGLOBAL hGlobal = MakeGlobalData() )
		{
			pDataSource->CacheGlobalData( m_clipFormat, hGlobal );
			return true;
		}
		return false;
	}

	bool CTransferBlob::ExtractFrom( COleDataObject* pDataObject )
	{
		if ( !CanExtractFrom( pDataObject ) )
			return false;

		STGMEDIUM stgMedium = { 0 };	// defend against buggy data object
		if ( !pDataObject->GetData( m_clipFormat, &stgMedium ) )		// get the transfer data
			return false;

		bool succeeded = ReadFromGlobalData( stgMedium.hGlobal );

		::ReleaseStgMedium( &stgMedium );
		return succeeded;
	}

	bool CTransferBlob::CanPaste( void ) const
	{
		return CClipboard::IsFormatAvailable( m_clipFormat );
	}

	bool CTransferBlob::Paste( const CClipboard* pClipboard )
	{
		HGLOBAL hGlobal = safe_ptr( pClipboard )->GetData( m_clipFormat );
		return hGlobal != nullptr && ReadFromGlobalData( hGlobal );
	}

	bool CTransferBlob::Copy( CClipboard* pClipboard )
	{
		HGLOBAL hGlobal = MakeGlobalData();
		return hGlobal != nullptr && safe_ptr( pClipboard )->SetData( m_clipFormat, hGlobal );
	}

} //namespace ole
