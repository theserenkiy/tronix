frames = []
for i in range(16):
    data = []
    for j in range(16):
        data += ([0,64,0] if i==j else [0,0,0])
    frames.append((data))

print(frames)