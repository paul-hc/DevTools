
#include "stdafx.h"
#include "GeneralOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("Settings\\GeneralOptions");
	static const TCHAR entry_smallIconStdSize[] = _T("SmallIconStdSize");
	static const TCHAR entry_largeIconStdSize[] = _T("LargeIconStdSize");
}


CGeneralOptions::CGeneralOptions( void )
	: m_smallIconStdSize( SmallIcon )
	, m_largeIconStdSize( HugeIcon_48 )
{
}

CGeneralOptions::~CGeneralOptions()
{
}

CGeneralOptions& CGeneralOptions::Instance( void )
{
	static CGeneralOptions generalOptions;
	return generalOptions;
}

void CGeneralOptions::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();

	int smallStdSize = static_cast< IconStdSize >( pApp->GetProfileInt( reg::section, reg::entry_smallIconStdSize, CIconId::GetStdSize( m_smallIconStdSize ).cx ) );
	int largeStdSize = static_cast< IconStdSize >( pApp->GetProfileInt( reg::section, reg::entry_largeIconStdSize, CIconId::GetStdSize( m_largeIconStdSize ).cx ) );

	m_smallIconStdSize = CIconId::FindStdSize( CSize( smallStdSize, smallStdSize ), SmallIcon );
	m_largeIconStdSize = CIconId::FindStdSize( CSize( largeStdSize, largeStdSize ), HugeIcon_48 );
}

void CGeneralOptions::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section, reg::entry_smallIconStdSize, CIconId::GetStdSize( m_smallIconStdSize ).cx );
	pApp->WriteProfileInt( reg::section, reg::entry_largeIconStdSize, CIconId::GetStdSize( m_largeIconStdSize ).cx );
}

const std::tstring& CGeneralOptions::GetCode( void ) const
{
	static const std::tstring s_code = _T("General");
	return s_code;
}

bool CGeneralOptions::operator==( const CGeneralOptions& right ) const
{
	return
		m_smallIconStdSize == right.m_smallIconStdSize &&
		m_largeIconStdSize == right.m_largeIconStdSize;
}
