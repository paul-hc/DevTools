#ifndef ContentFitBase_h
#define ContentFitBase_h
#pragma once


class CLayoutEngine;


namespace ui
{
	// provides support for size to fit contents in controls
	//
	abstract class CContentFitBase
	{
	protected:
		CContentFitBase( CWnd* pCtrl, int contentFitFlags = 0 );
	public:
		enum ContentFitFlags { FitHorizontally = 1 << 0, FitVertically = 1 << 1, FitAllowShrinking = 1 << 2 };

		bool HasContentFitFlag( int contentFitFlag ) const { return HasFlag( m_contentFitFlags, contentFitFlag ); }
		int GetContentFitFlags( void ) const { return m_contentFitFlags; }
		void SetContentFitFlags( int contentFitFlags );

		bool ResizeToFit( int fitFlags );

		// overrideables
		virtual CSize ComputeIdealSize( void ) = 0;
		virtual CSize ComputeIdealTextSize( void );
	protected:
		virtual UINT GetDrawTextFlags( void ) const;
		virtual void OnContentChanged( void );
	private:
		CWnd* m_pCtrl;
		int m_contentFitFlags;
	};
}


#endif // ContentFitBase_h
