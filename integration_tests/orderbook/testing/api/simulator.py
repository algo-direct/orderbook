from orderbook.testing.api import utils
import subprocess
import logging
import os
import json

class Simulator:
    def __init__(self, host='127.0.0.1', port=40000):
        self.m_host = host
        self.m_port = port
        args = ['--host', host]
        args += ['--port', str(port)]
        args += ['--passive', 'True']
        args += ['--log-level', logging.getLevelName(logging.getLogger().getEffectiveLevel())]
        cmd = [os.getenv("BUILD_DIR", "") + "/simulator/order_book_simulator.py"] + args
        logging.debug(f"Running command: {cmd}")
        logfile = open("/tmp/simulator.log", "wb")
        self.process = subprocess.Popen(cmd, stdout=logfile, stderr=logfile)
        # import time
        # time.sleep(1)

    def terminate(self):
        self.process.terminate()

    def waitForRunningState(self):
        withRetry = True
        result = utils.httpGet(self.m_host, self.m_port, "/isRunning", withRetry)

 
    def setSnapshot(self, data):
        return utils.httpPost(self.m_host, self.m_port, "/setSnapshot", data)
    
    def processPendingSnapshotRequests(self):
        result = utils.httpGet(self.m_host, self.m_port, "/processPendingSnapshotRequests")
        logging.debug(f"result: {result}")
        return result
    
    def addLevel(self, isBid, price, size):
        data = {
            "isBid": str(isBid),
            "price": str(price),
            "size": str(size)
        }
        result = utils.httpPost(self.m_host, self.m_port, "/addLevel", json.dumps(data))
        logging.debug(f"result: {result}")
        return result
    
    def removeLevel(self, isBid, price):
        data = {
            "isBid": str(isBid),
            "price": str(price)
        }
        result = utils.httpPost(self.m_host, self.m_port, "/removeLevel", json.dumps(data))
        logging.debug(f"result: {result}")
        return result
    
    def sendIncrementalUpdate(self):
        result = utils.httpGet(self.m_host, self.m_port, "/sendIncrementalUpdate")
        logging.debug(f"result: {result}")
        return result
    
    