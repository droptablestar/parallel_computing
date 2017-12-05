/*
 * graph_fhandler: A file handler for graphs of the form
 * Destination\tSource\tInfo
 *
 * @author jreese
 */

#include <iostream>

#include "../include/graph_fhandler.hpp"

using namespace std;

/**
 * Constructs a new fhandler object
 *
 * @param filename the name of the file to read from
 */
graph_fhandler::graph_fhandler(const char *filename):
    filename(filename), file(NULL){
} // graph_fhandler()

/**
 * Deconstructs a fhandler object
 *
 * Releases memory allocated by the file handler and closes the file.
 */
graph_fhandler::~graph_fhandler() {
    close();
    free_file();
} // ~graph_fhandler()

/**
 * Opens the file this handler is operating on
 */
int graph_fhandler::open() {
    inputfile.open(filename, ios::in);

    if (inputfile.is_open()) return 1;
    else {er+="Error opening file\n"; return 0;}
} // open()

/**
 * Closes the file this handler is operating on
 */
void graph_fhandler::close() {
    if (inputfile.is_open()) inputfile.close();
} // close()

/**
 * Obtains the size of the file (in bytes), reads it into memory, and stores
 * the result in the file variable
 */
int graph_fhandler::read_file() {
    long begin, end;

    if (!open()) return 0;

    begin = inputfile.tellg();
    inputfile.seekg(0, ios::end);
    end = inputfile.tellg();
    inputfile.seekg(0, ios::beg);

    long nbytes = end-begin;

    file = (char *)operator new(sizeof(char) * nbytes);
    inputfile.read(file, nbytes);

    return 1;
} // read_file()

/**
 * Deletes the memory allocated for the file. This should be called when
 * the file is no longer needed (e.g. when the parser is finished
 * constructing the node objects.
 */
void graph_fhandler::free_file() {
    if (file) {
        delete file;
        file = NULL;
    }
} // free_file()

/**
 * Returns the contents of the file
 *
 * @return the contents of the file
 */
char *graph_fhandler::get_data() {
    return file;
} // get_data()

/**
 * Returns any errors associated with opening and reading this file
 *
 * @return the contents of the error log
 */
string graph_fhandler::get_error() {
    return er;
} // get_error()
