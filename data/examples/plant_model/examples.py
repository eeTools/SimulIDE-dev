# ***************************************************************************
#     Copyright (C) 2026 by Stefan Persson                                  *
#                                                                           *
# ***( see copyright.txt file at root folder )*******************************

# This is an example file for python processes
# Copy to your project dir; set class_name to Plant1/Plant2 etc.
# The general form of the process is:
'''
class Plant:
    def __init__(self):
        pass

    def process(self, sim_time_ps: int, inputs: list[float]) -> list[float]:
        return "array or numpy array"
'''

# n inputs and 1 output, return 3 times the value
class Plant1:
    """ One input and one output port. The output is 3 times the input. """
    def __init__(self):
        pass

    def process(self, sim_time_ps: int, inputs: list[float]) -> list[float]:
        return [3*inputs[0]]


class Plant2:
    """ Example with internal variable and n inputs and n outputs. """
    def __init__(self):
        self.counter = 0

    def process(self, sim_time, values):
        """
        sim_time : int (uint64)
        values   : list of float
        returns  : list of float
        """
        self.counter += 1

        # Example logic: scale inputs using time & internal state
        factor = 10.0 + 0.001 * self.counter
        return [v * factor for v in values]
        
        
class Plant3:
    """ Example with raise. """
    def __init__(self):
        self.counter = 0

    def process(self, sim_time, values):
        """
        sim_time : int (uint64)
        values   : list of float
        returns  : list of float
        """
        self.counter += 1
        if any(x < 0 for x in values):
            raise ValueError("At least one of the inputs is < 0")

        # Example logic: scale inputs using time & internal state
        factor = 10.0 + 0.001 * self.counter
        return [v * factor for v in values]


import numpy as np
class Plant4:
    """ Example with numpy and raise."""
    def __init__(self):
        self.gain = np.array([2.0, 0.9, 1.05])

    def process(self, sim_time, values):

        if (len(values) != 3):
            raise ValueError("The length of the input array should be 3.")
        values = np.asarray(values, dtype=float)
        out = values * self.gain
        return out

        
        
        
         

