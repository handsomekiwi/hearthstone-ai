#pragma once

#include "MCTS/Tree.h"

#include "MCTS/stages/Selection.h"
#include "MCTS/stages/Simulation.h"
#include "MCTS/detail/EpisodeState.h"
#include "MCTS/detail/Statistic.h"
#include "MCTS/ActionType.h"

namespace mcts
{
	class MCTS
	{
	public:
		template <typename StartBoardGetter>
		void Iterate(StartBoardGetter&& start_board_getter);

		int ChooseAction(ActionType action_type, int choices);

		void PrintStatistic() {
			statistic_.PrintMessage();
		}

	private:
		int ActionCallback(ActionType action_type, int choices);

		void SwitchToSimulationMode() {
			// We use a flag here, since we cannot switch to simulation mode
			// in sub-actions.
			flag_switch_to_simulation_ = true;
		}

	private:
		detail::EpisodeState episode_state_;
		detail::Statistic<> statistic_;

		stages::Selection selection_stage_;
		stages::Simulation simulation_stage_;

		bool flag_switch_to_simulation_;
	};
}