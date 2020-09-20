import time

class Zoo:
    
    def __init__(self):
        self.aardvark = 1
        self.baboon = 1
        self.cat = 1
        self.donkey = 1
        self.elephant = 1
        self.fox = 1

    def ant(self): 
        return self.aardvark
    def banana(self):
        return self.baboon
    def tuna(self):
        return self.cat
    def hay(self):
        return self.donkey
    def grass(self):
        return self.elephant
    def mouse(self):
        return self.fox

zoo = Zoo()
sumed = 0
start = time.perf_counter()
while (sumed < 100000):
    sumed = sumed + zoo.ant() + zoo.banana() + zoo.tuna() + zoo.hay() 
    + zoo.grass() + zoo.mouse()

print(time.perf_counter() - start)
print(sumed);
