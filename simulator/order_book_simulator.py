#!/usr/bin/python3

import logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

import aiohttp
from aiohttp import web, WSCloseCode
import asyncio
import time
import json
import random
import argparse


import utils

aiohttp_logger = logging.getLogger("aiohttp")
aiohttp_logger.setLevel(logging.INFO)

def getMillisecondsSinceEpoch():
    return int(round(time.time() * 1000))

class FakeOrderBook:    
    def __init__(self, priceDecimalPlaces = 4, sizeDecimalPlaces = 8, spreadRange = [0.01, 0.1], orderSizeRange = [0.0001, 1], maxLevels = 100):
        self.priceDecimalPlaces = priceDecimalPlaces
        self.sizeDecimalPlaces = sizeDecimalPlaces
        self.spreadRange = spreadRange # gap between bbo and ltp, in % of ltp
        self.orderSizeRange = orderSizeRange
        self.maxLevels = maxLevels
        self.sequence = 0
        self.bids = []
        self.asks = []
        self.timestamp = getMillisecondsSinceEpoch()

        self.marketData = json.load(open("simulator/sim_data.json"))["close"]
        self.index = 0 # index in self.marketData 
        self.ltp = 0   
    
    def generateFirstRandomSpanshot(self):
        self.updateNextLTP()
        self.sequence = 0
        bestBid = self.ltp - random.uniform(self.spreadRange[0], self.spreadRange[0])
        bestAsk = self.ltp + random.uniform(self.spreadRange[0], self.spreadRange[0])
        bids = [bestBid]
        bids.extend([ bestBid - level for level in self.generateRandomPriceLevels(numLevels = self.maxLevels, reverseSort=True)])
        asks = [bestAsk]
        asks.extend([ bestAsk + level for level in self.generateRandomPriceLevels(numLevels = self.maxLevels)])
        while len(bids) or len(asks):
            bidOrAsk = random.choice(["bid", "ask"])
            levels = None
            if bidOrAsk == "bid":
                if not len(bids): continue
                nextPriceLevel = bids.pop(0)
                levels = self.bids
            else:
                if not len(asks): continue
                nextPriceLevel = asks.pop(0)
                levels = self.asks
            self.sequence += 1
            nextPriceLevel = utils.truncate(nextPriceLevel, self.priceDecimalPlaces)
            nextSize = utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1])
                    ,self.sizeDecimalPlaces)
            levels.append([nextPriceLevel, nextSize])
        
        self.asks = self.asks[:self.maxLevels]
        self.bids = self.bids[:self.maxLevels]
        self.timestamp = getMillisecondsSinceEpoch()
    
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
    
    def generateRandomPriceLevels(self, numLevels, reverseSort = False):
        alpha = 0.1
        deltas = []
        for _ in range(numLevels):
            paretoRandom = random.paretovariate(alpha)
            scaledValue = ((paretoRandom - 1) / (100 - 1)) * (100 - 1) + 1
            clampedValue = min(max(paretoRandom, 1), 100)
            if clampedValue == 100: continue
            deltas.append(clampedValue)
        deltas.sort(reverse=reverseSort)
        return deltas

    def updateOrderBookUsingNextLTP(self):
        self.updateNextLTP()
        sequenceStart = self.sequence + 1        

        nextBestBid = utils.truncate(
            self.ltp - random.uniform(self.spreadRange[0], self.spreadRange[0]),
            self.priceDecimalPlaces
        )
        nextBestAsk = utils.truncate(
            self.ltp + random.uniform(self.spreadRange[0], self.spreadRange[0]),
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
                    removedBids.append([str(bid), str(size), str(self.sequence)])
                else:
                    break
            self.bids = self.bids[sequenceStart - self.sequence + 1:] # remove from the top
        elif lastBestAsk < nextBestAsk:
            # need to removed some Bid levels
            for ask, size in self.asks:
                if ask > nextBestBid:
                    self.sequence += 1
                    size = 0
                    removedAsks.append([str(ask), str(size), str(self.sequence)])
            self.asks = self.asks[sequenceStart - self.sequence + 1:] # remove from the top

        # ------------ Add best bids and best asks levels ------------
        self.bids = [
            [nextBestBid, utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1])
                    ,self.sizeDecimalPlaces)
            ]
        ] + self.bids

        self.asks = [
            [nextBestAsk, utils.truncate(
                random.uniform(self.orderSizeRange[0], self.orderSizeRange[1])
                    ,self.sizeDecimalPlaces)
            ]
        ] + self.asks

        # ------------ Update order size of random levels of bids and asks ------------

        updatedBids = []
        updatedAsks = []

        updateSequenceStart = self.sequence + 1

        def updateRandomLevels(levels, updates):
            levelsToUpdate = set([int(index) for index in  self.generateRandomPriceLevels(numLevels = self.maxLevels)])
            for index in levelsToUpdate:
                if index < 1: continue
                if index >= len(levels): break
                newSize = utils.truncate(
                    random.uniform(self.orderSizeRange[0], self.orderSizeRange[1]),
                    self.sizeDecimalPlaces
                )
                levels[index][1] = newSize
                self.sequence += 1  
                updates.append([str(levels[index][0]), str(newSize)])

        updateRandomLevels(self.bids, updatedBids)
        updateRandomLevels(self.asks, updatedAsks)
        sequenceEnd = self.sequence
        randomSequences = list(range(updateSequenceStart, sequenceEnd + 1))
        random.shuffle(randomSequences)
        assert(len(randomSequences) == (len(updatedBids) + len(updatedAsks)))
        for update in updatedBids:
            update.append(str(randomSequences.pop(0)))
        for update in updatedAsks:
            update.append(str(randomSequences.pop(0)))

        self.timestamp = getMillisecondsSinceEpoch()
        orderbookIncrement = {
            "topic": "/market/level2:BTC-USDT",
            "type": "message",
            "subject": "trade.l2update",
            "data": {
                    "changes": {
                        "asks": removedAsks + updatedAsks,
                        "bids": removedBids + updatedBids
                    },
                    "sequenceEnd": sequenceEnd,
                    "sequenceStart": sequenceStart,
                    "symbol": "BTC-USDT",
                    "time": self.timestamp
                }
        }
        return orderbookIncrement    

class OrderBookSimulator:

    def __init__(self):
        self.webSocket = None
        self.fakeOrderBook = FakeOrderBook()
        self.fakeOrderBook.generateFirstRandomSpanshot()

    async def webSocketSenderLoop(self):
        while True:
            # logging.debug("sleeping..")
            await asyncio.sleep(2)
            # logging.debug("running..")
            try:
                delta = self.fakeOrderBook.updateOrderBookUsingNextLTP()
                # delta = { "dd": "yy "}
                # logging.debug(f"running, not self.webSocket: {not self.webSocket}")
                # logging.debug(f"running, None == self.webSocket: {None == self.webSocket}")                
                if None != self.webSocket:
                    logging.debug("sending data on websocket")                
                    await self.webSocket.send_json(delta)
            finally:
                pass

    async def snapshotHandler(self, request):
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
        logging.debug(f"1 None == self.webSocket: {None == self.webSocket}")
        logging.debug(f"3 None == newWebSocekt: {None == newWebSocekt}")
        logging.debug(f"4 newWebSocekt: {newWebSocekt}")
        try:
            async for msg in self.webSocket:
                logging.info(f"msg on websocket: {msg}")
                logging.debug(f"2 None == self.webSocket: {None == self.webSocket}")
                if msg.type == aiohttp.WSMsgType.TEXT:
                    if msg.data == 'close':
                        await self.webSocket.close()
                    # else:
                    #     await self.webSocket.send_str('some websocket message payload')
                elif msg.type == aiohttp.WSMsgType.ERROR:
                    logging.error('websocket connection closed with exception %s' % self.webSocket.exception())
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
    parser.add_argument("--host", type=str, default="127.0.0.1", help="webserver listening host")
    parser.add_argument("--port", type=int, default=40000, help="webserver listening port")
    args = parser.parse_args()

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    orderBookSimulator = OrderBookSimulator()
    loop.run_until_complete(orderBookSimulator.startServer(args.host, args.port))
    loop.run_forever()