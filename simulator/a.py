import json

j = json.load(open("sim_data.json"))
o = {
    "open": [],
    "high": [],
    "low": [],
    "close": [],
}
for i in range(len(j['open'])):
    op = j['open'][i]
    if type(op) != type(1.1): continue
    o["open"].append(j['open'][i])
    o["high"].append(j['high'][i])
    o["low"].append(j['low'][i])
    o["close"].append(j['close'][i])

json.dump(o, open("o.json", "w"), indent=4)