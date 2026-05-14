#include <diameter/core/controller/watchdog_controller.h>

namespace diameter::core::controller {

const WatchdogController::FsmTransitionTableType WatchdogController::m_fsm_transition_table {
    {{States::INITIAL, Events::RCV_DWA},       {States::INITIAL, &WatchdogController::throwaway}},
    {{States::INITIAL, Events::RCV_MESSAGE},   {States::INITIAL, &WatchdogController::throwaway}},
    {{States::INITIAL, Events::TIMER_EXPIRES}, {States::INITIAL, &WatchdogController::open}},
};

}