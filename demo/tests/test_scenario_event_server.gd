extends ReloadedGutTest

## ScenarioEventServer - the dispatcher of a scenario's events: an event is queued to run after its
## delay, or run at once without the queue.


## An action counting its runs, its runs after a failed condition and the activators it ran with
class CountingAction:
    extends ScenarioEventAction

    var runs:int = 0
    var else_runs:int = 0
    var activators:Array[RID] = []

    func _run(_event:RID, activator:RID) -> void:
        runs += 1
        activators.append(activator)

    func _run_else(_event:RID, _activator:RID) -> void:
        else_runs += 1


## A condition that always fails
class FailingCondition:
    extends ScenarioEventCondition

    func _test(_event:RID, _activator:RID) -> bool:
        return false


var _events:Array[RID] = []


func after_each() -> void:
    for event:RID in _events:
        ScenarioEventServer.event_free(event)
    _events.clear()


func _create_event(action:ScenarioEventAction) -> RID:
    var event:RID = ScenarioEventServer.event_create()
    _events.append(event)
    ScenarioEventServer.event_attach_action(event, action)
    return event


func test_a_run_event_runs_at_once_whatever_its_delay() -> void:
    var action:CountingAction = CountingAction.new()
    var event:RID = _create_event(action)
    ScenarioEventServer.event_set_delay(event, 10.0)

    ScenarioEventServer.event_run(event)

    assert_eq(action.runs, 1, "it ran at once")
    assert_false(ScenarioEventServer.event_is_queued(event), "nothing waits in the queue")


func test_every_run_of_an_event_runs_it() -> void:
    var action:CountingAction = CountingAction.new()
    var event:RID = _create_event(action)

    ScenarioEventServer.event_run(event)
    ScenarioEventServer.event_run(event)

    assert_eq(action.runs, 2, "each run ran, none is refused as a second queueing is")


func test_a_run_event_tests_its_condition() -> void:
    var action:CountingAction = CountingAction.new()
    var event:RID = _create_event(action)
    ScenarioEventServer.event_attach_condition(event, FailingCondition.new())

    ScenarioEventServer.event_run(event)

    assert_eq(action.runs, 0)
    assert_eq(action.else_runs, 1, "a failed condition runs the action's else")


func test_a_queued_event_waits_and_refuses_a_second_request() -> void:
    var action:CountingAction = CountingAction.new()
    var event:RID = _create_event(action)

    assert_true(ScenarioEventServer.event_queue(event))
    assert_false(ScenarioEventServer.event_queue(event), "queued once")
    assert_eq(action.runs, 0, "it waits for the queue")
    await step(1)
    assert_eq(action.runs, 1, "the queue ran it once")
