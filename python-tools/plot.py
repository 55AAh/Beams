import sys
import json
from itertools import cycle

from matplotlib import pyplot as plt


def main():
    if len(sys.argv) == 2:
        file_path = sys.argv[1]
    else:
        file_path = input('File path? > ')

    with open(file_path) as f:
        j = json.load(f)

    vp = j['visual_params']
    sp = j['solver_params']
    up = j['problem']

    el_j = j['solution']
    el_x, el_y = [], []
    for el in el_j:
        el_x.append(el['full']['x'])
        el_y.append(el['full']['y'])
    # plt.scatter(el_x, el_y)

    if 'solution_seg' in j:
        el_j_seg = j['solution_seg']
        for el, color in zip(el_j_seg, cycle(['red', 'green', 'blue'])):
            el_seg_x, el_seg_y = [], []
            for seg in el:
                el_seg_x.append(seg['full']['x'])
                el_seg_y.append(seg['full']['y'])
            plt.plot(el_seg_x, el_seg_y, color=color)

    plt.title(str(up), wrap=True, pad=-20)
    plt.show()


def show_exception_and_pause(*args, **kwargs):
    import traceback
    traceback.print_exception(*args, **kwargs)
    input('Press Enter to exit...')
    sys.exit(-1)


if __name__ == '__main__':
    sys.excepthook = show_exception_and_pause
    main()
