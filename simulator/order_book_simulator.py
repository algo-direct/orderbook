#!/usr/bin/python3

import utils
import argparse
import random
import json
import time
import queue
import asyncio
from aiohttp import web, WSCloseCode
import aiohttp
import logging
import os

def getMillisecondsSinceEpoch():
    return int(round(time.time() * 1000))

class FakeOrderBook:
    def __init__(self, priceDecimalPlaces=4, sizeDecimalPlaces=8, spreadRange=[0.01, 0.1], orderSizeRange=[0.0001, 1], maxLevels=100):
        self.priceDecimalPlaces = priceDecimalPlaces
        self.sizeDecimalPlaces = sizeDecimalPlaces
        self.spreadRange = spreadRange  # gap between bbo and ltp, in % of ltp
        self.orderSizeRange = orderSizeRange
        self.maxLevels = maxLevels
        self.sequence = 0
        self.bids = []
        self.asks = []
        self.timestamp = getMillisecondsSinceEpoch()
        logging.debug(f"os.getcwd(): {os.getcwd()}")
        simeDataFile = "simulator/sim_data.json"
        if not os.path.isfile(simeDataFile):
            simeDataFile = "../simulator/sim_data.json"
        self.marketData = json.load(open(simeDataFile))["close"]


        self.index = 0  # index in self.marketData
        self.ltp = 0

    def setSnapshot(self, data):
        logging.debug(f"data: {data}")
        self.sequence = int(data.get("sequence", 1))
        self.timestamp = int(data.get("timestamp", getMillisecondsSinceEpoch()))
        self.bids = [[float(priceAndSize[0]), float(priceAndSize[1])] for priceAndSize in data["bids"]]
        self.asks = [[float(priceAndSize[0]), float(priceAndSize[1])] for priceAndSize in data["asks"]]
        self.resetIncrementalData()

    def resetIncrementalData(self):
        self.updatedBids = []
        self.updatedAsks = []
        self.sequenceStart = self.sequence

    def getIncrementalUpdateJson(self):
        update = {
            "topic": "/market/level2:BTC-USDT",
            "type": "message",
            "subject": "trade.l2update",
            "data": {
                    "changes": {
                        "asks": self.updatedBids,
                        "bids": self.updatedAsks
                    },
                "sequenceEnd": self.sequence,
                "sequenceStart": self.sequenceStart,
                "symbol": "BTC-USDT",
                "time": self.timestamp
            }
        }
        self.resetIncrementalData()
        return update

    def addLevel(self, data):
        logging.debug(f"data: {data}")
        isBid = data["isBid"].lower() == "true"
        price = float(data["price"])
        size = int(data["size"])
        levels = self.bids if isBid else self.asks
        updatedLevels = self.updatedBids if isBid else self.updatedAsks
        tmpLevels = []
        tmpLevels.extend(levels)
        logging.debug(f"before levels: {levels}")
        levels.clear()
        added = False
        for priceAndSize in tmpLevels:
            if added:
                levels.append(priceAndSize)
                continue
            if price == priceAndSize[0]:
                levels.append([price, size])
                self.sequence += 1
                updatedLevels.append([str(price), str(size), str(self.sequence)])
                added = True
            elif (isBid and price > priceAndSize[0]) or (not isBid and price < priceAndSize[0]):
                levels.append([price, size])
                levels.append(priceAndSize)
                self.sequence += 1
                updatedLevels.append([str(price), str(size), str(self.sequence)])
                added = True
            else:
                levels.append(priceAndSize)
        if not added:
            levels.append([price, size])
            self.sequence += 1
            updatedLevels.append([str(price), str(size), str(self.sequence)])
            added = True
        self.timestamp = data.get("timestamp", getMillisecondsSinceEpoch())
        logging.debug(f"after levels: {levels}")
        return added

    def removeLevel(self, data):
        logging.debug(f"data: {data}")
        isBid = data["isBid"].lower() == "true"
        price = float(data["price"])
        levels = self.bids if isBid else self.asks
        updatedLevels = self.updatedBids if isBid else self.updatedAsks
        tmpLevels = []
        tmpLevels.extend(levels)
        logging.debug(f"before levels: {levels}")
        levels.clear()
        removed = False
        for priceAndSize in tmpLevels:
            if removed:
                levels.append(priceAndSize)
                continue
            if price == priceAndSize[0]:
                self.sequence += 1
                updatedLevels.append([str(price), "0", str(self.sequence)])
                self.timestamp = data.get("timestamp", getMillisecondsSinceEpoch())
                removed = True
            else:
                levels.append(priceAndSize)
        logging.debug(f"after levels: {levels}")
        return removed
    
    def generateFirstRandomSnapshot(self):
        self.updateNextLTP()
        self.sequence = 0
        bestBid = self.ltp - \
            random.uniform(self.spreadRange[0], self.spreadRange[0])
        bestAsk = self.ltp + \
            random.uniform(self.spreadRange[0], self.spreadRange[0])
        bids = [bestBid]
        bids.extend(
            [bestBid - level for level in self.generateRandomPriceLevels(numLevels=self.maxLevels)])
        asks = [bestAsk]
        asks.extend(
            [bestAsk + level for level in self.generateRandomPriceLevels(numLevels=self.maxLevels)])
        while len(bids) or len(asks):
            bidOrAsk = random.choice(["bid", "ask"])
            levels = None
            if bidOrAsk == "bid":
                if not len(bids):
                    continue
                nextPriceLevel = bids.pop(0)
                levels = self.bids
            else:
                if not len(asks):
                    continue
                nextPriceLevel = asks.pop(0)
                levels = self.asks
            self.sequence += 1
            nextPriceLevel = utils.truncate(
                nextPriceLevel, self.priceDecimalPlaces)
            nextSize = utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1]), self.sizeDecimalPlaces)
            levels.append([nextPriceLevel, nextSize])

        self.timestamp = getMillisecondsSinceEpoch()
        logging.debug(f"snapshot: {self.getSnapshot()}")

    def getSnapshot(self):
        return {
            "code": "200000",
            "data": {
                "time": self.timestamp,
                "sequence": str(self.sequence),
                "bids": [[str(price), str(size)] for price, size in self.bids],
                "asks": [[str(price), str(size)] for price, size in self.asks]
            }
        }

    def updateNextLTP(self):
        self.ltp = self.marketData[self.index]
        self.index += 1
        if self.index >= len(self.marketData):
            return False
        return True

    def generateRandomPriceLevels(self, numLevels, reverseSort=False):
        alpha = 0.1
        deltas = []
        if numLevels < 1:
            return deltas
        for _ in range(numLevels):
            paretoRandom = random.paretovariate(alpha)
            scaledValue = ((paretoRandom - 1) / (100 - 1)) * (100 - 1) + 1
            clampedValue = min(max(paretoRandom, 1), 100)
            if clampedValue == 100:
                continue
            deltas.append(clampedValue)
        deltas.sort(reverse=reverseSort)
        return deltas

    def updateOrderBookUsingNextLTP(self):
        logging.debug(f"start len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        sequenceStart = self.sequence + 1

        nextBestBid = utils.truncate(
            self.ltp -
            random.uniform(self.spreadRange[0], self.spreadRange[0]),
            self.priceDecimalPlaces
        )
        nextBestAsk = utils.truncate(
            self.ltp +
            random.uniform(self.spreadRange[0], self.spreadRange[0]),
            self.priceDecimalPlaces
        )

        lastBestBid = self.bids[0][0]
        lastBestAsk = self.asks[0][0]

        removedBids = []
        removedAsks = []

        # ------------ Remove bids or asks if needed ------------
        if lastBestBid > nextBestBid:
            # need to removed some Bid levels
            for bid, size in self.bids:
                if bid >= nextBestBid:
                    self.sequence += 1
                    size = 0
                    removedBids.append(
                        [str(bid), str(size), str(self.sequence)])
                else:
                    break
            self.bids = self.bids[len(removedBids):]  # remove from the top
        elif lastBestAsk < nextBestAsk:
            # need to removed some Ask levels
            for ask, size in self.asks:
                if ask <= nextBestAsk:
                    self.sequence += 1
                    size = 0
                    removedAsks.append(
                        [str(ask), str(size), str(self.sequence)])
            self.asks = self.asks[len(removedAsks):]  # remove from the top

        logging.debug(f"remove len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        # ------------ Add best bids and best asks levels ------------
        self.bids = [
            [nextBestBid, utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1]), self.sizeDecimalPlaces)
             ]
        ] + self.bids

        self.asks = [
            [nextBestAsk, utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1]), self.sizeDecimalPlaces)
             ]
        ] + self.asks

        logging.debug(f"add 1 len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        logging.debug(f"len(removedAsks): {
                     len(removedAsks)} len(removedBids): {len(removedBids)}")

        def removeExtraLevels(alreadyRemoved, levels):
            logging.debug(f"len(levels): {len(levels)} , self.maxLevels: {
                         self.maxLevels}")
            if len(levels) <= self.maxLevels:
                return
            count = len(levels) - self.maxLevels
            logging.debug(f"count: {count}")
            if len(levels) < 1 and count < 1:
                return
            for i in range(count):
                idx = len(levels) - 1
                priceToRemoved = levels[idx][0]
                levelAlreadyRemoved = False
                for alreadyRemovedLevel in alreadyRemoved:
                    if alreadyRemovedLevel[0] != priceToRemoved:
                        levelAlreadyRemoved = True
                        break
                    if idx == 0:
                        break
                    idx -= 1
                if not levelAlreadyRemoved:
                    del levels[idx]
                    self.sequence += 1
                    size = 0
                    alreadyRemoved.append(
                        [str(priceToRemoved), str(size), str(self.sequence)])

        removeExtraLevels(removedBids, self.bids)
        removeExtraLevels(removedAsks, self.asks)

        logging.debug(f"rm ex len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")

        # ------------ Update order size of random levels of bids and asks ------------

        updatedBids = []
        updatedAsks = []

        updateSequenceStart = self.sequence + 1

        def updateRandomLevels(levels, updates):
            levelsToUpdate = set(
                [int(index) for index in self.generateRandomPriceLevels(numLevels=self.maxLevels)])
            for index in levelsToUpdate:
                if index < 1:
                    continue
                if index >= len(levels):
                    break
                newSize = utils.truncate(
                    random.uniform(
                        self.orderSizeRange[0], self.orderSizeRange[1]),
                    self.sizeDecimalPlaces
                )
                levels[index][1] = newSize
                self.sequence += 1
                updates.append([str(levels[index][0]), str(newSize)])

        updateRandomLevels(self.bids, updatedBids)
        updateRandomLevels(self.asks, updatedAsks)

        logging.debug(f"update len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        sequenceEnd = self.sequence
        randomSequences = list(range(updateSequenceStart, sequenceEnd + 1))
        random.shuffle(randomSequences)
        assert (len(randomSequences) == (len(updatedBids) + len(updatedAsks)))
        for update in updatedBids:
            update.append(str(randomSequences.pop(0)))
        for update in updatedAsks:
            update.append(str(randomSequences.pop(0)))

        # add more levels if still can
        worstBid = self.bids[-1][0]
        worstAsk = self.asks[-1][0]

        addedBids = []
        addedAsks = []
        bids = [worstBid - (4 * level)
                for level in self.generateRandomPriceLevels(numLevels=self.maxLevels)]
        asks = [worstAsk + (4 * level)
                for level in self.generateRandomPriceLevels(numLevels=self.maxLevels)]
        while len(bids) or len(asks):
            bidOrAsk = random.choice(["bid", "ask"])
            levels = None
            if bidOrAsk == "bid":
                if not len(bids):
                    continue
                nextPriceLevel = bids.pop(0)
                levels = self.bids
            else:
                if not len(asks):
                    continue
                nextPriceLevel = asks.pop(0)
                levels = self.asks
            if len(levels) >= self.maxLevels:
                continue
            self.sequence += 1
            nextPriceLevel = utils.truncate(
                nextPriceLevel, self.priceDecimalPlaces)
            nextSize = utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1]), self.sizeDecimalPlaces)
            levels.append([nextPriceLevel, nextSize])
            if bidOrAsk == "bid":
                addedBids.append(
                    [str(nextPriceLevel), str(nextSize), str(self.sequence)])
            else:
                addedAsks.append(
                    [str(nextPriceLevel), str(nextSize), str(self.sequence)])

        sequenceEnd = self.sequence
        logging.debug(f"add   len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        logging.debug(f"len(removedAsks): {len(removedAsks)} len(updatedAsks): {
                     len(updatedAsks)} len(addedAsks): {len(addedAsks)} ")
        logging.debug(f"len(removedBids): {len(removedBids)} len(updatedBids): {
                     len(updatedBids)} len(removedBids): {len(addedBids)} ")
        changedAsks = removedAsks + updatedAsks + addedAsks
        changedBids = removedBids + updatedBids + addedBids
        self.timestamp = getMillisecondsSinceEpoch()
        orderbookIncrement = {
            "topic": "/market/level2:BTC-USDT",
            "type": "message",
            "subject": "trade.l2update",
            "data": {
                    "changes": {
                        "asks": changedAsks,
                        "bids": changedBids
                    },
                "sequenceEnd": sequenceEnd,
                "sequenceStart": sequenceStart,
                "symbol": "BTC-USDT",
                "time": self.timestamp
            }
        }
        logging.debug(f"orderbookIncrement: changedAsks {len(changedAsks)}  changedBids {len(changedBids)}") # {orderbookIncrement}")
        logging.debug(f"asks {len(self.asks)}  bids {len(self.bids)}")
        return orderbookIncrement


class OrderBookSimulator:

    def __init__(self, loop):
        self.loop = loop
        self.webSocket = None
        self.pendingSnapshotRequestSignals = queue.Queue()

    async def renewFakeOrderBook(self):
        await self.disconnectWsImpl()
        self.webSocket = None
        self.fakeOrderBook = FakeOrderBook()
        if not self.passive:
            self.fakeOrderBook.generateFirstRandomSnapshot()
        self.lastIncrementalUpdates = []
        logging.info("renewed FakeOrderBook")

    async def sendIncrementalUpdateOnWebSocket(self):
        try:
            if self.lastIncrementalUpdates and None != self.webSocket:
                logging.debug("sending data on websocket")
                for lastIncrementalUpdate in self.lastIncrementalUpdates:
                    await self.webSocket.send_json(lastIncrementalUpdate)
                self.lastIncrementalUpdates.clear()
                return True
        except Exception as ex:
            logging.exception(f"Exception: {ex}")
        finally:
            return False
    
    async def sendIncrementalUpdate(self, request):
        if self.fakeOrderBook:
            self.lastIncrementalUpdates.append(self.fakeOrderBook.getIncrementalUpdateJson())
        result = await self.sendIncrementalUpdateOnWebSocket()
        return web.json_response({
            "result" : result
        })

    async def disconnectWsImpl(self):
        try:
            if None != self.webSocket:
                webSocket = self.webSocket
                self.webSocket = None
                logging.debug("disconnecting websocket")
                await webSocket.close()
                logging.debug("disconnected websocket")
                return True
        except Exception as ex:
            logging.exception(f"Exception: {ex}")
        return False
    
    async def disconnectWs(self, request):
        result = await self.disconnectWsImpl()
        return web.json_response({
            "wasConnected" : result
        })

    async def webSocketCount(self, request):
        result = "0"
        if None != self.webSocket:
            result = "1"
        return web.json_response({
            "count" : result
        })

    async def clearPendingIncrementalUpdates(self, request):
        self.fakeOrderBook.resetIncrementalData()
        return web.json_response({
            "sequence": str(self.fakeOrderBook.sequence)
        })
    
    async def processPendingSnapshotRequests(self, request):
        try:
            pendingRequestsCount = 0
            while not self.pendingSnapshotRequestSignals.empty():
                signal = self.pendingSnapshotRequestSignals.get()
                signal.set()
                pendingRequestsCount += 1
            return web.json_response({
                "pendingRequestsCount" : pendingRequestsCount
            })
        except Exception as ex:
            logging.exception(f"Exception: {ex}")
        finally:
            pass

    async def webSocketSenderLoop(self):
        while True:
            if not self.passive and self.fakeOrderBook:
                self.lastIncrementalUpdates.append(await self.updateOrderBook())
                await self.sendIncrementalUpdateOnWebSocket()
            await asyncio.sleep(0.4)

    async def isRunning(self, request):
        return web.json_response({
            "result": "running"
        })
    
    async def snapshotHandler(self, request):
        if self.passive:
            signal = asyncio.Event()
            self.pendingSnapshotRequestSignals.put(signal)
            snapshot = self.fakeOrderBook.getSnapshot()
            await signal.wait()
            return web.json_response(snapshot)
        else:
            await asyncio.sleep(1)
            return web.json_response(self.fakeOrderBook.getSnapshot())

    async def setSnapshot(self, request):
        if not self.passive:
            # only passive mode allowed to set snapshot
            return web.json_response({
                "result": "not allowed"
            })
        data = await request.json()
        self.fakeOrderBook = FakeOrderBook()
        self.fakeOrderBook.setSnapshot(data)
        return web.json_response({
            "result": "stored"
        })
    
    async def setIncrementalUpdate(self, request):
        if not self.passive:
            # only passive mode allowed to set snapshot
            return web.json_response({
                "result": False,
                "msg": "not allowed"
            })
        data = await request.json()
        self.fakeOrderBook = FakeOrderBook()
        self.fakeOrderBook.setSnapshot(data)
        return web.json_response({
            "result": True,
            "msg": "stored",
        })
    
    async def addLevel(self, request):
        if not self.passive:
            # only passive mode allowed to Add level
            return web.json_response({
                "result": False,
                "msg": "not allowed"
            })
        data = await request.json()
        return web.json_response({
            "result": self.fakeOrderBook.addLevel(data),
            "msg": "stored",
            "sequence": str(self.fakeOrderBook.sequence)
        })

    async def removeLevel(self, request):
        if not self.passive:
            # only passive mode allowed to Add level
            return web.json_response({
                "result": False,
                "msg": "not allowed"
            })
        data = await request.json()
        return web.json_response({
            "result": self.fakeOrderBook.removeLevel(data),
            "msg": "stored",
            "sequence": str(self.fakeOrderBook.sequence)
        })

    async def updateOrderBook(self):
        if self.fakeOrderBook.updateNextLTP():
            return self.fakeOrderBook.updateOrderBookUsingNextLTP()
        await self.renewFakeOrderBook();
        return self.fakeOrderBook.updateOrderBookUsingNextLTP()

    async def generateIncrementHandler(self, request):
        result = await self.updateOrderBook()
        return web.json_response(result)

    async def websocketHandler(self, request):
        logging.info("webscoket request received..")
        newWebSocekt = web.WebSocketResponse()
        await newWebSocekt.prepare(request)
        if self.webSocket:
            try:
                logging.info(f"closing existing websocket")
                await self.webSocket.close(code=1000, message='Goodbye!')
            finally:
                self.webSocket = None

        self.webSocket = newWebSocekt
        sentIncrementalUpdate = False
        try:
            async for msg in self.webSocket:
                if msg.type == aiohttp.WSMsgType.TEXT:
                    if msg.data == 'close':
                        await self.webSocket.close()
                    else:
                        if not sentIncrementalUpdate and not self.passive:
                            await self.sendIncrementalUpdateOnWebSocket()
                            sentIncrementalUpdate = True
                elif msg.type == aiohttp.WSMsgType.ERROR:
                    logging.error(
                        'websocket connection closed with exception %s' % self.webSocket.exception())
        except Exception as e:
            logging.exception(f"WebSocket handler error: {e}")
        finally:
            logging.info("Disconnecting websocket..")
        self.webSocket = None
        return newWebSocekt

    def createRunner(self):
        app = web.Application()
        app.add_routes([
            web.get('/isRunning',   self.isRunning),
            web.get('/snapshot',   self.snapshotHandler),
            web.post('/setSnapshot',   self.setSnapshot),
            web.post('/addLevel',   self.addLevel),
            web.post('/removeLevel',   self.removeLevel),
            web.get('/generateIncrement',   self.generateIncrementHandler),
            web.get('/ws', self.websocketHandler),
            web.get('/disconnectWs', self.disconnectWs),
            web.get('/webSocketCount', self.webSocketCount),
            web.get('/clearPendingIncrementalUpdates', self.clearPendingIncrementalUpdates),
            web.get('/processPendingSnapshotRequests', self.processPendingSnapshotRequests),
            web.get('/sendIncrementalUpdate', self.sendIncrementalUpdate),
        ])
        return web.AppRunner(app)

    async def startServer(self, host, port, passive):
        self.passive = passive
        await self.renewFakeOrderBook()
        runner = self.createRunner()
        await runner.setup()
        site = web.TCPSite(runner, host, port)
        await asyncio.gather(
            site.start(),
            self.webSocketSenderLoop())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OrderBookSimulator")
    parser.add_argument("--host", type=str, default="0.0.0.0",
                        help="webserver listening host")
    parser.add_argument("--port", type=int, default=40000,
                        help="webserver listening port")
    parser.add_argument("--passive", type=bool, default=False,
                        help="A passive simulator only acts via web api")
    parser.add_argument('--log-level', default='INFO',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help='Set the logging level.')
    args = parser.parse_args()
    logging_level = getattr(logging, args.log_level.upper())
    logging.basicConfig(
        level=logging_level,
        format='%(asctime)s - %(levelname)s - %(filename)s:%(lineno)d - %(message)s'
    )
    aiohttp_logger = logging.getLogger("aiohttp")
    aiohttp_logger.setLevel(logging_level)

    logging.info(f"args: {args}")
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    orderBookSimulator = OrderBookSimulator(loop)
    loop.run_until_complete(
        orderBookSimulator.startServer(args.host, args.port, args.passive))
    loop.run_forever()
