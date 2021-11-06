#ifndef OleUtils_h
#define OleUtils_h
#pragma once

#include <afxole.h>
#include "Serialization_fwd.h"


class CClipboard;


namespace ole_utl
{
	inline CLIPFORMAT RegisterFormat( const TCHAR* pFormat ) { return static_cast<CLIPFORMAT>( ::RegisterClipboardFormat( pFormat ) ); }
	bool IsFormatRegistered( CLIPFORMAT cfFormat, const TCHAR* pFormat );


	// drag-drop and clipboard (IDataObject*)

	void CacheGlobalData( COleDataSource* pDataSource, CLIPFORMAT clipFormat, HGLOBAL hGlobal );
	bool ExtractData( FORMATETC* pOutFormatEtc, STGMEDIUM* pOutStgMedium, IDataObject* pDataObject, const TCHAR* pFormat );

	DWORD GetValueDWord( IDataObject* pDataObject, const TCHAR* pFormat );
	bool SetValueDWord( IDataObject* pDataObject, DWORD value, const TCHAR* pFormat, bool force = true );

	void CacheTextData( COleDataSource* pDataSource, const std::tstring& text );


	// dragging utils

	DROPIMAGETYPE DropImageTypeFromEffect( DROPEFFECT effect );
}


namespace ole
{
	class CDataSource;


	// allows the owner of a dragging control to create specific data source objects for clipboard and drag-drop
	//
	interface IDataSourceFactory
	{
		virtual ole::CDataSource* NewDataSource( void ) = 0;
	};

	IDataSourceFactory* GetStdDataSourceFactory( void );


	// base for streamable classes that encapsulate drag-drop or cliboard data
	//
	abstract class CTransferBlob : protected serial::IStreamable
	{
	protected:
		CTransferBlob( CLIPFORMAT clipFormat ) : m_clipFormat( clipFormat ) {}
	public:
		CLIPFORMAT GetFormat( void ) const { return m_clipFormat; }

		HGLOBAL MakeGlobalData( void ) const throws_();
		bool ReadFromGlobalData( HGLOBAL hGlobal ) throws_();

		// drag & drop
		bool CacheTo( COleDataSource* pDataSource ) const throws_();
		bool ExtractFrom( COleDataObject* pDataObject ) throws_();
		bool CanExtractFrom( COleDataObject* pDataObject ) const { return safe_ptr( pDataObject )->IsDataAvailable( m_clipFormat ) != FALSE; }

		// clipboard
		bool CanPaste( void ) const;
		bool Paste( const CClipboard* pClipboard );
		bool Copy( CClipboard* pClipboard );
	private:
		CLIPFORMAT m_clipFormat;
	};
}


#endif // OleUtils_h
