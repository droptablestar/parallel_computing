from sys import argv

def main():
    hmap = {}
    with open(argv[1], 'r') as f:
        lines = f.readlines()
    lines = map(lambda x: x.strip('\n').split('\t'), lines)

    for line in lines[:-1]:
        key = int(line[0])
        value = int(line[1])

        if hmap.has_key(key):
            hmap[key] += value
        else:
            hmap[key] = value

    print hmap


if __name__ == '__main__':
    main()
