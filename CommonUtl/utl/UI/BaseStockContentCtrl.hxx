#ifndef BaseStockContentCtrl_hxx
#define BaseStockContentCtrl_hxx


// CBaseStockContentCtrl template code

template< typename BaseCtrl >
inline CBaseStockContentCtrl< BaseCtrl >::CBaseStockContentCtrl( void )
	: BaseCtrl()
	, m_duringCreation( false )
{
}

template< typename BaseCtrl >
BOOL CBaseStockContentCtrl< BaseCtrl >::PreCreateWindow( CREATESTRUCT& cs )
{
	m_duringCreation = true;		// skip stock content set up on pre-subclassing, do it later on WM_CREATE

	return BaseCtrl::PreCreateWindow( cs );
}

template< typename BaseCtrl >
void CBaseStockContentCtrl< BaseCtrl >::PreSubclassWindow( void )
{
	BaseCtrl::PreSubclassWindow();

	if ( !m_duringCreation )
		InitStockContent();			// CComboBox::AddString() fails during creation
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseStockContentCtrl, BaseCtrl, BaseCtrl )
	ON_WM_CREATE()
END_MESSAGE_MAP()

template< typename BaseCtrl >
int CBaseStockContentCtrl< BaseCtrl >::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == BaseCtrl::OnCreate( pCS ) )
		return -1;

	if ( m_duringCreation )
	{
		InitStockContent();			// delayed stock content initialization
		m_duringCreation = false;
	}
	return 0;
}


#endif // BaseStockContentCtrl_hxx
