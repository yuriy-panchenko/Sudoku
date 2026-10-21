
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
	, m_iSelected{ -1 }
{}

CChildView::~CChildView()
{}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
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
	constexpr COLORREF clrSelectedBorder = RGB(30, 100, 210);
	constexpr COLORREF clrDeadBg = RGB(255, 205, 205);
	constexpr COLORREF clrDeadBorder = RGB(200, 40, 40);
	constexpr COLORREF clrThinLine = RGB(176, 184, 198);
	constexpr COLORREF clrThickLine = RGB(40, 48, 66);
	constexpr COLORREF clrGiven = RGB(28, 34, 48);
	constexpr COLORREF clrPlayer = RGB(76, 92, 48);
	constexpr COLORREF clrCandidate = RGB(80, 92, 116);			// legal and selected
	constexpr COLORREF clrCandidateFaint = RGB(196, 203, 215);	// legal, but erased by the player
	constexpr COLORREF clrCandidateHover = RGB(200, 200, 20);	// text color of the candidate under the mouse

	void MakeFont(CFont& font, int px, int weight, bool italic = false)
	{
		LOGFONT lf{};
		lf.lfHeight = -px;
		lf.lfWeight = weight;
		lf.lfItalic = italic;
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

	m_rTable.SetRectEmpty();
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

		// 1. Backgrounds: checkerboard of 3x3 rooms, cells with no legal digit in red.
		for (size_t iY = 0; iY < 9; iY++)
			for (size_t iX = 0; iX < 9; iX++)
			{
				bool const tinted{ ((iX / 3 + iY / 3) % 2) != 0 };
				COLORREF clr{ tinted ? clrRoomTint : clrRoomLight };
				if (m_Game.IsDead(iY * 9 + iX))
					clr = clrDeadBg;
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

		// 3. Frames on top of the grid: thick red for contradictions, 1px blue for the selection.
		auto frame = [&](CRect const& r, int t, COLORREF clr)
			{
				dc.FillSolidRect(r.left, r.top, r.Width(), t, clr);
				dc.FillSolidRect(r.left, r.bottom - t, r.Width(), t, clr);
				dc.FillSolidRect(r.left, r.top, t, r.Height(), clr);
				dc.FillSolidRect(r.right - t, r.top, t, r.Height(), clr);
			};

		int const thickDead{ max(3, cell / 12) };
		for (size_t iY = 0; iY < 9; iY++)
			for (size_t iX = 0; iX < 9; iX++)
				if (m_Game.IsDead(iY * 9 + iX))
				{
					CRect r{ cell_rect(iX, iY) };
					r.DeflateRect(1, 1);
					frame(r, thickDead, clrDeadBorder);
				}

		if (m_iSelected >= 0)
		{
			CRect r{ cell_rect(m_iSelected % 9, m_iSelected / 9) };
			//r.DeflateRect(1, 1);
			if (m_Game.IsDead(m_iSelected))
				r.DeflateRect(thickDead, thickDead);	// keep the red frame visible
			frame(r, 2, clrSelectedBorder);
		}

		// 4. Digits and candidates.
		CFont fontGiven, fontPlayer, fontCandidate, fontCandidateFaint;
		MakeFont(fontGiven, cell * 75 / 100, FW_BOLD);
		MakeFont(fontPlayer, cell * 75 / 100, FW_NORMAL);
		MakeFont(fontCandidate, max(6, cell * 22 / 100), FW_NORMAL);
		MakeFont(fontCandidateFaint, max(5, cell * 17 / 100), FW_NORMAL, true);

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
					dc.SetTextColor(given ? clrGiven : clrPlayer);

					CString s;
					s.Format(_T("%d"), (int)val);
					dc.DrawText(s, r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
				else if (m_bShowCandidates)
				{
					// Every legal digit is drawn: solid if still selected, faint (smaller, italic) if the player erased it.
					int const sub{ cell / 3 };
					for (auto d : m_Game.Legal(ind))
					{
						bool const selected{ possible[ind].count(d) != 0 };
						bool const hovered{ (int)ind == m_Hover.cell && d == m_Hover.digit };
						dc.SelectObject(selected ? &fontCandidate : &fontCandidateFaint);
						dc.SetTextColor(hovered ? clrCandidateHover : selected ? clrCandidate : clrCandidateFaint);
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

CChildView::Hit CChildView::HitTest(CPoint pt) const
{
	Hit hit;
	if (m_rTable.IsRectEmpty() || !m_rTable.PtInRect(pt))
		return hit;

	// Same integer geometry as OnPaint: the board is 9 cells of `cell` pixels, a cell holds 3x3 slots of `cell / 3`.
	int const cell{ m_rTable.Width() / 9 };
	int const sub{ cell / 3 };
	int const x{ pt.x - m_rTable.left }, y{ pt.y - m_rTable.top };

	hit.cell = y / cell * 9 + x / cell;
	hit.digit = min((y % cell) / sub, 2) * 3 + min((x % cell) / sub, 2) + 1;
	return hit;
}

void CChildView::EnterNumber(unsigned char val)
{
	if (m_iSelected < 0 || !m_Game.SetNumber(m_iSelected, val))
		return;

	Invalidate(FALSE);

	if (val && m_Game.IsComplete())
	{
		UpdateWindow();	// show the finished board before the message
		AfxMessageBox(_T("Solved!"));
	}
}

void CChildView::ToggleSelectedCandidate(unsigned char digit)
{
	if (m_iSelected >= 0 && m_bShowCandidates && m_Game.ToggleCandidate(m_iSelected, digit))
		Invalidate(FALSE);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();	// keyboard input goes to the focused window

	auto const hit{ HitTest(point) };
	if (hit.cell < 0)
		return;

	m_iSelected = hit.cell;

	// Click on a candidate: erase it if selected, put it back if it is legal here. Numbers are only selected.
	if (m_bShowCandidates)
		m_Game.ToggleCandidate(hit.cell, (unsigned char)hit.digit);

	Invalidate(FALSE);
}

void CChildView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	auto const hit{ HitTest(point) };
	if (hit.cell < 0)
		return;

	m_iSelected = hit.cell;

	if (m_Game.GetTable()[hit.cell])
	{
		EnterNumber(0);	// double-click on a Number removes it (givens ignore this)
		return;
	}

	if (!m_bShowCandidates)
		return;

	// The first click of this double-click has just toggled the candidate. If that erased it, put it back
	// (a no-op for illegal digits), so placing works the same for solid and faint candidates.
	auto const digit{ (unsigned char)hit.digit };
	if (!m_Game.GetPossible()[hit.cell].count(digit))
		m_Game.ToggleCandidate(hit.cell, digit);

	EnterNumber(digit);
	Invalidate(FALSE);
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	enum { idRepopulate = 1, idShowCandidates };

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, idRepopulate, _T("Repopulate all candidates"));
	menu.AppendMenu(MF_STRING | (m_bShowCandidates ? MF_CHECKED : MF_UNCHECKED), idShowCandidates, _T("Show candidates"));

	ClientToScreen(&point);
	switch (menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this))
	{
	case idRepopulate:
		m_Game.RepopulateCandidates();
		break;
	case idShowCandidates:
		m_bShowCandidates = !m_bShowCandidates;
		break;
	default:
		return;
	}

	Invalidate(FALSE);
}

void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 1..9 work on candidates, Shift+0..9 on Numbers (Shift+0 removes the Number). The virtual key of a digit
	// does not depend on Shift, so this is layout independent.
	int digit{ -1 };
	if (nChar >= _T('0') && nChar <= _T('9'))
		digit = (int)(nChar - _T('0'));
	else if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
		digit = (int)(nChar - VK_NUMPAD0);

	if (digit >= 0)
	{
		if (::GetKeyState(VK_SHIFT) & 0x8000)
			EnterNumber((unsigned char)digit);
		else if (digit > 0)
			ToggleSelectedCandidate((unsigned char)digit);
		return;
	}

	int const sel{ m_iSelected < 0 ? 40 : m_iSelected };
	int x{ sel % 9 }, y{ sel / 9 };

	switch (nChar)
	{
	case VK_DELETE:
	case VK_BACK:	EnterNumber(0);			return;
	case VK_LEFT:	x = max(0, x - 1);		break;
	case VK_RIGHT:	x = min(8, x + 1);		break;
	case VK_UP:		y = max(0, y - 1);		break;
	case VK_DOWN:	y = min(8, y + 1);		break;
	default:		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);	return;
	}

	m_iSelected = y * 9 + x;
	Invalidate(FALSE);
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTrackingMouse)	// ask for WM_MOUSELEAVE, so the highlight goes away when the mouse leaves the window
	{
		TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, m_hWnd, 0 };
		m_bTrackingMouse = ::TrackMouseEvent(&tme);
	}

	// Only a candidate slot (a legal digit of an empty cell) is highlighted.
	Hit hit{ HitTest(point) };
	if (!m_bShowCandidates || hit.cell < 0 || m_Game.GetTable()[hit.cell] || !m_Game.Legal(hit.cell).count((unsigned char)hit.digit))
		hit = Hit{};

	if (hit.cell != m_Hover.cell || hit.digit != m_Hover.digit)
	{
		m_Hover = hit;
		Invalidate(FALSE);
	}
}

void CChildView::OnMouseLeave()
{
	m_bTrackingMouse = FALSE;

	if (m_Hover.cell >= 0)
	{
		m_Hover = Hit{};
		Invalidate(FALSE);
	}
}
