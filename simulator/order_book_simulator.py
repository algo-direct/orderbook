#!/usr/bin/python3

import aiohttp
from aiohttp import web, WSCloseCode
import asyncio
import time
import json
import random

def getMillisecondsSinceEpoch():
    return int(round(time.time() * 1000))



class FakeOrderBook:    
    def __init__(self):
        self.index = 0
        self.marketData = json.load(open("sim_data.json"))["close"]
        self.spreadRange = [0.002, 200]
        self.sequence = 0
        self.bids = []
        self.asks = []
        self.maxLevels = 100
        self.snapshot = {
            "time": str(getMillisecondsSinceEpoch()),
            "sequence": str(self.sequence),
            "asks":[
                ["3988.62","8"],
                ["3988.61","32"],
                ["3988.60","47"],
                ["3988.59","3"],
            ],
            "bids":[
                ["3988.51","56"],
                ["3988.50","15"],
                ["3988.49","100"],
                ["3988.48","10"]
            ]
        }
    
    def generateRandomPriceChanges(self):
        alpha = 0.1
        deltas = []
        for i in range(self.maxLevels):
            paretoRandom = random.paretovariate(alpha)
            scaledValue = ((paretoRandom - 1) / (100 - 1)) * (100 - 1) + 1
            clampedValue = min(max(paretoRandom, 1), 100)
            if clampedValue == 100: continue
            deltas.append(clampedValue)
        return deltas

    def updateOrderBook(self):
        ltp = self.marketData[self.index]

        topOftheBook = 

        self.index += 1
        if self.index >= len(self.marketData):
            self.index = 0
    

class OrderBookSimulator:

    def __init__(self):
        self.ws = None
        self.sequence = 16
        self.snapshot = {
            "time": str(getMillisecondsSinceEpoch()),
            "sequence": str(self.sequence),
            "asks":[
                ["3988.62","8"],
                ["3988.61","32"],
                ["3988.60","47"],
                ["3988.59","3"],
            ],
            "bids":[
                ["3988.51","56"],
                ["3988.50","15"],
                ["3988.49","100"],
                ["3988.48","10"]
            ]
        }

    # async def snapshot_handler(self, request):

    async def snapshot_handler(self, request):
        return web.json_response(self.snapshot)

    async def websocket_handler(self, request):
        ws = web.WebSocketResponse()
        await ws.prepare(request)

        async for msg in ws:
            if msg.type == aiohttp.WSMsgType.TEXT:
                if msg.data == 'close':
                    await ws.close()
                else:
                    await ws.send_str('some websocket message payload')
            elif msg.type == aiohttp.WSMsgType.ERROR:
                print('ws connection closed with exception %s' % ws.exception())

        return ws

    def create_runner(self):
        app = web.Application()
        app.add_routes([
            web.get('/snapshot',   self.snapshot_handler),
            web.get('/ws', self.websocket_handler),
        ])
        return web.AppRunner(app)


    async def start_server(self, host="127.0.0.1", port=40000):
        runner = self.create_runner()
        await runner.setup()
        site = web.TCPSite(runner, host, port)
        await site.start()


if __name__ == "__main__":
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    orderBookSimulator = OrderBookSimulator()
    loop.run_until_complete(orderBookSimulator.start_server())
    loop.run_forever()