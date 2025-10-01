import requests

url = "https://api.kucoin.com/api/v3/market/orderbook/level2?symbol=BTC-USDT"

payload={}
headers = {}

response = requests.request("GET", url, headers=headers, data=payload)

print(response.text)

logging.info(f"len(removedBids): {len(removedBids)} len(updatedBids): {len(updatedBids)} len(removedBids): {len(addedBids)} ")