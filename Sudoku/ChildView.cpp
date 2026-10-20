
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Sudoku.h"
#include "ChildView.h"
#include "game.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
	:m_Game{ 20, 20 }
{}

CChildView::~CChildView()
{}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);

	return TRUE;
}

void CChildView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect canvas;
	GetClientRect(canvas);
	int const big_side{ min(canvas.Height(), canvas.Width()) };
	auto const cp{ canvas.CenterPoint() };
	CRect const rTable{ CPoint{ cp.x - big_side / 2, cp.y - big_side / 2 }, CSize{ big_side, big_side } };

	auto& tab{ m_Game.GetTable() };
	CString s;

	for (size_t iY = 0; iY < 9; iY++)
		for (size_t iX = 0; iX < 9; iX++)
			if (auto val{ tab.at(iY,iX) })
			{
				s.Format(_T("%d"), val);
				dc.TextOut(200 + (int)iX * 40, 150 + (int)iY * 30, s);
			}
}

