from PIL import Image

im = Image.open("hp41dispc.png")

out = open('hp41disp.bin', 'wb')

for l in range(8):
    print(l)
    y = l*25
    for c in range(16):
        x = c*19
        for j in range(25):
            for i in range(19):
                col = im.getpixel((x+i, y+j))
                rgb = ((col >> 3) << 11) | ((col >> 2) << 5) | (col >> 3)
                o = bytes([rgb & 0xFF, rgb >> 8])
                out.write(o)
for l in range(5):
    print(l)
    y = l*25
    x = 15*19+19    
    for j in range(25):
        for i in range(7):
            col = im.getpixel((x+i, y+j))
            rgb = ((col >> 3) << 11) | ((col >> 2) << 5) | (col >> 3)
            o = bytes([rgb & 0xFF, rgb >> 8])
            out.write(o)

out.close()
