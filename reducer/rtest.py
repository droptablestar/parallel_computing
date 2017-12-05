from commands import getoutput
from sys import argv
from os import listdir

def main():
    times = []
    path = 'tests/'
    files = listdir(path)

    s_files = map(lambda x: (int(x[:x.find('p')]),path+x), files)
    s_files.sort()

    for f in s_files:
        print f
        for i in range(10):
            print i
            times.append(\
                getoutput('mpirun --machinefile machines -np ' + str(f[0]) +\
                              ' reducer ' + f[1]))
        reals = map(float, times)
        # reals = parseStats(times)
        
        avg,var = getStats(reals)
        print avg,var

        with open('results.txt','a') as fd:
            fd.write('%d\t%s\t%.2f\t%.2f\n' % \
                        (f[0],int(f[1][f[1].rfind('/')+1:f[1].find('p')])\
                             ,avg,var))
        times = []
    # print times

def getStats(time):
    time = map(float,time)
    avg = sum(time)/len(time)
    var = sum(map(lambda x: pow(x-avg,2),time))/(len(time)-1)

    return avg,var
                
def parseStats(times):
    reals = []
    print times
    for t in times:
        startM = t.find('real')+len('real')+1
        startS = t.find('m') + 1
        minutes = int(t[startM:startS-1])*60000
        endS = t.find('.',startS)
        s = int(t[startS:endS])*1000
        startMs = endS + 1
        endMs = t.find('s',startMs)
        ms = int(round(float(t[startMs:endMs])))
        t = minutes + s + ms
        reals.append(t)
    # print reals

    return reals

if __name__ == '__main__':
    main()
