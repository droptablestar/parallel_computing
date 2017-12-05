d = 0.85
def main():
    to_me = {0 : [1, 2, 3, 4], 2 : [0, 1, 3], 3 : [1, 4], 4 : [0, 3]}
    out = [0.5, 0.333333, 1, 0.333333, 0.5]
    ranks = [0.2, 0.2, 0.2, 0.2, 0.2]
    compute(5, out, ranks, to_me)


def compute(numnodes, out, ranks, to_me):
    psums = [0.0 for i in range(len(ranks))]
    nrm = (1 - d) / numnodes
    for k in to_me:
        print k,':',to_me[k]
        psums[k] = 0
        for i in to_me[k]:
            psums[k] += ranks[i] * out[i]
        print psums[k]
        psums[k] = nrm + d*psums[k]
    print psums
if __name__ == '__main__':
    main()
