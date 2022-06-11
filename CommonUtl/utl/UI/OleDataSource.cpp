
#include "stdafx.h"
#include "OleDataSource.h"
#include "OleDropSource.h"
#include "OleDragDrop_fwd.h"
#include "OleUtils.h"
#include "ShellUtilities.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ole
{
	/*
		Rendering is not recommended for drag-drop;  may be used when providing file data that did not change.

		http://www.codeproject.com/Articles/840/How-to-Implement-Drag-and-Drop-Between-Your-Progra
		Shell Clipboard Formats: http://msdn.microsoft.com/en-us/library/windows/desktop/bb776902%28v=vs.85%29.aspx
		Shell Data Object: http://msdn.microsoft.com/en-us/library/windows/desktop/bb776903%28v=vs.85%29.aspx
	*/

	const CLIPFORMAT CDataSource::s_cfDropDescription = ole_utl::RegisterFormat( CFSTR_DROPDESCRIPTION );
	CRect CDataSource::m_nullRect( 0, 0, 0, 0 );

	CDataSource::CDataSource( ole::IRenderDataWnd* pRenderDataWnd /*= NULL*/, CWnd* pSrcWnd /*= NULL*/ )
		: COleDataSource()
		, m_pRenderDataWnd( pRenderDataWnd )
		, m_pSrcWnd( pSrcWnd )
		, m_dragImager( this )
		, m_dragResult( 0 )
		, m_hasDropTipText( false )
	{
		if ( NULL == m_pSrcWnd && m_pRenderDataWnd != NULL )
			m_pSrcWnd = m_pRenderDataWnd->GetSrcWnd();

		// enable drop-tips by default: this will create a global DWORD data object of format "DragSourceHelperFlags"
		if ( m_dragImager.GetHelper2() != NULL )
			m_dragImager.GetHelper2()->SetFlags( DSH_ALLOWDROPDESCRIPTIONTEXT );
	}

	CDataSource::~CDataSource()
	{
	}

	DROPEFFECT CDataSource::DragAndDrop( DROPEFFECT dropEffect, const RECT* pStartDragRect /*= NULL*/ )
	{
		// for drop-tips: we must use a ole::CDropSource object that handles drop descriptions within GiveFeedback()

		bool useDropTips = ::IsAppThemed() && m_dragImager.GetHelper2() != NULL;		// use old cursors when visual styles are disabled

		if ( useDropTips && m_hasDropTipText )
		{
			if ( s_cfDropDescription != 0 )
				if ( HGLOBAL hGlobal = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( DROPDESCRIPTION ) ) )
				{
					// create a DropTip data object with image type DROPIMAGE_INVALID
					DROPDESCRIPTION* pDropTip = static_cast<DROPDESCRIPTION*>( ::GlobalLock( hGlobal ) );
					pDropTip->type = DROPIMAGE_INVALID;
					::GlobalUnlock( hGlobal );
					CacheGlobalData( s_cfDropDescription, hGlobal );
				}
		}

		ole::CDropSource dropSource;					// for drop-tips support
		if ( useDropTips )
		{
			dropSource.SetDataSource( this );
			if ( m_hasDropTipText )
				dropSource.SetDropTip( &m_dropTip );
		}

		dropEffect = DoDragDrop( dropEffect, pStartDragRect, static_cast<COleDropSource*>( &dropSource ) );	// perform the drag operation (modal)
		m_dragResult = dropSource.GetDragResult();
		if ( dropEffect & ~DROPEFFECT_SCROLL )
			m_dragResult |= ole::DragDropped;			// indicate that data has been successfully dropped
		return dropEffect;
	}

	bool CDataSource::DisableDropTipText( void )
	{
		return m_dragImager.GetHelper2() != NULL && HR_OK( m_dragImager.GetHelper2()->SetFlags( 0 ) );		// clear DSH_ALLOWDROPDESCRIPTIONTEXT flag
	}

	bool CDataSource::SetDropTipText( DROPIMAGETYPE dropImageType, const wchar_t* pMessage, const wchar_t* pInsertFmt, const wchar_t* pInsertText /*= NULL*/ )
	{
		// PHC: not sure this has any effect; most likely the CDropTarget::SetDropTipText() is the one that works.
		// If any text args is NULL, the Explorer default text is used.
		// Because the %1 placeholder is used to insert the szInsert text, a single percent character inside the strings must be esacped by another one.

		bool changed = false;
		if ( m_dragImager.GetHelper2() != NULL )
		{
			changed = m_dropTip.StoreTypeField( dropImageType, pMessage, pInsertFmt );
			if ( changed && pInsertText != NULL )
				m_dropTip.SetInsert( pInsertText );

			m_hasDropTipText |= changed;
		}
		return changed;
	}

	void CDataSource::CacheShellFilePaths( const std::vector< fs::CPath >& filePaths )
	{
		shell::xfer::CacheDragDropSrcFiles( *this, filePaths );
	}

	void CDataSource::CacheShellFilePath( const fs::CPath& filePath )
	{
		std::vector< fs::CPath > filePaths( 1, filePath );
		CacheShellFilePaths( filePaths );
	}

	BOOL CDataSource::OnRenderData( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium )
	{
		if ( !COleDataSource::OnRenderData( pFormatEtc, pStgMedium ) )
			if ( m_pRenderDataWnd != NULL )
				return m_pRenderDataWnd->HandleRenderData( pFormatEtc, pStgMedium );		// let the source window render the data (delayed)

		return false;
	}


	// drag source helper - see http://www.codeproject.com/Articles/3530/DragSourceHelper-MFC

	BEGIN_INTERFACE_MAP( CDataSource, COleDataSource )
		INTERFACE_PART( CDataSource, IID_IDataObject, DataObjectEx )
	END_INTERFACE_MAP()

	STDMETHODIMP_( ULONG ) CDataSource::XDataObjectEx::AddRef( void )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->ExternalAddRef();
	}

	STDMETHODIMP_( ULONG ) CDataSource::XDataObjectEx::Release( void )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->ExternalRelease();
	}

	STDMETHODIMP CDataSource::XDataObjectEx::QueryInterface( REFIID iid, void** ppvObj )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return (HRESULT)pThis->ExternalQueryInterface( &iid, ppvObj );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::GetData( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.GetData( pFormatEtc, pStgMedium );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::GetDataHere( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.GetDataHere( pFormatEtc, pStgMedium );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::QueryGetData( FORMATETC* pFormatEtc )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.QueryGetData( pFormatEtc );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::GetCanonicalFormatEtc( FORMATETC* pFormatEtcIn, FORMATETC* pFormatEtcOut )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.GetCanonicalFormatEtc( pFormatEtcIn, pFormatEtcOut );
	}

	/*
		IDragSourceHelper calls this with custom clipboard formats.
		This call fails because custom formats are not supported by the COleDataSource::XDataObject::SetData() function.
		So cache the data here to make them available for displaying the drag image by the IDropTargetHelper.
		Checking for the format numbers here will not work because these numbers may differ.
		We may check for the format names here, but this requires getting and comparing the names which may also change with Windows versions.

		Names are: DragImageBits, DragContext, IsShowingLayered, DragWindow, IsShowingText
	*/
	STDMETHODIMP CDataSource::XDataObjectEx::SetData( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium, BOOL bRelease )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		HRESULT hr = pThis->m_xDataObject.SetData( pFormatEtc, pStgMedium, bRelease );
		if ( DATA_E_FORMATETC == hr &&					// Error: Invalid FORMATETC structure
			 HasFlag( pFormatEtc->tymed, TYMED_HGLOBAL | TYMED_ISTREAM ) &&
			 pFormatEtc->cfFormat >= 0xC000 )
		{
			pThis->CacheData( pFormatEtc->cfFormat, pStgMedium, pFormatEtc );
			hr = S_OK;
		}
		return hr;
	}

	STDMETHODIMP CDataSource::XDataObjectEx::EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.EnumFormatEtc( dwDirection, ppEnumFormatEtc );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::DAdvise( FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.DAdvise( pFormatEtc, advf, pAdvSink, pdwConnection );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::DUnadvise( DWORD dwConnection )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.DUnadvise( dwConnection );
	}

	STDMETHODIMP CDataSource::XDataObjectEx::EnumDAdvise( IEnumSTATDATA** ppEnumAdvise )
	{
		METHOD_PROLOGUE( CDataSource, DataObjectEx )
		return pThis->m_xDataObject.EnumDAdvise( ppEnumAdvise );
	}

} //namespace ole
