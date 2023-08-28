
#include "pch.h"
#include "MfcUtilities.h"
#include "Path.h"
#include "Serialization.h"
#include "WndUtils.h"
#include "ListLikeCtrlBase.h"	// for is_a<CListLikeCtrlBase>()
#include "ThemeStatic.h"		// for is_a<CStatusStatic>()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CMemFile_ : public CMemFile
	{
		// public access
		using CMemFile::m_lpBuffer;
	};
}


namespace mfc
{
	const BYTE* GetFileBuffer( const CMemFile* pMemFile, OUT OPTIONAL size_t* pBufferSize /*= nullptr*/ )
	{
		ASSERT_PTR( pMemFile );

		utl::AssignPtr( pBufferSize, static_cast<size_t>( pMemFile->GetLength() ) );
		return mfc::nosy_cast<nosy::CMemFile_>( pMemFile )->m_lpBuffer;
	}
}


namespace utl
{
	CGlobalMemFile::CGlobalMemFile( HGLOBAL hSrcBuffer ) throws_( CException )
		: CMemFile( 0 )				// no growth when reading
		, m_hLockedSrcBuffer( nullptr )
	{
		if ( hSrcBuffer != nullptr )
			if ( size_t bufferSize = ::GlobalSize( hSrcBuffer ) )
				if ( BYTE* pSrcBuffer = (BYTE*)::GlobalLock( hSrcBuffer ) )
				{
					m_hLockedSrcBuffer = hSrcBuffer;
					Attach( pSrcBuffer, static_cast<UINT>( bufferSize ) );
				}

		if ( nullptr == m_hLockedSrcBuffer )
			AfxThrowOleException( E_POINTER );			// source bufer not accessible
	}

	CGlobalMemFile::CGlobalMemFile( size_t growBytes /*= KiloByte*/ )
		: CMemFile( static_cast<UINT>( growBytes ) )
		, m_hLockedSrcBuffer( nullptr )
	{
	}

	CGlobalMemFile::~CGlobalMemFile()
	{
		if ( m_hLockedSrcBuffer != nullptr )
			::GlobalUnlock( m_hLockedSrcBuffer );
	}

	HGLOBAL CGlobalMemFile::MakeGlobalData( UINT flags /*= GMEM_MOVEABLE*/ )
	{
		HGLOBAL hGlobal = nullptr;
		if ( size_t bufferSize = static_cast<size_t>( GetLength() ) )
		{
			ASSERT_PTR( m_lpBuffer );
			hGlobal = ::GlobalAlloc( flags, bufferSize );
			if ( hGlobal != nullptr )
				if ( void* pDestBuffer = ::GlobalLock( hGlobal ) )
				{
					::CopyMemory( pDestBuffer, m_lpBuffer, bufferSize );
					::GlobalUnlock( hGlobal );
				}
		}
		return hGlobal;
	}
}


namespace serial
{
	fs::CPath GetDocumentPath( const CArchive& archive )
	{
		return path::ExtractPhysical( archive.m_strFileName.GetString() );
	}


	// CScopedLoadingArchive implementation

	int CScopedLoadingArchive::s_latestModelSchema = UnitializedVersion;
	const CArchive* CScopedLoadingArchive::s_pLoadingArchive = nullptr;
	int CScopedLoadingArchive::s_docLoadingModelSchema = UnitializedVersion;

	CScopedLoadingArchive::CScopedLoadingArchive( const CArchive& rArchiveLoading, int docLoadingModelSchema )
	{
		REQUIRE( s_latestModelSchema != UnitializedVersion );		// (!) must have beeen initialized at application startup
		ASSERT_NULL( s_pLoadingArchive );							// nesting of loading archives not allowed
		REQUIRE( rArchiveLoading.IsLoading() );

		s_pLoadingArchive = &rArchiveLoading;
		s_docLoadingModelSchema = docLoadingModelSchema;
	}

	CScopedLoadingArchive::~CScopedLoadingArchive()
	{
		s_pLoadingArchive = nullptr;
		s_docLoadingModelSchema = -1;
	}

	bool CScopedLoadingArchive::IsValidLoadingArchive( const CArchive& rArchive )
	{
		if ( !rArchive.IsLoading() )
			return false;

		if ( IsFileBasedArchive( rArchive ) )
			return s_pLoadingArchive != nullptr;			// must have been created in the scope of loading a FILE with backwards compatibility

		return true;
	}


	// CStreamingGuard implementation

	std::vector<CStreamingGuard*> CStreamingGuard::s_instances;

	CStreamingGuard::CStreamingGuard( const CArchive& rArchive )
		: m_rArchive( rArchive )
		, m_streamingFlags( 0 )
	{
		s_instances.push_back( this );
	}

	CStreamingGuard::~CStreamingGuard()
	{
		ASSERT( s_instances.back() == this );
		s_instances.pop_back();
	}
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()

	CAdapterDocument::CAdapterDocument( serial::IStreamable* pStreamable, const fs::CPath& docPath )
		: CDocument()
		, m_pStreamable( pStreamable )
		, m_pObject( nullptr )
	{
		ASSERT_PTR( m_pStreamable );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	CAdapterDocument::CAdapterDocument( CObject* pObject, const fs::CPath& docPath )
		: CDocument()
		, m_pStreamable( nullptr )
		, m_pObject( pObject )
	{
		ASSERT_PTR( m_pObject );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	void CAdapterDocument::Serialize( CArchive& archive )
	{
		if ( m_pStreamable != nullptr )
		{
			if ( archive.IsLoading() )
				m_pStreamable->Load( archive );
			else
				m_pStreamable->Save( archive );
		}
		else if ( m_pObject != nullptr )
			m_pObject->Serialize( archive );
	}

	bool CAdapterDocument::Load( void ) throws_()
	{
		if ( fs::FileExist( GetPathName() ) )
			if ( OnOpenDocument( GetPathName() ) )
				return true;
			else
				TRACE( _T(" * Error loading document adapter file: %s\n"), GetPathName().GetString() );

		return false;
	}

	bool CAdapterDocument::Save( void ) throws_()
	{
		if ( !OnSaveDocument( GetPathName() ) )
		{
			TRACE( _T(" * Error saving document adapter file: %s\n"), GetPathName().GetString() );
			return false;
		}

		return true;
	}

	void CAdapterDocument::ReportSaveLoadException( const TCHAR* pFilePath, CException* pExc, BOOL isSaving, UINT idDefaultPrompt )
	{
		if ( const CArchiveException* pArchiveExc = dynamic_cast<const CArchiveException*>( pExc ) )
			switch ( pArchiveExc->m_cause )
			{
				case CArchiveException::badSchema:
				case CArchiveException::badClass:
				case CArchiveException::badIndex:
				case CArchiveException::endOfFile:
					ui::MessageBox( str::Format( _T("The loading binary file format is incompatible with current document schema version!\n\n%s"), pFilePath ) );
					return;
			}

		__super::ReportSaveLoadException( pFilePath, pExc, isSaving, idDefaultPrompt );
	}
}


namespace ui
{
	// CTooltipTextMessage implementation

	const std::tstring CTooltipTextMessage::s_nilText = _T("<nil>");

	CTooltipTextMessage::CTooltipTextMessage( NMHDR* pNmHdr )
		: m_pTooltip( static_cast<CToolTipCtrl*>( CWnd::FromHandlePermanent( pNmHdr->hwndFrom ) ) )
		, m_pTttA( TTN_NEEDTEXTA == pNmHdr->code ? reinterpret_cast<TOOLTIPTEXTA*>( pNmHdr ) : nullptr )
		, m_pTttW( TTN_NEEDTEXTW == pNmHdr->code ? reinterpret_cast<TOOLTIPTEXTW*>( pNmHdr ) : nullptr )
		, m_cmdId( static_cast<UINT>( pNmHdr->idFrom ) )
		, m_hCtrl( nullptr )
		, m_pData( reinterpret_cast<void*>( m_pTttW != nullptr ? m_pTttW->lParam : m_pTttA->lParam ) )
	{
		ASSERT( m_pTttA != nullptr || m_pTttW != nullptr );

		if ( HasFlag( m_pTttW != nullptr ? m_pTttW->uFlags : m_pTttA->uFlags, TTF_IDISHWND ) )
		{
			m_hCtrl = (HWND)pNmHdr->idFrom;						// idFrom is actually the HWND of the tool
			m_cmdId = ::GetDlgCtrlID( m_hCtrl );
		}
	}

	CTooltipTextMessage::CTooltipTextMessage( TOOLTIPTEXT* pNmToolTipText )
		: m_pTooltip( static_cast<CToolTipCtrl*>( CWnd::FromHandlePermanent( pNmToolTipText->hdr.hwndFrom ) ) )
		, m_pTttA( nullptr )
		, m_pTttW( pNmToolTipText )
		, m_cmdId( static_cast<UINT>( pNmToolTipText->hdr.idFrom ) )
		, m_hCtrl( nullptr )
		, m_pData( reinterpret_cast<void*>( m_pTttW->lParam ) )
	{
		ASSERT_PTR( m_pTttW );

		if ( HasFlag( m_pTttW->uFlags, TTF_IDISHWND ) )
		{
			m_hCtrl = (HWND)pNmToolTipText->hdr.idFrom;			// idFrom is actually the HWND of the tool
			m_cmdId = ::GetDlgCtrlID( m_hCtrl );
		}
	}

	bool CTooltipTextMessage::IsValidNotification( void ) const
	{
		return m_cmdId != 0;
	}

	bool CTooltipTextMessage::AssignTooltipText( const std::tstring& text )
	{
		if ( text.empty() || s_nilText == text )
			return false;

		if ( m_pTttA != nullptr )
		{
			static std::string s_narrowText;			// static buffer (can be longer than 80 characters default limit)
			s_narrowText = str::AsNarrow( text );
			m_pTttA->lpszText = const_cast<char*>( s_narrowText.c_str() );
		}
		else if ( m_pTttW != nullptr )
		{
			static std::wstring s_wideText;				// static buffer (can be longer than 80 characters default limit)
			s_wideText = str::AsWide( text );
			m_pTttW->lpszText = const_cast<wchar_t*>( s_wideText.c_str() );
		}
		else
			ASSERT( false );

		if ( text.find( '\n' ) != std::tstring::npos )	// multi-line text?
			if ( m_pTooltip->GetSafeHwnd() != nullptr )
			{
				// Win32 requirement for multi-line tooltips: we must send TTM_SETMAXTIPWIDTH to the tooltip
				if ( -1 == m_pTooltip->GetMaxTipWidth() )	// not initialized?
					m_pTooltip->SetMaxTipWidth( ui::FindMonitorRect( m_pTooltip->GetSafeHwnd(), ui::Workspace ).Width() );		// the entire desktop width

				m_pTooltip->SetDelayTime( TTDT_AUTOPOP, 30 * 1000 );		// display for 1/2 minute (16-bit limit: it doesn't work beyond 32768 miliseconds)
			}

		// bring the tooltip window above other popup windows
		if ( m_pTooltip != nullptr )
			ui::BringWndToTop( *m_pTooltip );			// move window to the top/bottom of Z-order (above other popup windows)

		return true;			// valid tooltip text
	}

	bool CTooltipTextMessage::IgnoreResourceString( void ) const
	{
		return m_hCtrl != nullptr && IgnoreResourceString( m_hCtrl );
	}

	bool CTooltipTextMessage::IgnoreResourceString( HWND hCtrl )
	{
		if ( CWnd* pCtrl = CWnd::FromHandlePermanent( hCtrl ) )
			return is_a<CListLikeCtrlBase>( pCtrl ) || is_a<CStatusStatic>( pCtrl );	// ignore column layout descriptors (list controls, grids, etc)

		return false;
	}
}


namespace mfc
{
	CMDIChildWnd* GetFirstMdiChildFrame( const CMDIFrameWnd* pMdiFrameWnd /*= mfc::GetMainMdiFrameWnd()*/ )
	{
		ASSERT_PTR( pMdiFrameWnd->GetSafeHwnd() );
		ASSERT_PTR( pMdiFrameWnd->m_hWndMDIClient );

		if ( CWnd* pMdiChild = CWnd::FromHandle( pMdiFrameWnd->m_hWndMDIClient )->GetWindow( GW_CHILD ) )
		{
			pMdiChild = pMdiChild->GetWindow( GW_HWNDLAST );		// in display terms, last MDI means on top of the MDI children Z order
			ASSERT_PTR( pMdiChild );

			while ( !is_a<CMDIChildWnd>( pMdiChild ) )
				pMdiChild = pMdiChild->GetWindow( GW_HWNDPREV );	// assume MDI Client has other windows other than frames (which is very unusual)

			return checked_static_cast<CMDIChildWnd*>( pMdiChild );
		}

		return nullptr;
	}
}
