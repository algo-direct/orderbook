import subprocess
from orderbook.testing.api import utils
import logging
import os
import time

class OrderBook:
    def __init__(self, simulatorHost='127.0.0.1', simulatorPort=40000, httpServerPort=48080):
        self.m_simulatorHost = simulatorHost
        self.m_simulatorPort = simulatorPort
        self.m_httpServerHost = simulatorHost
        self.m_httpServerPort = httpServerPort

        self.snapshot = None
        args = ['--host', simulatorHost]
        args += ['--port', str(simulatorPort)]
        args += ['--http_server_host', str(self.m_httpServerHost)]
        args += ['--http_server_port', str(httpServerPort)]
        cmd = [os.getenv("BUILD_DIR", "") + "/src/OrderBook"] + args
        logging.debug(f"Running command: {cmd}")
        logfile = open("/tmp/orderbook.log", "wb")
        self.process = subprocess.Popen(cmd, stdout=logfile, stderr=logfile)

    def terminate(self):
        self.process.terminate()
 
    def waitForRunningState(self):
        withRetry = True
        utils.httpGet(self.m_httpServerHost, self.m_httpServerPort, "/snapshot.api", withRetry)

    def waitForSequenceNumber(self, sequenceNumber, retries=50):
        for attempt in range(retries):
            self.snapshot = utils.httpGet(self.m_httpServerHost, self.m_httpServerPort, "/snapshot.api")
            if self.snapshot["sequence"] == str(sequenceNumber):
                return True
            time.sleep(0.1)
        return False

    def getSnapshot(self):
        self.snapshot = utils.httpGet(self.m_httpServerHost, self.m_httpServerPort, "/snapshot.api")
        return self.snapshot