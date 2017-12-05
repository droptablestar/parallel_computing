from sys import argv
from random import randint

def main():
    numprocs = int(argv[1])
    size = numprocs * (int(argv[2]) * (10**3))
    name = str(numprocs) + 'p_' + argv[2] + 'K.txt'
    print name, numprocs, size
    fd = open(name, 'w')

    fsize = 0
    while fsize < size:
        for i in range(numprocs):
            fd.write('%d\t%d\n' % (randint(0,5),randint(0,5)))
        fsize = fd.tell()
    fd.write("EOF")

if __name__ == '__main__':
    main()
