def hue2color(v):
    r = 120-abs(v-120)
    g = 120-abs(v-240)
    b = 120-abs(v-360) if v > 120 else 120-v;
    r,g,b = [0 if v < 0 else v*4 for v in [r,g,b]]


hue2color(30)