#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <new>
#include <queue>
#include <algorithm>
#include <utility>
#include <unordered_map>

#include "shortest-path.hpp"

using namespace std;

int stop = 0;

// vector containing a NodeData object for each node in the graph
vector<QNode> qnodes_v;
vector<QNode> qnodes_vs;

// number of threads being used, and number of nodes in the graph
uint numthrds, num_nodes;

struct token_t token;

// lock for printing
pthread_mutex_t pr_lock;

// locks for update vectors
vector<pthread_mutex_t> up_lock;

// source for shortest path
uint source;

// vectors to store updates for personal heaps
vector<vector<QNode> > update_v;

int main(int argc, char **argv) {
    if (argc < 5)
        error("Usage: ./shortest-path <input file> <output file> "
              "<source_id_node> <number of threads>");

    pthread_mutex_init(&pr_lock, NULL);
    cout << "reading file...\n";
    char *file = read_file(argv[1]);
    cout << "parsing input...\n";
    parse_input(file);
    source = atoi(argv[3]);
    set_weights(qnodes_v);
    token = {WHITE, 0, false};
    
    // sequential
    numthrds = 1;
    update_v.resize(numthrds);
    up_lock.resize(numthrds);
    pthread_mutex_init(&(up_lock[0]), NULL);

    double stime=0.0, ptime=0.0;
    struct timeval start, end;

    printf("Computing sequential time...\n");
    gettimeofday(&start, NULL);
    compute_seq();
    gettimeofday(&end, NULL);
    stime = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);

    numthrds = atoi(argv[4]);

    update_v.resize(numthrds);
    up_lock.resize(numthrds);

    for (int i=1; i<numthrds; i++) 
        pthread_mutex_init(&(up_lock[i]), NULL);
    
    if (numthrds > MAX_TH) error("Too many threads. Can be changed by "
                                 "modifying const uint MAX_TH");

    // parallel
    printf("Computing parallel time...\n");
    gettimeofday(&start, NULL);
    compute_par();
    gettimeofday(&end, NULL);
    ptime = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
    printf("Writing output...\n");

    char *filename = argv[2];
    write_results(filename, stime, ptime);

    return 0;
} // main()

// run a parallel computation
void compute_par() {
    pthread_t p_threads[numthrds];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    struct thread_data t_data[numthrds];

    for (uint i=0; i<numthrds; i++) {
        uint lower, upper;
        compute_bounds(lower, upper, i);
        // contruct a thread struct for each thread and pass this as a
        // parameter this will hold things like the thread is, and its
        // boundaries
        t_data[i].id = i;
        t_data[i].lower = lower;
        t_data[i].upper = upper;
        t_data[i].color = BLACK;
        pthread_create(&p_threads[i], &attr, path, &t_data[i]);
    }

    for (uint i=0; i<numthrds; i++) {
        pthread_join(p_threads[i], NULL);
    }
} // compute_par()

// run a sequential computation
void compute_seq() {
    struct thread_data s_data;
    s_data.id = 0;
    s_data.lower = 0;
    s_data.upper = num_nodes;
    s_data.color = BLACK;
    path(&s_data);
} // compute_seq()

// find the shortest paths...
void *path(void *ptr) {
    struct thread_data *t_ptr  = (struct thread_data *)ptr;
    const uint seed = t_ptr->id;
    const uint lower = t_ptr->lower;
    const uint upper = t_ptr->upper;
    short color = t_ptr->color;
    uint range = upper-lower;
    
    vector<QNode> myqueue;
    set<QNode,setcomp> remote_updates[numthrds];

    for (uint i=lower; i<upper; i++) {
        vector<QNode>::iterator si;
        if (i == source) {range--; continue;}
        myqueue.push_back(qnodes_v[i]);

    }
    make_heap(myqueue.begin(), myqueue.end(), CompareNodes_q());

    QNode qn;
    uint attempts = 0;
    while (!token.isover) {
        if (myqueue.size() != 0) {
            qn = myqueue.front();
            if (qn.distance == UINT_MAX - 2) {
                perform_update(seed, myqueue, &color);
                qn = myqueue.front();
                if (qn.distance == UINT_MAX - 2) {
                    if (attempts++ > 200) break;
                    usleep(20);
                    continue;
                }
            }
            uint distance = qnodes_v[qn.id].distance+WEIGHT;

            for (vector<uint>::iterator vi=qn.connect_to.begin();
                 vi!=qn.connect_to.end(); ++vi) {
                if ((*vi) == source) continue;
                // check local
                if (*vi >= lower && *vi < upper) {
                    uint d = qnodes_v[*vi].distance;
                    if (distance < d) {
                        color = BLACK;
                        decrease_key(*vi, distance, myqueue, &color);
                    }
                }
                // alert remote thread
                else {
                    uint id = get_proc(*vi);
                    color = BLACK;
                    set<QNode,setcomp>::iterator si;
                    set_find(remote_updates[id], *vi, si);
                    if (si != remote_updates[id].end()) {
                        if (si->distance > distance) {
                            QNode qn = *si;
                            qn.distance = distance;
                            remote_updates[id].erase(si);
                            remote_updates[id].insert(qn);
                        }
                    }
                    else {
                        QNode qn = qnodes_v[*vi];
                        qn.distance = distance;
                        remote_updates[id].insert(qn);
                    }
                }
            }
        }
        // send remote updates
        send_remotes(remote_updates, seed);
        perform_update(seed, myqueue, &color);
        if (myqueue.size() > 0) {
            pop_heap(myqueue.begin(), myqueue.end(), CompareNodes_q());
            myqueue.pop_back();
        }

        // no more work - pass the token
        if (myqueue.empty() && (token.owner == seed)) {
            if (color == WHITE && token.owner == 0 &&
                token.color == WHITE && seed == 0) {
                token.isover = true;
                break;
            }
            if (seed == 0 && token.owner == 0) {
                token.color = color;
                token.owner = 1%numthrds;
            }
            else if (color == BLACK) {
                token.color = BLACK;
                token.owner = (seed+1)%numthrds;
            }
            else 
                token.owner = (seed+1)%numthrds;
            color = WHITE;
            usleep(2);
        }
        if (myqueue.empty() && color == WHITE) 
            usleep(2);

        if (seed == 0) stop++;
    }
} // path()

void send_remotes(set<QNode,setcomp> *remote_updates, const uint seed) {
    for (uint i=0; i<numthrds; i++) {
        if (remote_updates[i].size() != 0) {
            for (set<QNode,setcomp>::iterator si=remote_updates[i].begin();
                 si!=remote_updates[i].end(); ++si) {
                if (qnodes_v[si->id].distance > si->distance) {
                    pthread_mutex_lock(&(up_lock[i]));
                    update_v[i].push_back(*si);
                    pthread_mutex_unlock(&(up_lock[i]));
                }
            }
            remote_updates[i].clear();
        }
    }
}

void set_find(set<QNode,setcomp> &myset, const uint id,
              set<QNode,setcomp>::iterator &si) {
    for (si=myset.begin(); si!=myset.end(); ++si)
        if (si->id == id)
            break;
}

void perform_update(const uint seed, vector<QNode> &myqueue, short *color) {
    vector<QNode>::iterator vis,vie;
    pthread_mutex_lock(&(up_lock[seed]));
    vis = update_v[seed].begin();
    vie = update_v[seed].end();
    pthread_mutex_unlock(&(up_lock[seed]));
    for (; vis!=vie; ++vis) 
        decrease_key(vis->id, vis->distance, myqueue, color);

    pthread_mutex_lock(&(up_lock[seed]));
    update_v[seed].clear();
    pthread_mutex_unlock(&(up_lock[seed]));
}

inline void decrease_key(const uint id, const uint newkey,
                         vector<QNode> &myqueue, short *color) {
    vector<QNode>::iterator ui;
    QNode *qn = &(qnodes_v[id]);
    for (ui=myqueue.begin(); ui!=myqueue.end(); ++ui) {
        if (ui->id == id && ui->distance > newkey) {
            qn->distance = newkey;
            vector<QNode>::iterator mi;
            for (mi=myqueue.begin();
                 mi!=myqueue.end(); ++mi) {
                if (mi->id == id) break;
                    
            }
            if (mi != myqueue.end()) 
                myqueue.erase(mi);
            (*color) = BLACK;
            myqueue.push_back(*qn);
            push_heap(myqueue.begin(), myqueue.end(), CompareNodes_q());
            
            break;
        }
    }
    if (ui == myqueue.end() && qn != NULL && qn->distance > newkey) {
        (*color) = BLACK;
        qn->distance = newkey;
        myqueue.push_back(*qn);
        push_heap(myqueue.begin(), myqueue.end(), CompareNodes_q());
    }
}
        
// write the results of this random walk to the file specified
void write_results(const char *filename, const double stime,
                   const double ptime) {
    ofstream outfile;
    outfile.open(filename, ios::out);
    outfile << "sequential: " << stime << endl;
    outfile << "parallel: " << ptime << endl;

    for (vector<QNode>::iterator vi=qnodes_v.begin();
         vi!=qnodes_v.end(); ++vi) 
        outfile << vi->id << "\t" << vi->distance << endl;
    outfile.close();
} // write_results()

// parses the input file and creates a NodeData object for each node
void parse_input(char *file) {
    char *line = strtok(file, "\n");
    uint line_nums[] = {0,0};
    uint last_id = 0, maxid = 0, nsize;
    num_nodes = 0;

    QNode qn;
    while (line != NULL) {
        get_line(line, line_nums);
        nsize = max(line_nums[0],line_nums[1]);
        if (line_nums[0] >= qnodes_v.size() ||
            line_nums[1] >= qnodes_v.size()) {
            qnodes_v.resize(nsize * 2);
        }
        if (qnodes_v[line_nums[1]].connect_to.size() == 0) {
            qn.id=line_nums[1];
            qnodes_v[line_nums[1]] = qn;
        }
        qnodes_v[line_nums[1]].connect_to.push_back(line_nums[0]);
        if (qnodes_v[line_nums[0]].connect_to.size() == 0) {
            qn.id=line_nums[0];
            qnodes_v[line_nums[0]] = qn;
        }
        maxid = (maxid > nsize)?maxid:nsize;

        line = strtok(NULL, "\n");
    }
    maxid++;
    qnodes_v.resize(maxid);
    qnodes_v.shrink_to_fit();
    num_nodes = qnodes_v.size();

    free(file);
} // parse_input()

// gets a line from the file and parses it. places the first two itmes in
// line_nums[0] and [1]
inline void get_line(char *line, uint line_nums[2]) {
    int start=0;
    line_nums[0] = get_line_part(line, &start);
    line_nums[1] = get_line_part(line, &start);
} // get_line()

// gets portion of the line
inline uint get_line_part(char *line, int *start) {
    char buf[100];
    int i=0;
    while (line[*start] != '\t') 
        buf[i++] = line[(*start)++];

    (*start)++;
    buf[i] = '\0';

    return atoi(buf);
} // get_lines_part()

// read the file fname from the disk and return a pointer to the file
char *read_file(const char *fname) {
    char *file;
    int fd = open(fname, O_RDONLY);
    if (fd <= 0) error("Error opening file.");

    long end = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    long nbytes = end;

    file = (char *)malloc(sizeof(char) * nbytes);
    read(fd, file, nbytes);

    return file;
} // read_file()

// print str and exit the program (something has gone wrong)
void error(const char *str) {
    printf("%s\n",str);
    exit(-1);
} // error()

// given a nodes rank will compute the lower and upper ids that it controls
void compute_bounds(uint &lower, uint &upper, const uint rank) {
    int nodes_per;
    nodes_per = round((float)num_nodes / (uint)numthrds);
    if (rank == numthrds-1 && (uint)(numthrds*nodes_per) < num_nodes) {
        nodes_per += num_nodes - (numthrds * nodes_per);
        lower = rank * round((float)num_nodes / numthrds);
        upper = lower + nodes_per;
    }
    else {
        lower = rank * nodes_per;;
        upper = lower + nodes_per;
    }
    if (upper > num_nodes) upper = num_nodes;
} // compute_bounds()

uint get_proc(const uint rank) {
    uint lower, upper;
    for (int i=0; i<numthrds; i++) {
        compute_bounds(lower, upper, i);
        if (rank >= lower && rank < upper)
            return i;
    }
}

void set_weights(vector<QNode> &nodes) {
    vector<QNode>::iterator it, si;
    for (it=qnodes_v.begin(); it!=qnodes_v.end(); ++it)
        (*it).distance = UINT_MAX-2;
    
    for (it=qnodes_v.begin(); it!=qnodes_v.end(); ++it) {
        if ((*it).id == source) {
            (*it).distance = 0;
            break;
        }
    }
    
    for (vector<uint>::iterator vi=(*it).connect_to.begin();
         vi!=(*it).connect_to.end(); ++vi) {
        for (si=qnodes_v.begin(); si!=qnodes_v.end(); ++si)
            if ((*si).id == *vi)
                break;
        (*si).distance = WEIGHT;
    }
}

void print_heap(const uint seed, const uint lower, const uint upper,
                vector<QNode> &myqueue, const uint max) {
    vector<QNode> pheap = myqueue;
    vector<QNode>::iterator pnode;
    uint stop = 0;
    pthread_mutex_lock(&pr_lock);
    cout << "th: " << seed << " ";
    while (pheap.size() > 0 && stop++ < max) {
        pnode = pheap.begin();
        cout << pnode->id << "[" << pnode->distance << "] ";
        pop_heap(pheap.begin(), pheap.end(), CompareNodes_q());
        pheap.pop_back();
        make_heap(pheap.begin(), pheap.end(), CompareNodes_q());
    }
    cout << endl;
    pthread_mutex_unlock(&pr_lock);
}

// void print_nodes(vector<NodeData> &nodes, const char *name, uint seed) {
void print_nodes(vector<QNode> &qnodes_v, uint seed) {
    for (uint i=0; i<qnodes_v.size(); i++) 
        cout << qnodes_v[i].id << "\t" << qnodes_v[i].distance << endl;
}
