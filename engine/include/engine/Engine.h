#pragma once

#include "state/State.h"
#include "engine/FlowControl/FlowController.h"
#include "engine/FlowControl/FlowContext.h"
#include "engine/Result.h"

namespace engine
{
	class Engine
	{
	public:
		void SetStartState(state::State const& state) {
			state_ = state;
		}

		state::State const& GetCurrentState() const { return state_; }

		Result PerformOperation(state::IRandomGenerator & random_cb, engine::IActionParameterGetterWithoutAnalyzer & action_cb) {
			FlowControl::FlowContext flow_context(random_cb, action_cb);
			FlowControl::FlowController flow_controller(state_, flow_context);
			return flow_controller.PerformOperation();
		}

	private:
		state::State state_;
	};
}