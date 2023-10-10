from font16 import font

a = 3

def qq():
    global a
    a = 5

qq()
print(a)