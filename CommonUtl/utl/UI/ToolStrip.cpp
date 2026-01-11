
#include "pch.h"
#include "ToolStrip.h"
#include "Icon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CToolStrip implementation

CToolStrip::~CToolStrip()
{
}

CToolStrip& CToolStrip::AddButton( UINT buttonId, UINT iconId /*= (UINT)UseButtonId*/ )
{
	HICON hIcon = nullptr;

	if ( buttonId != ID_SEPARATOR )
	{
		CIconId btnIconId( buttonId, GetIconStdSize() );

		switch ( iconId )
		{
			case NullIconId:
				break;
			case UseButtonId:
				hIcon = ui::GetImageStoresSvc()->RetrieveIcon( btnIconId )->GetSafeHandle();
				break;
			default:
			{
				hIcon = ui::GetImageStoresSvc()->RetrieveIcon( CIconId( iconId, GetIconStdSize() ) )->GetSafeHandle();
				if ( nullptr == hIcon )
				{
					ASSERT( false );		// missing icon resource: fallback to loading the button icon
					hIcon = ui::GetImageStoresSvc()->RetrieveIcon( btnIconId )->GetSafeHandle();
				}
			}
		}

		if ( nullptr == hIcon )
		{	// missing store icon: use a placeholder
			TRACE_FL( ":\n (!) Missing image for toolstrip button: buttonId=%d,  iconId=%d\n", buttonId, iconId );
			hIcon = CIcon::GetUnknownIcon().GetHandle();
		}
	}

	return AddButton( buttonId, hIcon );
}

CToolStrip& CToolStrip::AddButton( UINT buttonId, HICON hIcon )
{
	if ( hIcon != nullptr )
	{
		if ( nullptr == m_pImageList.get() )
			CreateImageList();

		ENSURE( GetImageSize() == CIconInfo( hIcon ).m_size );		// ensure equivalent size to prevent scaling distorsions
		VERIFY( m_pImageList->Add( hIcon ) >= 0 );
	}
	else if ( buttonId != ID_SEPARATOR )
	{
		// (*) SPECIAL CASE: toolbar buttons that use controls must have a placeholder image associated; otherwise the image list gets shifted completely;
		// To get rid of this warning, register a command alias for { buttonId, ID_EDIT_DETAILS }
		//
		TRACE_FL( _T(":\n ** Missing image for button id %d (hex=0x%04X) - using the placeholder image IDI_UNKNOWN **\n"), buttonId, buttonId );
		return AddButton( buttonId, CIcon::GetUnknownIcon().GetHandle() );
	}

	m_buttonIds.push_back( buttonId );
	return *this;
}

CToolStrip& CToolStrip::AddButtons( const UINT buttonIds[], size_t buttonCount, IconStdSize iconStdSize /*= SmallIcon*/ )
{
	ASSERT( buttonIds != nullptr && buttonCount != 0 );

	SetImageSize( iconStdSize );

	for ( size_t i = 0; i != buttonCount; ++i )
		AddButton( buttonIds[ i ] );

	return *this;
}
