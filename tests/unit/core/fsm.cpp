#include <boost/test/unit_test.hpp>

#include <diameter/core/fsm.h>

#include <list>

using namespace diameter::core;

enum TestEvent
{
    EVENT1,
    EVENT2
};

enum TestState
{
    STATE1,
    STATE2
};

class TestContext
{
public:
    void action1([[maybe_unused]] const int i = int{})
    {
    }
    void action2([[maybe_unused]] const int i = int{})
    {
    }
};

BOOST_AUTO_TEST_SUITE(fsm_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    using FSM = typename diameter::core::StateMachine<TestState, TestEvent, TestContext, int>;
    auto test_context = std::make_unique<TestContext>();

    const FSM::transition_table_t table = {
        {{TestState::STATE1, TestEvent::EVENT1}, {TestState::STATE2, &TestContext::action1}}
    };

    FSM test_fsm(test_context.get(), TestState::STATE1, &table);

    BOOST_CHECK_EQUAL(test_fsm.state(), TestState::STATE1);
    test_fsm.process_event(TestEvent::EVENT1, 0);

    BOOST_CHECK_EQUAL(test_fsm.state(), TestState::STATE2);
}

BOOST_AUTO_TEST_SUITE_END()
