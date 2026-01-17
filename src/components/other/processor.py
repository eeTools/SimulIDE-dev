class Processor:
    def __init__(self):
        self.counter = 0

    def process(self, value):
        self.counter += 1
        return value * 3, self.counter

