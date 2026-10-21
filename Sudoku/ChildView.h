
// ChildView.h : interface of the CChildView class
//


#pragma once
#include "game.h"

// CChildView window

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

// Attributes
public:

// Operations
public:

// Overrides
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CChildView();

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()

private:
	struct Hit
	{
		int cell{ -1 };		// 0..80, -1 = outside the board
		int digit{ 0 };		// 1..9 = which candidate slot of the cell was hit
	};
	Hit HitTest(CPoint) const;

	// val 1..9 places a Number into the selected cell, 0 removes it.
	void EnterNumber(unsigned char val);

	// Erases / puts back a candidate of the selected cell.
	void ToggleSelectedCandidate(unsigned char digit);

private:
	game::Sudoku m_Game;
	BOOL m_bShowCandidates;
	CRect m_rTable;
	int m_iSelected;	// selected cell, -1 = none
	Hit m_Hover;		// candidate under the mouse, cell == -1 = none
	BOOL m_bTrackingMouse{ FALSE };
};

