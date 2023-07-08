#ifndef SpinTargetButton_h
#define SpinTargetButton_h
#pragma once


namespace ui { interface ISpinTarget; }


class CSpinTargetButton : public CSpinButtonCtrl
{
public:
	CSpinTargetButton( CWnd* pBuddyCtrl, ui::ISpinTarget* pSpinTarget );
	virtual ~CSpinTargetButton();

	ui::ISpinTarget* GetSpinTarget( void ) const { return m_pSpinTarget; }
	void SetSpinTarget( ui::ISpinTarget* pSpinTarget ) { m_pSpinTarget = pSpinTarget; }

	bool IsBuddyEditable( void ) const;

	bool Create( DWORD alignment = UDS_ALIGNRIGHT );
	void UpdateState( void );
	void Layout( void );
private:
	CWnd* m_pBuddyCtrl;					// typically a CEdit
	ui::ISpinTarget* m_pSpinTarget;

	// generated stuff
protected:
	afx_msg BOOL OnUdnDeltaPos( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // SpinTargetButton_h
