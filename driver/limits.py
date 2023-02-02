# From Fast Downward

def round_time_limit(limit):
    """
    Return the time limit rounded down to an integer. If the limit is within 1ms
    of an integer, we round up instead. We have to do this as the system calls
    for setting time limits only support integer-valued limits.
    """
    return int(limit + 0.001)
