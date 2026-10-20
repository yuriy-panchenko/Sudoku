#pragma once
#include <array>

namespace game
{
	using Line = std::array<unsigned char, 9>;
	using table_t = std::array<unsigned char, 9 * 9>;

	bool is_valid(Line const&);

	class Table
		:public table_t
	{
	public:
		static constexpr table_t initial =
		{
			1,2,3,4,5,6,7,8,9,
			4,5,6,7,8,9,1,2,3,
			7,8,9,1,2,3,4,5,6,
			2,3,4,5,6,7,8,9,1,
			5,6,7,8,9,1,2,3,4,
			8,9,1,2,3,4,5,6,7,
			3,4,5,6,7,8,9,1,2,
			6,7,8,9,1,2,3,4,5,
			9,1,2,3,4,5,6,7,8,
		};

	public:
		Table(size_t shuffle_count);
		Table(Table const&, size_t remain);

		Line get_row(size_t)const;
		Line get_column(size_t)const;
		Line get_room(size_t)const;

		void set_row(size_t, Line);
		void set_column(size_t, Line);

		bool is_valid()const;
	};

	class Sudoku
	{
	public:
		Sudoku() = default;
		Sudoku(size_t digits_remain, size_t shuffle_count);
		Sudoku(Table&&);

	private:
		Table m_Origin, m_Game;
	};
}