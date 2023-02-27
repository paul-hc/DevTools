#ifndef ScopedBitmapMemDC_h
#define ScopedBitmapMemDC_h
#pragma once


namespace bmp
{
	class CMemDC;


	// base for objects that have a bitmap handle and require a shared source mem DC with bitmap selected into it

	abstract class CSharedAccess
	{
		friend class CMemDC;
	protected:
		CSharedAccess( void ) : m_pMemDC( nullptr ) {}

		virtual CSharedAccess* GetTarget( void ) const { return const_cast<CSharedAccess*>( this ); }		// overridden for proxy access redirection
	public:
		virtual HBITMAP GetHandle( void ) const = 0;

		CDC* GetBitmapMemDC( void ) const { return (CDC*)GetTarget()->m_pMemDC; }
		bool HasBitmapMemDC( void ) const { return GetBitmapMemDC() != nullptr; }
		bool HasValidBitmapMemDC( void ) const { return GetBitmapMemDC()->GetSafeHdc() != nullptr; }
	private:
		mutable CMemDC* m_pMemDC;			// allow mem DC for const source
	};


	// DC compatible with screen DC that keeps the bitmap selected into it

	class CMemDC : public CDC
	{
	public:
		CMemDC( const CSharedAccess* pAccess, CDC* pTemplateDC = nullptr )
			: m_pTarget( pAccess->GetTarget() )
			, m_hOldBitmap( nullptr )
		{
			ASSERT_PTR( m_pTarget );
			ASSERT_PTR( m_pTarget->GetHandle() );
			ASSERT_NULL( m_pTarget->m_pMemDC );

			if ( CreateCompatibleDC( pTemplateDC ) )			// compatible with screen DC if pTemplateDC is NULL
			{
				m_pTarget->m_pMemDC = this;
				m_hOldBitmap = SelectObject( m_pTarget->GetHandle() );
			}
		}

		~CMemDC()
		{
			ASSERT_PTR( m_pTarget );
			if ( GetSafeHdc() != nullptr )
			{
				m_pTarget->m_pMemDC = nullptr;
				if ( m_hOldBitmap != nullptr )
					SelectObject( m_hOldBitmap );
			}
		}
	private:
		CSharedAccess* m_pTarget;
		HGDIOBJ m_hOldBitmap;
	};

} //namespace bmp


// manages one shared CMemDC instance per CSharedAccess object; multiple such objects can be created auto

class CScopedBitmapMemDC
{
public:
	CScopedBitmapMemDC( const bmp::CSharedAccess* pAccess, CDC* pTemplateDC = nullptr, bool condition = true )
	{
		ASSERT_PTR( pAccess  );
		if ( !pAccess->HasBitmapMemDC() && condition )
			m_pSharedMemDC.reset( new bmp::CMemDC( pAccess, pTemplateDC ) );
	}
private:
	std::auto_ptr<bmp::CMemDC> m_pSharedMemDC;
};


#endif // ScopedBitmapMemDC_h
