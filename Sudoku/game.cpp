#include "pch.h"
#include "game.h"
#include <set>
#include <vector>

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

	Table::Table(size_t shuffle_count)
		:table_t{ initial }
	{
		auto pick_pair = []()->std::pair<size_t, size_t>
			{
				size_t a{ size_t(rand() % 9) },
					b{ size_t(rand() % 8) };
				if (b >= a)
				{
					++b;
					if (b == 8)
						b = 0;
				}
				return { a, b };
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

	Sudoku::Sudoku(size_t digits_remain, size_t shuffle_count)
		:m_Origin{ shuffle_count }
		, m_Game{ m_Origin, digits_remain }
	{}
}