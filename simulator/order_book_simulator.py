#!/usr/bin/python3

import utils
import argparse
import random
import json
import time
import asyncio
from aiohttp import web, WSCloseCode
import aiohttp
import logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)


aiohttp_logger = logging.getLogger("aiohttp")
aiohttp_logger.setLevel(logging.INFO)


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

        self.marketData = json.load(open("simulator/sim_data.json"))["close"]
        self.index = 0  # index in self.marketData
        self.ltp = 0

    def generateFirstRandomSpanshot(self):
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
        logging.info(f"getSpanshot: {self.getSpanshot()}")

    def getSpanshot(self):
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
            self.index = 0
            originalSequence = self.sequence
            self.generateFirstRandomSpanshot()
            self.sequence = self.sequence + originalSequence

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
        logging.info(f"start len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        self.updateNextLTP()
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

        logging.info(f"remov len(self.asks): {
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

        logging.info(f"add 1 len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        logging.info(f"len(removedAsks): {
                     len(removedAsks)} len(removedBids): {len(removedBids)}")

        def removeExtraLevels(alreadyRemoved, levels):
            logging.info(f"len(levels): {len(levels)} , self.maxLevels: {
                         self.maxLevels}")
            if len(levels) <= self.maxLevels:
                return
            count = len(levels) - self.maxLevels
            logging.info(f"count: {count}")
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

        logging.info(f"rm ex len(self.asks): {
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

        logging.info(f"updat len(self.asks): {
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
        logging.info(f"add   len(self.asks): {
                     len(self.asks)} len(self.bids): {len(self.bids)}")
        logging.info(f"len(removedAsks): {len(removedAsks)} len(updatedAsks): {
                     len(updatedAsks)} len(addedAsks): {len(addedAsks)} ")
        logging.info(f"len(removedBids): {len(removedBids)} len(updatedBids): {
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
        # logging.info(f"orderbookIncrement: changedAsks {len(changedAsks)}  changedBids {len(changedBids)}") # {orderbookIncrement}")
        logging.info(f"getSpanshot       : \n self.asks {
                     len(self.asks)}  self.bids {len(self.bids)} \n")
        logging.info(f"getSpanshot       : {self.getSpanshot()}")
        return orderbookIncrement


class OrderBookSimulator:

    def __init__(self):
        self.webSocket = None
        self.fakeOrderBook = FakeOrderBook()
        self.fakeOrderBook.generateFirstRandomSpanshot()
        self.lastIncrementalUpdate = None

    async def sendIncrementalUpdate(self):
        try:
            if None != self.lastIncrementalUpdate and None != self.webSocket:
                logging.debug("sending data on websocket")
                await self.webSocket.send_json(self.lastIncrementalUpdate)
        except Exception as ex:
            logging.exception(f"Exception: {ex}")
        finally:
            pass

    async def webSocketSenderLoop(self):
        while True:
            self.lastIncrementalUpdate = self.fakeOrderBook.updateOrderBookUsingNextLTP()
            await self.sendIncrementalUpdate()
            await asyncio.sleep(0.4)

    async def snapshotHandler(self, request):
        await asyncio.sleep(10)
        return web.json_response(self.fakeOrderBook.getSpanshot())

    async def generateIncrementHandler(self, request):
        return web.json_response(self.fakeOrderBook.updateOrderBookUsingNextLTP())

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
                        if not sentIncrementalUpdate:
                            await self.sendIncrementalUpdate()
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
            web.get('/snapshot',   self.snapshotHandler),
            web.get('/generateIncrement',   self.generateIncrementHandler),
            web.get('/ws', self.websocketHandler),
        ])
        return web.AppRunner(app)

    async def startServer(self, host, port):
        runner = self.createRunner()
        await runner.setup()
        site = web.TCPSite(runner, host, port)
        await asyncio.gather(
            self.webSocketSenderLoop(),
            site.start())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OrderBookSimulator")
    parser.add_argument("--host", type=str, default="127.0.0.1",
                        help="webserver listening host")
    parser.add_argument("--port", type=int, default=40000,
                        help="webserver listening port")
    args = parser.parse_args()

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    orderBookSimulator = OrderBookSimulator()
    loop.run_until_complete(
        orderBookSimulator.startServer(args.host, args.port))
    loop.run_forever()
