#ifndef MfcUtilities_h
#define MfcUtilities_h
#pragma once

#include "utl/MultiThreading.h"
#include "utl/Timer.h"


namespace com
{
	// wrapper for a PROPVARIANT value
	//
	class CPropVariant
	{
	public:
		CPropVariant( void ) { ::PropVariantInit( &m_value ); }
		~CPropVariant() { Clear(); }

		const PROPVARIANT& Get( void ) const { return m_value; }
		void Clear( void ) { ::PropVariantClear( &m_value ); }

		PROPVARIANT* operator&( void ) { Clear(); return &m_value; }		// clear before passing variant to get new value

		template< typename ValueType >
		bool GetByteAs( ValueType* pValue )
		{
			if ( m_value.vt != VT_UI1 )
				return false;
			*pValue = static_cast<ValueType>( m_value.bVal );
			return true;
		}

		template< typename ValueType >
		bool GetWordAs( ValueType* pValue )
		{
			if ( m_value.vt != VT_UI2 )
				return false;
			*pValue = static_cast<ValueType>( m_value.uiVal );
			return true;
		}

		bool GetBool( bool* pValue )
		{
			if ( m_value.vt != VT_BOOL )
				return false;
			*pValue = m_value.boolVal != FALSE;
			return true;
		}
	private:
		PROPVARIANT m_value;
	};
}


namespace mfc
{
	const BYTE* GetFileBuffer( const CMemFile* pMemFile, OUT OPTIONAL size_t* pBufferSize = nullptr );
}


namespace utl
{
	template< typename Type >
	HGLOBAL CopyToGlobalData( const Type array[], size_t count, UINT flags = GMEM_MOVEABLE )
	{
		if ( HGLOBAL hGlobal = ::GlobalAlloc( flags, count * sizeof( Type ) ) )
		{
			if ( void* pDestData = ::GlobalLock( hGlobal ) )
			{
				::CopyMemory( pDestData, array, count * sizeof( Type ) );
				::GlobalUnlock( hGlobal );
				return hGlobal;
			}
			::GlobalFree( hGlobal );		// free the memory on error
		}
		return nullptr;
	}


	// operates on global memory; used for clipboard and drag-drop transfers
	//
	class CGlobalMemFile : public CMemFile
	{
	public:
		CGlobalMemFile( HGLOBAL hSrcBuffer ) throws_( CException );		// reading
		CGlobalMemFile( size_t growBytes = KiloByte );					// writing
		virtual ~CGlobalMemFile();

		HGLOBAL MakeGlobalData( UINT flags = GMEM_MOVEABLE );			// end of writing: allocate and copy buffer contents
	private:
		HGLOBAL m_hLockedSrcBuffer;										// locked buffer, used only when reading
	};
}


namespace fs { class CPath; }


namespace serial
{
	interface IStreamable;


	inline bool IsFileBasedArchive( const CArchive& rArchive ) { return !rArchive.m_strFileName.IsEmpty(); }
	fs::CPath GetDocumentPath( const CArchive& archive );


	// Must have been created in the scope of loading a FILE with backwards compatibility.
	// Not necessary to create when loading from a CMemFile archive - it will assume s_latestModelSchema.
	//
	class CScopedLoadingArchive
	{
	public:
		CScopedLoadingArchive( const CArchive& rLoadingArchive, int docLoadingModelSchema );
		~CScopedLoadingArchive();

		static void SetLatestModelSchema( int latestModelSchema ) { s_latestModelSchema = latestModelSchema; }

		static bool IsValidLoadingArchive( const CArchive& rArchive );

		template< typename EnumType >
		static EnumType GetModelSchema( const CArchive& rArchive )
		{
			ASSERT( s_latestModelSchema != UnitializedVersion );		// (!) must have beeen initialized at application startup
			ASSERT( IsValidLoadingArchive( rArchive ) );				// if a file archive, ensure CScopedLoadingArchive is created in the scope of loading

			if ( &rArchive == s_pLoadingArchive )
				return static_cast<EnumType>( s_docLoadingModelSchema );

			return static_cast<EnumType>( s_latestModelSchema );
		}
	private:
		enum { UnitializedVersion = -1 };

		static int s_latestModelSchema;

		// file loading only
		static const CArchive* s_pLoadingArchive;
		static int s_docLoadingModelSchema;
	};


	class CStreamingGuard : private utl::noncopyable
	{
	public:
		CStreamingGuard( const CArchive& archive );
		~CStreamingGuard();

		double GetElapsedSeconds( void ) const { return m_timer.ElapsedSeconds(); }
		bool IsTimeout( double timeout ) const { return GetElapsedSeconds() > timeout; }
		const CArchive& GetArchive( void ) const { return m_rArchive; }

		CTimer& GetTimer( void ) { return m_timer; }

		bool HasStreamingFlag( int flag ) const { return HasFlag( m_streamingFlags, flag ); }
		void SetStreamingFlag( int flag, bool on = true ) { SetFlag( m_streamingFlags, flag, on ); }

		static CStreamingGuard* GetTop( void ) { return !s_instances.empty() ? s_instances.back() : nullptr; }
	private:
		const CArchive& m_rArchive;
		CTimer m_timer;
		int m_streamingFlags;		// client code maintains the actual flags

		static std::vector<CStreamingGuard*> s_instances;		// could be stacked, the deepest at the top (back)
	};
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()
	//
	class CAdapterDocument : public CDocument
	{
	public:
		CAdapterDocument( serial::IStreamable* pStreamable, const fs::CPath& docPath );
		CAdapterDocument( CObject* pObject, const fs::CPath& docPath );
		virtual ~CAdapterDocument() {}

		bool Load( void ) throws_();
		bool Save( void ) throws_();

		virtual void ReportSaveLoadException( const TCHAR* pFilePath, CException* pExc, BOOL isSaving, UINT idDefaultPrompt );
	protected:
		// base overrides
		virtual void Serialize( CArchive& archive );
	private:
		serial::IStreamable* m_pStreamable;			// use either one or the other
		CObject* m_pObject;
	};
}


namespace ui
{
	template< typename ViewT >
	ViewT* FindDocumentView( const CDocument* pDoc )
	{
		ASSERT_PTR( pDoc );

		for ( POSITION pos = pDoc->GetFirstViewPosition(); pos != nullptr; )
			if ( ViewT* pView = dynamic_cast<ViewT*>( pDoc->GetNextView( pos ) ) )
				return pView;

		return nullptr;
	}
}


namespace ui
{
	struct CTooltipTextMessage		// wrapper for TOOLTIPTEXTA/TOOLTIPTEXTW
	{
		CTooltipTextMessage( NMHDR* pNmHdr );
		CTooltipTextMessage( TOOLTIPTEXT* pNmToolTipText );

		bool IsValidNotification( void ) const;
		bool AssignTooltipText( const std::tstring& text );		// automatically configures the tooltip for multi-line display if text contains a '\n'

		bool IgnoreResourceString( void ) const;
		static bool IgnoreResourceString( HWND hCtrl );
	public:
		CToolTipCtrl* m_pTooltip;
		TOOLTIPTEXTA* m_pTttA;
		TOOLTIPTEXTW* m_pTttW;
		UINT m_cmdId;
		HWND m_hCtrl;
		void* m_pData;		// from TOOLINFO::lParam

		static const std::tstring s_nilText;		// use to preventing tooltips loaded by default
	};
}


namespace mfc
{
	// CMDIFrameWnd algorithms:

	inline CMDIFrameWnd* GetMainMdiFrameWnd( void ) { return checked_static_cast<CMDIFrameWnd*>( AfxGetMainWnd() ); }

	CMDIChildWnd* GetFirstMdiChildFrame( const CMDIFrameWnd* pMdiFrameWnd = mfc::GetMainMdiFrameWnd() );	// in display order (Z-order): GW_HWNDLAST

	template< typename MdiChildFrameT, typename FuncT >
	FuncT ForEach_MdiChildFrame( FuncT func, const MdiChildFrameT* pExceptMdiChild = nullptr )
	{
		// iterate in MDI child frame in display order (Z-order), which is reverese order GW_HWNDLAST -> GW_HWNDPREV (Z-top -> Z-bottm)
		for ( CWnd* pChild = mfc::GetFirstMdiChildFrame(); pChild != nullptr; pChild = pChild->GetNextWindow( GW_HWNDPREV ) )
			if ( MdiChildFrameT* pMdiChild = dynamic_cast<MdiChildFrameT*>( pChild ) )
				if ( pMdiChild != pExceptMdiChild )		// exclude exception, if any
					func( pMdiChild );

		return func;
	}
}


#endif // MfcUtilities_h
