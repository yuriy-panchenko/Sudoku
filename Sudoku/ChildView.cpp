
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
	, m_bShowCandidates{ TRUE }
{}

CChildView::~CChildView()
{}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
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

namespace
{
	constexpr COLORREF clrBackdrop = RGB(236, 239, 244);
	constexpr COLORREF clrRoomLight = RGB(255, 255, 255);
	constexpr COLORREF clrRoomTint = RGB(226, 232, 242);
	constexpr COLORREF clrConflictBg = RGB(255, 205, 205);
	constexpr COLORREF clrThinLine = RGB(176, 184, 198);
	constexpr COLORREF clrThickLine = RGB(40, 48, 66);
	constexpr COLORREF clrGiven = RGB(28, 34, 48);
	constexpr COLORREF clrPlayer = RGB(30, 100, 210);
	constexpr COLORREF clrConflictTxt = RGB(190, 30, 30);
	constexpr COLORREF clrCandidate = RGB(130, 140, 158);

	void MakeFont(CFont& font, int px, int weight)
	{
		LOGFONT lf{};
		lf.lfHeight = -px;
		lf.lfWeight = weight;
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfQuality = CLEARTYPE_QUALITY;
		_tcscpy_s(lf.lfFaceName, _T("Segoe UI"));
		font.CreateFontIndirect(&lf);
	}
}

BOOL CChildView::OnEraseBkgnd(CDC*)
{
	return TRUE; // OnPaint covers the whole client area, erasing here only causes flicker
}

void CChildView::OnPaint()
{
	CPaintDC paint{ this };

	CRect client;
	GetClientRect(client);
	if (client.IsRectEmpty())
		return;

	// Render off-screen, then blit once.
	CDC dc;
	dc.CreateCompatibleDC(&paint);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&paint, client.Width(), client.Height());
	auto* const oldBmp{ dc.SelectObject(&bmp) };

	dc.FillSolidRect(client, clrBackdrop);

	int const margin{ 16 };
	int const cell{ (min(client.Width(), client.Height()) - 2 * margin) / 9 };

	if (cell >= 12)
	{
		int const side{ cell * 9 };
		CPoint const origin{ client.CenterPoint().x - side / 2, client.CenterPoint().y - side / 2 };
		m_rTable = { origin, CSize{ side, side } };

		auto cell_rect = [&](size_t x, size_t y)
			{
				return CRect{ CPoint{ origin.x + (int)x * cell, origin.y + (int)y * cell }, CSize{ cell, cell } };
			};

		auto& tab{ m_Game.GetTable() };
		auto& possible{ m_Game.GetPossible() };

		// 1. Backgrounds: checkerboard of 3x3 rooms, conflicting cells in red.
		for (size_t iY = 0; iY < 9; iY++)
			for (size_t iX = 0; iX < 9; iX++)
			{
				bool const tinted{ ((iX / 3 + iY / 3) % 2) != 0 };
				COLORREF clr{ tinted ? clrRoomTint : clrRoomLight };
				if (m_Game.IsConflict(iY * 9 + iX))
					clr = clrConflictBg;
				dc.FillSolidRect(cell_rect(iX, iY), clr);
			}

		// 2. Grid: thin lines first, room borders on top.
		int const thick{ max(2, cell / 24) };
		for (int i = 0; i <= 9; i++)
		{
			if (i % 3 == 0)
				continue;
			dc.FillSolidRect(origin.x + i * cell, origin.y, 1, side, clrThinLine);
			dc.FillSolidRect(origin.x, origin.y + i * cell, side, 1, clrThinLine);
		}
		for (int i = 0; i <= 9; i += 3)
		{
			dc.FillSolidRect(origin.x + i * cell - thick / 2, origin.y - thick / 2, thick, side + thick, clrThickLine);
			dc.FillSolidRect(origin.x - thick / 2, origin.y + i * cell - thick / 2, side + thick, thick, clrThickLine);
		}

		// 3. Digits and candidates.
		CFont fontGiven, fontPlayer, fontCandidate;
		MakeFont(fontGiven, cell * 75 / 100, FW_BOLD);
		MakeFont(fontPlayer, cell * 75 / 100, FW_NORMAL);
		MakeFont(fontCandidate, max(6, cell * 22 / 100), FW_NORMAL);

		dc.SetBkMode(TRANSPARENT);
		auto* const oldFont{ dc.SelectObject(&fontGiven) };

		for (size_t iY = 0; iY < 9; iY++)
			for (size_t iX = 0; iX < 9; iX++)
			{
				auto const ind{ iY * 9 + iX };
				CRect r{ cell_rect(iX, iY) };

				if (auto const val{ tab.at(iY, iX) })
				{
					bool const given{ m_Game.IsGiven(ind) };
					dc.SelectObject(given ? &fontGiven : &fontPlayer);
					dc.SetTextColor(m_Game.IsConflict(ind) ? clrConflictTxt : given ? clrGiven : clrPlayer);

					CString s;
					s.Format(_T("%d"), (int)val);
					dc.DrawText(s, r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
				else if (m_bShowCandidates)
				{
					dc.SelectObject(&fontCandidate);
					dc.SetTextColor(clrCandidate);

					int const sub{ cell / 3 };
					for (auto d : possible[ind])
					{
						int const sx{ (d - 1) % 3 }, sy{ (d - 1) / 3 };
						CRect sr{ r.left + sx * sub, r.top + sy * sub, r.left + (sx + 1) * sub, r.top + (sy + 1) * sub };
						TCHAR const ch{ (TCHAR)(_T('0') + d) };
						dc.DrawText(&ch, 1, sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					}
				}
			}

		dc.SelectObject(oldFont);
	}

	paint.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
	dc.SelectObject(oldBmp);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_rTable.PtInRect(point))
	{
		auto OneIn = [point](CRect r, int in)->CPoint
			{
				return { (point.x - r.left) * in / r.Width(),
					(point.y - r.top) * in / r.Height() };
			};

		auto rect{ m_rTable };
		auto const cell_size{ rect.Width() / 9. };

		auto const coo_tab{ OneIn(rect, 9) };
		ASSERT(coo_tab.x > -1 && coo_tab.x < 9);
		ASSERT(coo_tab.y > -1 && coo_tab.y < 9);

		rect.left += int(coo_tab.x * cell_size);
		rect.right = m_rTable.left + int((coo_tab.x + 1) * cell_size);
		rect.top += int(coo_tab.y * cell_size);
		rect.bottom = m_rTable.top + int((coo_tab.y + 1) * cell_size);

		auto const coo_cell{ OneIn(rect,3) };
		ASSERT(coo_cell.x > -1 && coo_cell.x < 3);
		ASSERT(coo_cell.y > -1 && coo_cell.y < 3);

		/*int const gX{ (point.x - m_rTable.left) * 9 / m_rTable.Width() },
			gY{ (point.y - m_rTable.top) * 9 / m_rTable.Height() };
		ASSERT(gX > -1 && gX < 9);
		ASSERT(gY > -1 && gY < 9);*/
	}
}
