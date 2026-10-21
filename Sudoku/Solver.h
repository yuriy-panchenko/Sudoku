#pragma once
#include <vector>
#include <memory>
#include "game.h"

namespace solver
{
	class Brute
	{
		class Node
		{
			game::Table tab;
			std::vector<Node*> nodes;

		public:
			Node(game::Table&& t) :tab{ std::move(t) } {}
			~Node() { for (auto p : nodes)delete p; }

			bool extend()
			{
				game::Candidates pos;
				bool found_single;
				do
				{
					pos = tab.Repopulate();
					found_single = false;
					for (size_t i = 0; i < pos.size(); i++)
						if (!tab[i] && pos[i].size() == 1)
						{
							tab[i] = *pos[i].begin();
							found_single = true;
						}
				} while (found_single);

				if (GameOver())
					return true;
				if (!IsValid(pos))
					return false;

				for (size_t i = 0; i < pos.size(); i++)
				{
					nodes.reserve(pos[i].size());
					for (auto val : pos[i])
					{
						auto nTab{ tab };
						nTab[i] = val;

						auto ptr{ std::make_unique<Node>(std::move(nTab)) };
						if (ptr->extend())
							nodes.push_back(ptr.release());
					}
				}
				if (nodes.empty())
					return false;
				else
					return true;
			}

		private:
			bool GameOver()const
			{
				for (auto val : tab)
					if (!val)
						return false;

				ASSERT(tab.is_valid());
				return true;
			}

			bool IsValid(game::Candidates const& cnd)const
			{
				for (size_t i = 0; i < tab.size(); i++)
					if (!tab[i])
						if (cnd[i].empty())
							return false;
				return true;
			}

		} *m_pRoot{ nullptr };

	public:
		Brute(game::Table const& origin)
		{
			m_pRoot = new Node{ game::Table{ origin } };
			m_pRoot->extend();
		}

		~Brute() { delete m_pRoot; }
	};
}
