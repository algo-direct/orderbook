import random

def paretovariate_in_range(alpha, min_val, max_val):
    if min_val < 1:
        raise ValueError("min_val must be greater than or equal to 1 for standard Pareto distribution.")
    if max_val <= min_val:
        raise ValueError("max_val must be greater than min_val.")

    # Generate a Pareto variate (always >= 1)
    pareto_val = random.paretovariate(alpha)
    # pareto_val *= 2
    # return pareto_val

    # Scale and shift to the desired range
    # This is a simplified approach and might not perfectly preserve Pareto properties within the bounded range
    scaled_val = ((pareto_val - 1) / (max_val - 1)) * (max_val - min_val) + min_val
    return min(max(scaled_val, min_val), max_val) # Ensure it stays within bounds


o = []
for i in range(100):
    o.append(paretovariate_in_range(0.08, 1, 200))

o.sort()
for oi in o:
    print(oi)