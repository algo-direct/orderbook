from behave import step
from orderbook.testing.api  import simulator, orderbook
import logging

SIMULATOR_BASE_PORT_NUMBER = 20000
ORDERBOOK_BASE_PORT_NUMBER = 22000

@step('Order Book Simulator is running')
@step('Run an Order Book Simulator')
def step_impl(context):
    port = SIMULATOR_BASE_PORT_NUMBER
    context.simulators["main"] = simulator.Simulator(port=port)
    context.simulators["main"].waitForRunningState()

@step('Order Book is running')
@step('Run an Order Book')
def step_impl(context):
    simulatorPort = SIMULATOR_BASE_PORT_NUMBER
    orderBookHttpServerPort = ORDERBOOK_BASE_PORT_NUMBER
    context.orderbooks["main"] = orderbook.OrderBook(simulatorPort=simulatorPort, httpServerPort=orderBookHttpServerPort)
    context.orderbooks["main"].waitForRunningState()

@step('Set following snapshot in simulator')
def step_impl(context):
    context.simulators["main"].setSnapshot(context.text)

@step('Add {bidOrAsk} at level {price} and size {size}')
def step_impl(context, bidOrAsk, price, size):
    isBid = bidOrAsk.upper() == "BID"
    context.simulators["main"].addLevel(isBid, price, size)

@step('Remove {bidOrAsk} from level {price}')
def step_impl(context, bidOrAsk, price):
    isBid = bidOrAsk.upper() == "BID"
    context.simulators["main"].removeLevel(isBid, price)

@step('Simulator sends incremental update to Order Book')
def step_impl(context):
    context.simulators["main"].sendIncrementalUpdate()

@step("Simulator disconnect Order Book's websocket connection")
def step_impl(context):
    context.simulators["main"].disconnectWebSocket()

@step('Simulator process any pending snapshot request from Order Book')
def step_impl(context):
    result = context.simulators["main"].processPendingSnapshotRequests()
    logging.debug(f"result: {result}")

@step('Get snapshot from Order Book')
def step_impl(context):
    context.orderbooks["main"].getSnapshot()
    logging.debug(f"snapshot: {context.orderbooks["main"].snapshot}")

@step('Verify Order Book sequence number is {sequenceNumber}')
def step_impl(context, sequenceNumber):
    result = context.orderbooks["main"].waitForSequenceNumber(sequenceNumber)
    actualSequenceNumber = context.orderbooks["main"].snapshot["sequence"]
    assert result, f"Was expecting sequence number {sequenceNumber}, but got {actualSequenceNumber}"

@step('Verify Order Book is connected to simulator via websocket')
def step_impl(context):
    context.simulators["main"].waitForOrderBookToConnectViaWebSocket()

@step('{bidOrAsk} in the snapshot contains level {price} with size {size}')
def step_impl(context, bidOrAsk, price, size):
    isBid = bidOrAsk.upper() == "BIDS"
    context.simulators["main"].removeLevel(isBid, price)
    levels = context.orderbooks["main"].snapshot["bids"] if isBid else context.orderbooks["main"].snapshot["asks"]
    for priceAndSize in levels:
        if float(priceAndSize[0]) == float(price):
            assert float(priceAndSize[1]) == float(size), f"size at {bidOrAsk} level {price} is {priceAndSize[1]}, was expecting {size}"
            return
    assert False, f"{bidOrAsk} level {price} doesn't exist"


@step('{bidOrAsk} in the snapshot does not contains level {price}')
def step_impl(context, bidOrAsk, price):
    isBid = bidOrAsk.upper() == "BIDS"
    context.simulators["main"].removeLevel(isBid, price)
    levels = context.orderbooks["main"].snapshot["bids"] if isBid else context.orderbooks["main"].snapshot["asks"]
    for priceAndSize in levels:
        if float(priceAndSize[0]) == float(price):
            assert False, f"Was not expecting {bidOrAsk} level {price} to exist"