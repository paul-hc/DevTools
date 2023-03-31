#ifndef OleDataSource_h
#define OleDataSource_h
#pragma once

#include <afxole.h>
#include "utl/Path.h"
#include "ShellDragImager.h"
#include "ShellDropTip.h"


namespace ole
{
	interface IRenderDataWnd;


	// Clipboard and drag&drop copy with support for drag images, drag cursors, drop tip text (optional).
	// Takes advantage of DI_GETDRAGIMAGE registered shell message (that fills the SHDRAGIMAGE structure).
	//
	class CDataSource : public COleDataSource
	{
	public:
		CDataSource( ole::IRenderDataWnd* pRenderDataWnd = nullptr, CWnd* pSrcWnd = nullptr );
		virtual ~CDataSource();

		DROPEFFECT DragAndDrop( DROPEFFECT dropEffect, const RECT* pStartDragRect = nullptr );		// pass m_nullRect to disable the start drag delay

		int GetDragResult( void ) const { return m_dragResult; }

		// drop-tip text
		bool DisableDropTipText( void );
		bool SetDropTipText( DROPIMAGETYPE dropImageType, const wchar_t* pMessage, const wchar_t* pInsertFmt, const wchar_t* pInsertText = nullptr );

		// data caching overridables
		virtual void CacheShellFilePaths( const std::vector<fs::CPath>& filePaths );			// formats: shell::cfHDROP, shell::cfFileGroupDescriptor
		void CacheShellFilePath( const fs::CPath& filePath );

		// data rendering (delayed)
		CWnd* GetWnd( void ) const { return m_pSrcWnd; }
		shell::CDragImager& GetDragImager( void ) { return m_dragImager; }

		// base overrides
		virtual BOOL OnRenderData( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium );
	private:
		// hidden, use DragAndDrop() for all features
		using COleDataSource::DoDragDrop;
	private:
		ole::IRenderDataWnd* m_pRenderDataWnd;
		CWnd* m_pSrcWnd;

		shell::CDragImager m_dragImager;
		shell::CDropTip m_dropTip;

		bool m_hasDropTipText;						// set when drop description text string(s) has been specified
		int m_dragResult;							// DragAndDrop result flags
	public:
		static const CLIPFORMAT s_cfDropDescription;
		static CRect m_nullRect;
	public:
		BEGIN_INTERFACE_PART( DataObjectEx, IDataObject )
			INIT_INTERFACE_PART( CDataSource, DataObjectEx )
			STDMETHOD( GetData )( FORMATETC*, STGMEDIUM* );
			STDMETHOD( GetDataHere )( FORMATETC*, STGMEDIUM* );
			STDMETHOD( QueryGetData )( FORMATETC* );
			STDMETHOD( GetCanonicalFormatEtc )( FORMATETC*, FORMATETC* );
			STDMETHOD( SetData )( FORMATETC*, STGMEDIUM*, BOOL );
			STDMETHOD( EnumFormatEtc )( DWORD, IEnumFORMATETC** );
			STDMETHOD( DAdvise )( FORMATETC*, DWORD, IAdviseSink*, DWORD* );
			STDMETHOD( DUnadvise )( DWORD );
			STDMETHOD( EnumDAdvise )( IEnumSTATDATA** );
		END_INTERFACE_PART( DataObjectEx )

		DECLARE_INTERFACE_MAP()
	};
}


#endif // OleDataSource_h
