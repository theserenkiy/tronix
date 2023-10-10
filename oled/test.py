

def padRight(s,size,sym=' '):
    return s+(sym*(size-len(s)))

s = str(1234)

print(padRight(s,3)+'!')