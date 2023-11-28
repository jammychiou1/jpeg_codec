def entropy_coded(data):
    output = b''
    while True:
        i = data.find(b'\xff')
        if i == -1:
            # print(data)
            output += data
        # print(data[:i])
        output += data[:i]
        data = data[i:]
        if data[:2] == b'\xff\x00':
            # print(b'\xff')
            output += b'\xff'
            data = data[2:]
        elif data[:2] in [b'\xff\xd0', b'\xff\xd1', b'\xff\xd2', b'\xff\xd3',
                          b'\xff\xd4', b'\xff\xd5', b'\xff\xd6', b'\xff\xd7']:
            data = data[2:]
        else:
            break
    return output, data

def parse(data):
    while True:
        marker, data = data[:2], data[2:]
        if not marker.startswith(b'\xff'):
            return 'error'
        if marker in [b'\xff\xd8', b'\xff\xd9']:
            yield marker, b''
        elif marker in [b'\xff\xe0', b'\xff\xe1', b'\xff\xe2', b'\xff\xed', b'\xff\xee',
                        b'\xff\xfe', b'\xff\xdb', b'\xff\xdd', b'\xff\xc0', b'\xff\xc4']:
            sz = int.from_bytes(data[:2], 'big')
            payload, data = data[:sz], data[sz:]
            yield marker, payload
        elif marker in [b'\xff\xda']:
            payload, data = entropy_coded(data[2:])
            yield marker, payload
        else:
            return 'error'

import sys
# filenames = ['../imgs/teatime.jpg', '../imgs/gig-sn01.jpg', '../imgs/gig-sn08.jpg', '../imgs/monalisa.jpg', '/home/jammychiou1/workspace/scanned/BG.jpg', '/home/jammychiou1/workspace/scanned/wallpaper_1.jpg']

with open(sys.argv[1], 'rb') as f:
    data = f.read()
print(len(data))
for segment in parse(data):
    if segment[0] == b'\xff\xc4':
        # print(segment[0].hex())
        # print(len(segment[1]))
        # print(segment[1].hex())

        Lh = int.from_bytes(segment[1][:2], 'big')
        print(f'Lh: {Lh}')

        payload = segment[1][2:]
        while len(payload) > 0:
            Tc = payload[0] // 16
            Th = payload[0] % 16
            print(f'Tc: {Tc}')
            print(f'Th: {Th}')
            sz = Pq + 1
            Li_s = [0 for i in range(16)]
            for i in range(16):
                Li_s[i] = payload[1 + i]
                print(f'L{i}: {Li_s[i]}')
            Vi_s = [[] for i in range(16)]
            now = 17
            for i in range(16):
                Vi_s[i] = [payload[now + j : now + j + 1].hex() for j in range(Li_s[i])]
                print(f'V{i}: {Vi_s[i]}')
                now += Li_s[i]
            payload = payload[now:]

    if segment[0] == b'\xff\xdb':
        # print(segment[0].hex())
        # print(len(segment[1]))
        # print(segment[1].hex())

        Lq = int.from_bytes(segment[1][:2], 'big')
        # print(f'Lq: {Lq}')

        payload = segment[1][2:]
        while len(payload) > 0:
            Pq = payload[0] // 16
            Tq = payload[0] % 16
            # print(f'Pq: {Pq}')
            # print(f'Tq: {Tq}')
            sz = Pq + 1
            Qk_s = [0 for k in range(64)]
            for k in range(64):
                Qk_s[k] = int.from_bytes(payload[1 + k * sz : 1 + (k + 1) * sz], 'big')
                # print(f'Q{k}: {Qk_s[k]}')
            payload = payload[1 + 64 * sz:]
