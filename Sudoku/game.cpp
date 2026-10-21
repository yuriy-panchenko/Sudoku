#include "pch.h"
#include "game.h"
#include <vector>
#include <algorithm>

namespace game
{
	constexpr auto shuffle_count{ 20ull };

	bool is_valid(Line const& l)
	{
		std::set<unsigned char> s;

		for (auto ch : l)
			s.insert(ch);
		assert(s.size() == l.size());
		return s.size() == l.size();
	}

	size_t to_row(size_t ind)
	{
		return ind / 9;
	}

	size_t to_column(size_t ind)
	{
		return ind % 9;
	}

	size_t to_room(size_t ind)
	{
		return to_row(ind) / 3 * 3 + to_column(ind) / 3;
	}

	namespace
	{
		// Visits every cell that shares a row, column or room with `index` (the cell itself included).
		template<class F>
		void for_each_peer(size_t index, F&& f)
		{
			auto const row{ to_row(index) }, col{ to_column(index) }, room{ to_room(index) };
			auto const room_start{ room / 3 * 27 + room % 3 * 3 };

			for (size_t i = 0; i < 9; i++)
			{
				f(row * 9 + i);
				f(i * 9 + col);
				f(room_start + i / 3 * 9 + i % 3);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Line Table::get_row(size_t ind) const
	{
		auto itFrom{ begin() + 9 * ind }, itTo{ itFrom + 9 };
		Line ret;
		std::copy(itFrom, itTo, ret.begin());
		return ret;
	}

	Line Table::get_column(size_t ind) const
	{
		Line ret;

		for (size_t i = 0; i < 9; i++)
			ret[i] = *(begin() + i * 9 + ind);

		return ret;
	}

	Line Table::get_room(size_t ind) const
	{
		auto x{ ind % 3 }, y{ ind / 3 };
		auto const itStart{ begin() + y * 3 * 9 + x * 3 };

		Line ret;

		for (size_t i = 0; i < 9; i++)
			ret[i] = *(itStart + (i / 3) * 9 + i % 3);

		return ret;
	}

	void Table::set_row(size_t ind, Line l)
	{
		for (size_t i = 0; i < 9; i++)
			*(begin() + ind * 9 + i) = l[i];
	}

	void Table::set_column(size_t ind, Line l)
	{
		for (size_t i = 0; i < 9; i++)
			*(begin() + i * 9 + ind) = l[i];
	}

	bool Table::is_valid() const
	{
		for (size_t i = 0; i < 9; i++)
			if (!game::is_valid(get_row(i))
				|| !game::is_valid(get_column(i))
				|| !game::is_valid(get_room(i)))
				return false;
		return true;
	}

	unsigned char Table::at(size_t y, size_t x) const
	{
		return table_t::at(y * 9 + x);
	}

	Table::Table(size_t shuffle_count)
		:table_t{ initial }
	{
		// Rows may only be swapped inside one band of 3 (columns: inside one stack),
		// swapping across bands breaks the 3x3 rooms.
		auto pick_pair = []()->std::pair<size_t, size_t>
			{
				size_t const base{ size_t(rand() % 3) * 3 };
				size_t const a{ size_t(rand() % 3) };
				size_t const b{ (a + 1 + size_t(rand() % 2)) % 3 };
				return { base + a, base + b };
			};

		for (size_t i = 0; i < shuffle_count; i++)
		{
			auto p{ pick_pair() };
			auto a{ get_row(p.first) },
				b{ get_row(p.second) };
			set_row(p.first, b);
			set_row(p.second, a);

			p = pick_pair();
			a = get_column(p.first),
				b = get_column(p.second);
			set_column(p.first, b);
			set_column(p.second, a);
		}
	}

	Table::Table(Table const& origin, size_t remain)
		:table_t{}
	{
		std::vector<size_t> indx;
		indx.reserve(9 * 9);

		for (size_t i = 0; i < 9 * 9; i++)
			if (origin[i])
				indx.push_back(i);

		while (indx.size() > remain)
			indx.erase(indx.begin() + (rand() % indx.size()));

		for (auto ind : indx)
			(*this)[ind] = origin[ind];
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Sudoku::Sudoku(size_t digits_remain, size_t shuffle_count)
		:m_Origin{ shuffle_count }
		, m_Game{ m_Origin, digits_remain }
	{
		for (size_t i = 0; i < m_Game.size(); i++)
			m_Given[i] = m_Game[i];

		RepopulateCandidates();
	}

	bool Sudoku::IsComplete() const
	{
		return std::find(m_Game.begin(), m_Game.end(), 0) == m_Game.end();
	}

	bool Sudoku::IsDead(size_t index) const
	{
		return !m_Game[index] && Legal(index).empty();
	}

	std::set<unsigned char> Sudoku::Legal(size_t index) const
	{
		std::set<unsigned char> ret;
		if (m_Game[index])
			return ret;

		ret = all_possible;
		for_each_peer(index, [&](size_t i) { ret.erase(m_Game[i]); });
		return ret;
	}

	bool Sudoku::SetNumber(size_t index, unsigned char val)
	{
		if (index >= m_Game.size() || val > 9 || m_Given[index])
			return false;

		if (val == 0)
		{
			if (!m_Game[index])
				return false;

			auto const old{ m_Game[index] };
			m_Game[index] = 0;
			m_Possible[index] = Legal(index);

			// The digit is free again: revive it in the peers that nothing else blocks (Legal is empty for filled cells).
			for_each_peer(index, [&](size_t i)
				{
					if (i != index && Legal(i).count(old))
						m_Possible[i].insert(old);
				});
			return true;
		}

		if (m_Game[index] || !m_Possible[index].count(val))
			return false;

		m_Game[index] = val;
		Eliminate(index, val);
		return true;
	}

	bool Sudoku::ToggleCandidate(size_t index, unsigned char val)
	{
		if (index >= m_Game.size() || val < 1 || val > 9 || m_Game[index])
			return false;

		auto& cand{ m_Possible[index] };
		if (cand.erase(val))
			return true;

		if (!Legal(index).count(val))
			return false;

		cand.insert(val);
		return true;
	}

	void Sudoku::RepopulateCandidates()
	{
		for (size_t i = 0; i < m_Game.size(); i++)
			m_Possible[i] = Legal(i);
	}

	void Sudoku::Eliminate(size_t index, unsigned char val)
	{
		for_each_peer(index, [&](size_t i) { m_Possible[i].erase(val); });
		m_Possible[index].clear();	// a filled cell has no candidates
	}
}
