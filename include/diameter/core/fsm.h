#ifndef DIAMETER_CORE_FSM_H
#define DIAMETER_CORE_FSM_H

#include <map>

namespace diameter::core {

template <typename StateT,
          typename EventT,
          typename ContextT,
          typename UserDataT>
class StateMachine
{
public:
    using transition_table_key_t = std::pair<StateT, EventT>;
    using action_t = void (ContextT::*)(UserDataT);
    using transition_table_value_t = std::pair<StateT, action_t>;

    using transition_table_t = std::map<transition_table_key_t, transition_table_value_t>;
    using action_table_t = std::map<StateT, action_t>;

    explicit StateMachine(ContextT* context, const StateT& initial_state,
        const transition_table_t* transition_table)
        : m_context(context),
          m_current_state(initial_state),
          m_transition_table(transition_table),
          m_enter_table(nullptr),
          m_exit_table(nullptr)
    {
    }

    StateMachine(StateMachine const&) = delete;
    StateMachine& operator= (StateMachine const&) = delete;
    StateMachine(StateMachine&&) = default;
    StateMachine& operator= (StateMachine&&) = default;

    bool process_event(const EventT& event, UserDataT&& user_data)
    {
        auto it_transit = m_transition_table->find({m_current_state, event});
        if (it_transit == m_transition_table->end()) {
            return false;
        }
        auto old_state = m_current_state;
        auto new_state = it_transit->second.first;

        if ((old_state != new_state) && (m_exit_table != nullptr)) {
            auto it_exit = m_exit_table->find(m_current_state);
            if (it_exit != m_exit_table->end()) {
                const auto& exit_action = it_exit->second;
                if (exit_action) {
                    (*m_context.*exit_action)(std::forward<UserDataT>(user_data));
                }
            }
        }

        const auto& transition_action = it_transit->second.second;
        if (transition_action) {
            (*m_context.*transition_action)(std::forward<UserDataT>(user_data));
        }

        // Set new state and do enter action
        m_current_state = new_state;
        if ((old_state != new_state) && (m_enter_table != nullptr)) {
            auto it_enter = m_enter_table->find(new_state);
            if (it_enter != m_enter_table->end()) {
                const auto& enter_action = it_enter->second;
                if (enter_action) {
                    (*m_context.*enter_action)(std::forward<UserDataT>(user_data));
                }
            }
        }
        return true;
    }

    bool process_event(const EventT& event)
    {
        return process_event(event, nullptr);
    }

    const StateT& state()
    {
        return m_current_state;
    }

private:
    ContextT* m_context;
    StateT m_current_state;
    const transition_table_t* m_transition_table;
    const action_table_t* m_enter_table;
    const action_table_t* m_exit_table;
};

} // namespace diameter::core

#endif
