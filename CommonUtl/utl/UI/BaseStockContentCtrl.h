#ifndef BaseStockContentCtrl_h
#define BaseStockContentCtrl_h
#pragma once


// A control that manages its content automatically on creation/sub-classing.
// Useful for controls such as CComboBox, that can't add items on PreSubclassWindow() when created via CreateWindow().
// In this case the content is initialized later, when handling WM_CREATE.
//
template< typename BaseCtrlT >
abstract class CBaseStockContentCtrl : public BaseCtrlT
{
protected:
	CBaseStockContentCtrl( void );

	// override to do the content initialization at the right moment depending on how it's sub-classed: dialog control or CreateWindow()
	virtual void InitStockContent( void ) = 0;
private:
	bool m_duringCreation;				// e.g. cannot add combo items when creating via CComboBox::Create()

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );		// called before PreSubclassWindow() when created with CreateWindow()
	virtual void PreSubclassWindow( void );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseStockContentCtrl_h
