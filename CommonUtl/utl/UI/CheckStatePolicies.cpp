
#include "stdafx.h"
#include "CheckStatePolicies.h"
#include "ThemeItem.h"
#include "ReportListControl.h"
#include "Image_fwd.h"
#include "ui_fwd.h"
#include "utl/ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	bool IsCheckBoxState( int checkState, const ui::ICheckStatePolicy* pCheckStatePolicy /*= NULL*/ )
	{
		switch ( checkState )
		{
			case BST_UNCHECKED:
			case BST_CHECKED:
				return NULL == pCheckStatePolicy || pCheckStatePolicy->IsEnabledState( checkState );
		}
		return false;
	}

	void AppendToStateImageList( CImageList* pStateImageList, const std::vector< CThemeItem >& themeItems, COLORREF transpBkColor )
	{
		ASSERT_PTR( pStateImageList->GetSafeHandle() );
		CSize imageSize = gdi::GetImageSize( *pStateImageList );

		for ( std::vector< CThemeItem >::const_iterator itThemeItem = themeItems.begin(); itThemeItem != themeItems.end(); ++itThemeItem )
		{
			CBitmap imageBitmap, maskBitmap;

			itThemeItem->MakeBitmap( imageBitmap, transpBkColor, imageSize, H_AlignLeft | V_AlignTop );
			gdi::CreateBitmapMask( maskBitmap, imageBitmap, transpBkColor );
			pStateImageList->Add( &imageBitmap, &maskBitmap );
		}
	}
}


// CheckEx implementation

const ui::ICheckStatePolicy* CheckEx::Instance( void )
{
	static CheckEx s_checkStatePolicy;
	return &s_checkStatePolicy;
}

bool CheckEx::IsCheckedState( int checkState ) const
{
	return Checked == checkState;
}

bool CheckEx::IsEnabledState( int checkState ) const
{
	return checkState != Implicit;
}

int CheckEx::Toggle( int checkState ) const
{
	if ( Implicit == checkState )
		return checkState;

	++checkState;
	checkState %= Implicit;
	return checkState;
}

const std::vector< CThemeItem >* CheckEx::GetThemeItems( void ) const
{
	static std::vector< CThemeItem > s_items;
	if ( s_items.empty() )
	{
		s_items.reserve( _Count );
		s_items.push_back( CThemeItem( L"BUTTON", BP_CHECKBOX, CBS_MIXEDNORMAL ) );				// Mixed
		s_items.push_back( CThemeItem( L"BUTTON", BP_CHECKBOX, CBS_IMPLICITNORMAL ) );			// Implicit
	}
	return &s_items;
}


// CheckRadio implementation

const ui::ICheckStatePolicy* CheckRadio::Instance( void )
{
	static CheckRadio s_checkStatePolicy;
	return &s_checkStatePolicy;
}

bool CheckRadio::IsCheckedState( int checkState ) const
{
	switch ( checkState )
	{
		case Checked:
		case CheckedDisabled:
		case RadioChecked:
		case RadioCheckedDisabled:
			return true;
	}
	return false;
}

bool CheckRadio::IsRadioState( int checkState ) const
{
	switch ( checkState )
	{
		case RadioUnchecked:
		case RadioChecked:
		case RadioUncheckedDisabled:
		case RadioCheckedDisabled:
			return true;
	}
	return false;
}

bool CheckRadio::IsEnabledState( int checkState ) const
{
	return checkState <= RadioChecked;
}

int CheckRadio::Toggle( int checkState ) const
{
	switch ( checkState )
	{
		case RadioUnchecked:
		case Unchecked:
			++checkState;
			break;
		case Checked:
			--checkState;
			break;
	}
	return checkState;
}

const std::vector< CThemeItem >* CheckRadio::GetThemeItems( void ) const
{
	static std::vector< CThemeItem > s_items;
	if ( s_items.empty() )
	{
		s_items.reserve( _Count );
		s_items.push_back( CThemeItem( L"BUTTON", BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL ) );		// RadioUnchecked
		s_items.push_back( CThemeItem( L"BUTTON", BP_RADIOBUTTON, RBS_CHECKEDNORMAL ) );		// RadioChecked
		s_items.push_back( CThemeItem( L"BUTTON", BP_CHECKBOX, CBS_UNCHECKEDDISABLED ) );		// UncheckedDisabled
		s_items.push_back( CThemeItem( L"BUTTON", BP_CHECKBOX, CBS_CHECKEDDISABLED ) );			// CheckedDisabled
		s_items.push_back( CThemeItem( L"BUTTON", BP_RADIOBUTTON, RBS_UNCHECKEDDISABLED ) );	// RadioUncheckedDisabled
		s_items.push_back( CThemeItem( L"BUTTON", BP_RADIOBUTTON, RBS_CHECKEDDISABLED ) );		// RadioCheckedDisabled
	}
	return &s_items;
}


void CheckRadio::CheckRadioItems( CReportListControl* pListCtrl, int firstRadioIndex, UINT itemCount, int checkedPos, bool isEnabled /*= true*/ )
{
	ASSERT_PTR( pListCtrl->GetSafeHwnd() );

	CheckState uncheckedState = MakeCheckState( true, false, isEnabled );

	// uncheck all other radio button items
	for ( int i = 0; i != (int)itemCount; ++i )
		if ( i != checkedPos )
			pListCtrl->ModifyCheckState( firstRadioIndex + i, uncheckedState );

	// finally check the one radio button
	pListCtrl->SetCheckState( firstRadioIndex + checkedPos, MakeCheckState( true, true, isEnabled ) );
}

CheckRadio::CheckState CheckRadio::MakeCheckState( bool isRadio, bool isChecked, bool isEnabled )
{
	if ( isRadio )
		if ( isChecked )
			return isEnabled ? RadioChecked : RadioCheckedDisabled;
		else
			return isEnabled ? RadioUnchecked : RadioUncheckedDisabled;
	else
		if ( isChecked )
			return isEnabled ? Checked : CheckedDisabled;
		else
			return isEnabled ? Unchecked : UncheckedDisabled;
}
