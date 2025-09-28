import json

def convertNumbersToStrings(data):
    if isinstance(data, dict):
        return {k: convertNumbersToStrings(v) for k, v in data.items()}
    elif isinstance(data, list):
        return [convertNumbersToStrings(element) for element in data]
    elif isinstance(data, (int, float)):
        return str(data)
    else:
        return data
    
def truncate(number, decimals):
    if not isinstance(number, (float, int)):
        raise TypeError("Input 'number' must be a float or an integer.")
    if not isinstance(decimals, int) or decimals < 0:
        raise ValueError("Input 'decimals' must be a non-negative integer.")
    factor = 10 ** decimals
    return int(number * factor) / factor