import sys
import logging


def before_all(context):
    try:
        logLevelStr = context.config.userdata.get('LOG_LEVEL')
        if not logLevelStr:
            logLevel = logging.DEBUG
        else:
            logging.info(f"logLevelStr: {logLevelStr}")
            print(f"ss logLevelStr: {logLevelStr}")
            
            print(f"logging.getLevelNamesMapping(): {logging.getLevelNamesMapping()}")
            logLevel = logging.getLevelNamesMapping()[logLevelStr]
            print(f"ss logLevel: {logLevel}")
            logging.info(f"logLevelStr: {logLevelStr}")
        logLevel = logging.getLogger().getEffectiveLevel()
        logFormat = context.config.userdata.get('LOG_FORMAT', '%(asctime)s - %(levelname)s - %(name)s - %(message)s')
        logging.basicConfig(
            level=logLevel,
            format=logFormat,
            handlers=[
                logging.FileHandler("behave_test.log"),
                logging.StreamHandler(sys.stdout)
            ]
        )
        logging.info(f"logLevelStr: {logLevelStr}, logFormat: {logFormat}")
    except:
        logging.exception()


def after_all(context):
    print("Executing after all features...")
    # Global teardown: e.g., close WebDriver, close database connection

def before_scenario(context, scenario):
    logging.info(f"1111111111111 Executing before scenario: {scenario.name}")
    # Scenario-specific setup: e.g., log in a user

def after_scenario(context, scenario):
    logging.info(f"aaaa Executing after scenario: {scenario.name}")
    logging.info(f"nnn Executing after scenario: {scenario.name}")

    # Scenario-specific teardown: e.g., log out a user, clean up test data