import logging
import aiohttp
import asyncio
from functools import partial

def httpGet(host, port, uri, withRetry=False):
    url = f"http://{host}:{port}{uri}"
    fx = partial(_httpGet, url) 
    if withRetry:
        return asyncio.run(callWithRetry(fx, url))
    return asyncio.run(fx())

def httpPost(host, port, uri, payload, withRetry=False):
    url = f"http://{host}:{port}{uri}"
    fx = partial(_httpPost, url, payload) 
    if withRetry:
        return asyncio.run(callWithRetry(fx, url))
    return asyncio.run(fx())

async def callWithRetry(fx, url, retries=50, delay=0.1):
    for attempt in range(retries):
        try:
            return await fx()
        except (aiohttp.ClientConnectorError, asyncio.TimeoutError, aiohttp.ServerDisconnectedError) as e:
            logging.debug(f"Attempt {attempt + 1} failed for {url}: {e}")
            if attempt < retries - 1:
                logging.debug(f"Retrying in {delay} seconds...")
                await asyncio.sleep(delay)
            else:
                logging.error(f"Max retries reached for {url}. Giving up.")
                raise  # Re-raise the exception if all retries fail
            
async def _httpGet(url):
    timeout = aiohttp.ClientTimeout(total=10)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        async with session.get(url) as response:
            logging.debug(f"response.status: {response.status}")
            result = await response.json()
            logging.debug(f"response.json(): {result}")
            return result

async def _httpPost(url, payload):
    timeout = aiohttp.ClientTimeout(total=10)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        async with session.post(url, data=payload) as response:
            logging.debug(f"response.status: {response.status}")
            result = await response.json()
            logging.debug(f"response.json(): {result}")
            return result