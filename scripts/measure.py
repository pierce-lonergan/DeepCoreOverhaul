"""Score captured frames by their pixel histogram.

Judging a look by eye across many variants is unreliable -- the eye adapts between images and
cannot hold a scale. A histogram cannot. What we want from a lit mine frame is:

  - a MEDIAN in the readable midtones (not crushed, not milky)
  - very little pure white (blown highlights destroy surface detail permanently)
  - some genuine black, but not most of the frame (a dark game, not an empty one)
  - real CONTRAST spread, because a frame with the right median and no spread is fog

The weights below say clipping is the worst sin, then emptiness, then a wrong midpoint.
"""
import sys, glob, os
from PIL import Image


def score(path):
    im = Image.open(path).convert('L')
    px = list(im.getdata())
    n = len(px)
    s = sorted(px)
    median = s[n // 2]
    p05, p95 = s[int(n * 0.05)], s[int(n * 0.95)]
    black = 100.0 * sum(1 for v in px if v < 10) / n
    blown = 100.0 * sum(1 for v in px if v > 250) / n
    spread = p95 - p05
    # lower is better
    penalty = (abs(median - 95) * 1.0) + (blown * 6.0) + (max(0.0, black - 22.0) * 2.0) \
              + (max(0, 150 - spread) * 0.35)
    return dict(name=os.path.basename(path), median=median, black=black, blown=blown,
                spread=spread, penalty=penalty)


if __name__ == '__main__':
    pats = sys.argv[1:] or ['*.png']
    rows = []
    for pat in pats:
        for f in glob.glob(pat):
            try:
                rows.append(score(f))
            except Exception as e:
                print('ERR', f, e)
    rows.sort(key=lambda r: r['penalty'])
    print('%-26s %7s %8s %8s %8s %9s' % ('variant', 'median', 'black%', 'blown%', 'spread', 'penalty'))
    for r in rows:
        print('%-26s %7d %7.1f%% %7.1f%% %8d %9.1f'
              % (r['name'], r['median'], r['black'], r['blown'], r['spread'], r['penalty']))
