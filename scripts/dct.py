from math import cos, pi

phi = 2 * pi / 32
cs = [cos(i * phi) for i in range(8)]

def go(xs):
    # t1: pre mult w^k
    t1rs = [0 for _ in range(8)]
    t1is = [0 for _ in range(8)]

    t1rs[0] = 2 * xs[0]
    t1is[0] = 0
    t1rs[1] = cs[1] * xs[1]
    t1is[1] = cs[7] * xs[1]
    t1rs[2] = cs[2] * xs[2]
    t1is[2] = cs[6] * xs[2]
    t1rs[3] = cs[3] * xs[3]
    t1is[3] = cs[5] * xs[3]
    t1rs[4] = cs[4] * xs[4]
    t1is[4] = cs[4] * xs[4]
    t1rs[5] = cs[5] * xs[5]
    t1is[5] = cs[3] * xs[5]
    t1rs[6] = cs[6] * xs[6]
    t1is[6] = cs[2] * xs[6]
    t1rs[7] = cs[7] * xs[7]
    t1is[7] = cs[1] * xs[7]

    t1crs = [t1rs[i] for i in range(8)]
    t1cis = [-t1is[i] for i in range(8)]

    t2rs = [0 for _ in range(8)]
    t2is = [0 for _ in range(8)]

    t2rs[0] = t1rs[0] + 2 * t1rs[4]
    t2is[0] = 0
    t2rs[4] = t1rs[0] - 2 * t1is[4]
    t2is[4] = 0

    t2rs[2] = 2 * t1rs[2] + 2 * t1rs[6]
    t2is[2] = 0
    t2rs[6] = t1rs[2] - t1is[6] - t1crs[6] + t1cis[2]
    t2is[6] = t1is[2] + t1rs[6] - t1cis[6] - t1crs[2]
    t2rs[6], t2is[6] = cs[4] * t2rs[6] - cs[4] * t2is[6], cs[4] * t2rs[6] + cs[4] * t2is[6]
    # print(t2rs[6], t2is[6])

    t2rs[1] = t1rs[1] + t1rs[5] + t1crs[7] + t1crs[3]
    t2is[1] = t1is[1] + t1is[5] + t1cis[7] + t1cis[3]
    t2rs[5] = t1rs[1] - t1is[5] - t1crs[7] + t1cis[3]
    t2is[5] = t1is[1] + t1rs[5] - t1cis[7] - t1crs[3]
    t2rs[5], t2is[5] = cs[2] * t2rs[5] - cs[6] * t2is[5], cs[6] * t2rs[5] + cs[2] * t2is[5]
    # print(t2rs[5], t2is[5])

    t2rs[3] = t1rs[3] + t1rs[7] + t1crs[5] + t1crs[1]
    t2is[3] = t1is[3] + t1is[7] + t1cis[5] + t1cis[1]
    t2rs[7] = t1rs[3] - t1is[7] - t1crs[5] + t1cis[1]
    t2is[7] = t1is[3] + t1rs[7] - t1cis[5] - t1crs[1]
    t2rs[7], t2is[7] = cs[6] * t2rs[7] - cs[2] * t2is[7], cs[2] * t2rs[7] + cs[6] * t2is[7]
    # print(t2rs[7], t2is[7])

    outs = [0 for _ in range(8)]
    outs[0] = t2rs[0] + t2rs[1] + t2rs[2] + t2rs[3]
    outs[1] = t2rs[4] + t2rs[5] + t2rs[6] + t2rs[7]
    outs[4] = t2rs[0] - t2is[1] - t2rs[2] + t2is[3]
    outs[5] = t2rs[4] - t2is[5] - t2rs[6] + t2is[7]
    outs[7] = t2rs[0] - t2rs[1] + t2rs[2] - t2rs[3]
    outs[6] = t2rs[4] - t2rs[5] + t2rs[6] - t2rs[7]
    outs[3] = t2rs[0] + t2is[1] - t2rs[2] - t2is[3]
    outs[2] = t2rs[4] + t2is[5] - t2rs[6] - t2is[7]
    return outs

def go2(xs):
    # t1: pre mult w^k
    t1rs = [0 for _ in range(8)]
    t1is = [0 for _ in range(8)]

    t2rs = [0 for _ in range(8)]
    t2is = [0 for _ in range(8)]

    t2rs[0] = 2 * xs[0] + 2 * cs[4] * xs[4]
    t2rs[4] = 2 * xs[0] - 2 * cs[4] * xs[4]
    t2rs[2] = 2 * cs[2] * xs[2] + 2 * cs[6] * xs[6]
    t2rs[6] = 2 * cs[6] * xs[2] - 2 * cs[2] * xs[6]
    # print(t2rs[6], t2is[6])

    t1rs[1] = cs[1] * xs[1]
    t1is[1] = cs[7] * xs[1]
    t1rs[7] = cs[7] * xs[7]
    t1is[7] = cs[1] * xs[7]

    t1rs[3] = cs[3] * xs[3]
    t1is[3] = cs[5] * xs[3]
    t1rs[5] = cs[5] * xs[5]
    t1is[5] = cs[3] * xs[5]

    # t1crs = [t1rs[i] for i in range(8)]
    # t1cis = [-t1is[i] for i in range(8)]

    t2rs[1] = t1rs[1] + t1rs[5] + t1rs[7] + t1rs[3]
    t2is[1] = t1is[1] + t1is[5] - t1is[7] - t1is[3]
    t2rs[5] = t1rs[1] - t1is[5] - t1rs[7] - t1is[3]
    t2is[5] = t1is[1] + t1rs[5] + t1is[7] - t1rs[3]
    t2rs[5], t2is[5] = cs[2] * t2rs[5] - cs[6] * t2is[5], cs[6] * t2rs[5] + cs[2] * t2is[5]
    # print(t2rs[5], t2is[5])

    outs = [0 for _ in range(8)]
    outs[0] = t2rs[0] + 2 * t2rs[1] + t2rs[2]
    outs[1] = t2rs[4] + 2 * t2rs[5] + t2rs[6]
    outs[4] = t2rs[0] - 2 * t2is[1] - t2rs[2]
    outs[5] = t2rs[4] - 2 * t2is[5] - t2rs[6]
    outs[7] = t2rs[0] - 2 * t2rs[1] + t2rs[2]
    outs[6] = t2rs[4] - 2 * t2rs[5] + t2rs[6]
    outs[3] = t2rs[0] + 2 * t2is[1] - t2rs[2]
    outs[2] = t2rs[4] + 2 * t2is[5] - t2rs[6]
    return outs

for t in range(8):
    xs = [1 if i == t else 0 for i in range(8)]
    ref = [1024 * cos((2 * i + 1) * t * phi) / 2 for i in range(8)]
    if t == 0:
        ref = [a * cs[4] for a in ref]
    print(' '.join(f'{ref[i] : .0f}'.rjust(6) for i in range(8)))
