import sys
import logging
import signal
import functools
from orderbook.testing.api import simulator, orderbook

def terminate_all(proc_type, processes):
    if not processes: return
    for name, process in processes.items():
        logging.debug(f"Terminating {proc_type} {name}")
        process.terminate()

def signal_handler(sig, frame, context):
    print(f"Signal {sig} received. Performing cleanup...")
    if context:
        terminate_all("simulator", context.simulators)
        terminate_all("orderbook", context.orderbooks)
    sys.exit(0)

def before_all(context):
    signal.signal(signal.SIGINT, functools.partial(signal_handler, context=context))
    context.simulators = {}
    context.orderbooks = {}
    try:
        logFormat = context.config.userdata.get('LOG_FORMAT', '%(asctime)s - %(levelname)s - %(filename)s:%(lineno)d - %(message)s')
        logging.basicConfig(
            level=context.config.logging_level,
            format=logFormat,
            handlers=[
                logging.FileHandler("behave_test.log"),
                logging.StreamHandler(sys.stdout)
            ]
        )
    except:
        logging.exception()

def after_all(context):
    logging.debug("after all features...")
    terminate_all("simulator", context.simulators)
    terminate_all("orderbook", context.orderbooks)
    context.simulators = {}
    context.orderbooks = {}

def before_scenario(context, scenario):
    logging.debug(f"before scenario: {scenario.name}")
    context.simulators = {}
    context.orderbooks = {}

def after_scenario(context, scenario):
    logging.debug(f"after scenario: {scenario.name}, number of simulators : {len(context.simulators)}, number of order books: {len(context.orderbooks)}")
    terminate_all("simulator", context.simulators)
    terminate_all("orderbook", context.orderbooks)
    context.simulators = {}
    context.orderbooks = {}