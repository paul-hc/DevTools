#ifndef CheckStatePolicies_h
#define CheckStatePolicies_h
#pragma once


struct CThemeItem;
class CReportListControl;


namespace ui
{
	interface ICheckStatePolicy
	{
		virtual bool IsCheckedState( int checkState ) const = 0;
		virtual bool IsRadioState( int checkState ) const { checkState; return false; }
		virtual bool IsEnabledState( int checkState ) const { checkState; return true; }

		virtual int Toggle( int checkState ) const = 0;
		virtual const std::vector< CThemeItem >* GetThemeItems( void ) const { return NULL; }
	};

	bool IsCheckBoxState( int checkState, const ui::ICheckStatePolicy* pCheckStatePolicy = NULL );
	void AppendToStateImageList( CImageList* pStateImageList, const std::vector< CThemeItem >& themeItems, COLORREF transpBkColor );
}


struct CheckEx : public ui::ICheckStatePolicy
{
	enum CheckState { Unchecked = BST_UNCHECKED, Checked = BST_CHECKED, Mixed, Implicit, _Count };

	static const ui::ICheckStatePolicy* Instance( void );

	// ui::ICheckStatePolicy interface
	virtual bool IsCheckedState( int checkState ) const;
	virtual bool IsEnabledState( int checkState ) const;
	virtual int Toggle( int checkState ) const;
	virtual const std::vector< CThemeItem >* GetThemeItems( void ) const;
};


struct CheckRadio : public ui::ICheckStatePolicy
{
	enum CheckState
	{
		Unchecked = BST_UNCHECKED, Checked = BST_CHECKED, RadioUnchecked, RadioChecked,
		UncheckedDisabled, CheckedDisabled, RadioUncheckedDisabled, RadioCheckedDisabled,			// disabled states
			_Count
	};

	static const ui::ICheckStatePolicy* Instance( void );

	// ui::ICheckStatePolicy interface
	virtual bool IsCheckedState( int checkState ) const;
	virtual bool IsRadioState( int checkState ) const;
	virtual bool IsEnabledState( int checkState ) const;
	virtual int Toggle( int checkState ) const;
	virtual const std::vector< CThemeItem >* GetThemeItems( void ) const;

	static void CheckRadioItems( CReportListControl* pListCtrl, int firstRadioIndex, UINT itemCount, int checkedPos, bool isEnabled = true );
	static CheckState MakeCheckState( bool isRadio, bool isChecked, bool isEnabled );
};


#endif // CheckStatePolicies_h
