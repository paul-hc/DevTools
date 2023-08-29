#ifndef BaseStockContentCtrl_hxx
#define BaseStockContentCtrl_hxx


// CBaseStockContentCtrl template code

template< typename BaseCtrlT >
inline CBaseStockContentCtrl<BaseCtrlT>::CBaseStockContentCtrl( void )
	: BaseCtrlT()
	, m_duringCreation( false )
{
}

template< typename BaseCtrlT >
BOOL CBaseStockContentCtrl<BaseCtrlT>::PreCreateWindow( CREATESTRUCT& cs )
{
	m_duringCreation = true;		// skip stock content set up on pre-subclassing, do it later on WM_CREATE

	return __super::PreCreateWindow( cs );
}

template< typename BaseCtrlT >
void CBaseStockContentCtrl<BaseCtrlT>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( !m_duringCreation )
		InitStockContent();			// CComboBox::AddString() fails during creation
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseStockContentCtrl, BaseCtrlT, BaseCtrlT )
	ON_WM_CREATE()
END_MESSAGE_MAP()

template< typename BaseCtrlT >
int CBaseStockContentCtrl<BaseCtrlT>::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

	if ( m_duringCreation )
	{
		InitStockContent();			// delayed stock content initialization
		m_duringCreation = false;
	}
	return 0;
}


#endif // BaseStockContentCtrl_hxx
