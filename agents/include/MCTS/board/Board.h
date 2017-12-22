#pragma once

#include "engine/IActionParameterGetter.h"
#include "MCTS/board/BoardView.h"
#include "MCTS/Types.h"

namespace mcts
{
	namespace board
	{
		class Board;
		class SavedBoard {
			friend class Board;

		public:
			SavedBoard() : state_() {}
			SavedBoard(SavedBoard const&) = delete;
			SavedBoard & operator=(SavedBoard const&) = delete;

		private:
			SavedBoard(state::State const& state) : state_(state) {}

			state::State state_;
		};

		class CopiedBoard;

		// Make sure no hidden information is accessed by a player
		class Board
		{
			friend CopiedBoard;

		public:
			Board(state::State & board, state::PlayerSide side) : board_(board), side_(side) {}

		public:
			state::PlayerIdentifier GetCurrentPlayer() const {
				return board_.GetCurrentPlayerId();
			}

			state::PlayerSide GetViewSide() const { return side_; }

			BoardView CreateView() const {
				if (side_ == state::kPlayerFirst) {
					return BoardView(engine::PlayerStateView<state::kPlayerFirst>(board_));
				}
				else {
					assert(side_ == state::kPlayerSecond);
					return BoardView(engine::PlayerStateView<state::kPlayerSecond>(board_));
				}
			}

			template <class Functor>
			auto ApplyWithPlayerStateView(Functor && functor) const {
				if (side_ == state::kPlayerFirst) {
					return functor(engine::PlayerStateView<state::kPlayerFirst>(board_));
				}
				else {
					assert(side_ == state::kPlayerSecond);
					return functor(engine::PlayerStateView<state::kPlayerSecond>(board_));
				}
			}

			auto GetCurrentPlayerStateView() const {
				if (board_.GetCurrentPlayerId().GetSide() != side_) {
					assert(false);
					throw std::runtime_error("current player does not match.");
				}
				return engine::CurrentPlayerStateView(board_);
			}

		public: // bridge to action analyzer
			engine::Result ApplyAction(state::IRandomGenerator & random, engine::IActionParameterGetter & action_parameters) const
			{
				assert(board_.GetCurrentPlayerId().GetSide() == side_);
				engine::FlowControl::FlowContext flow_context(random, action_parameters);
				engine::FlowControl::FlowController flow_controller(board_, flow_context);
				return flow_controller.PerformOperation();
			}

		public:
			void Save(SavedBoard & save) const {
				save.state_ = board_;
			}

			void Restore(SavedBoard const& save) const {
				board_ = save.state_;
			}

		public:
			state::State const& RevealHiddenInformationForSimulation() const {
				// For simulation, it needs state information to estimate who's going to win
				// Hidden information is sometimes a big boost to make a better predict.
				// NOTE: THIS SHOULD ONLY BE USED FOR SIMULATION STATE-VALUE ESTIMATION
				return board_;
			}

		private:
			state::State & board_;
			state::PlayerSide side_;
		};

		class CopiedBoard {
		public:
			explicit CopiedBoard(state::PlayerSide side) : state_(), board_(state_, side) {}

			void FillWithBase(Board const& board) {
				state_.FillWithBase(board.board_);
			}

			Board & GetBoard() { return board_; }

		private:
			state::State state_;
			Board board_;
		};
	}
}