#pragma once

#include "State/Manipulators/Manipulators.h"
#include "State/State.h"
#include "FlowControl/Result.h"
#include "FlowControl/Helpers/Utils.h"
#include "FlowControl/Helpers/ActionParameterWrapper.h"
#include "FlowControl/Dispatchers/Minions.h"
#include "FlowControl/Context/BattleCry.h"
#include "FlowControl/Context/BeforeMinionSummoned.h"

namespace FlowControl
{
	namespace Helpers
	{
		template <class ActionParameterGetter, class RandomGenerator>
		class PlayCard
		{
		public:
			PlayCard(state::State & state, ActionParameterGetter & action_parameters, RandomGenerator & random)
				: state_(state), action_parameters_(action_parameters), random_(random), card_(nullptr)
			{

			}

			Result Go()
			{
				int hand_pos = action_parameters_.GetHandCardPosition();
				card_ref_ = state_.GetCurrentPlayer().hand_.Get(hand_pos);
				card_ = &state_.mgr.Get(card_ref_);

				switch (card_->GetCardType())
				{
				case state::kCardTypeMinion:
					return PlayMinionCard();
				}
				throw std::exception("not implemented");
			}

		private:
			Result PlayMinionCard()
			{
				Result rc = kResultNotDetermined;

				state_.event_mgr.TriggerEvent<state::Events::TriggerTypes::BeforeMinionSummoned>(
					Context::BeforeMinionSummoned(state_, card_ref_, *card_));

				state_.GetCurrentPlayer().resource_.Cost(card_->GetCost());

				int total_minions = (int)state_.GetCurrentPlayer().minions_.Size();
				int put_position = action_parameters_.GetMinionPutLocation(0, total_minions);

				state::Manipulators::StateManipulator(state_).Minion(card_ref_)
					.Zone().ChangeTo<state::kCardZonePlay>(state_.current_player, put_position);

				state_.event_mgr.TriggerEvent<state::Events::TriggerTypes::OnMinionPlay>(*card_);

				FlowControl::Dispatchers::Minions::BattleCry(card_->GetCardId(),
					Context::BattleCry(state_, card_ref_, *card_, [this]() {
						return action_parameters_.GetBattlecryTarget(state_, card_ref_, *card_);
				}));

				state_.event_mgr.TriggerEvent<state::Events::TriggerTypes::AfterMinionPlayed>(*card_);

				state_.event_mgr.TriggerEvent<state::Events::TriggerTypes::AfterMinionSummoned>();

				rc = Utils::CheckWinLoss(state_);
				if (rc != kResultNotDetermined) return rc;

				return kResultNotDetermined;
			}

		private:
			state::State & state_;
			ActionParameterWrapper<ActionParameterGetter> action_parameters_;
			RandomGenerator & random_;

			CardRef card_ref_;
			const state::Cards::Card * card_;
			CardRef battlecry_target_;
		};
	}
}