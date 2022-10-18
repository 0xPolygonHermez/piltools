#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <map>
#include <string>
#include <sys/mman.h>
#include <errno.h>
#include "parser.hpp"


int main ( int argc, char *argv [])
{
    pil::Parser p;


    p.compile("A +   3 === 12 *    2 ");
    p.compile("function(Pols + 2 * 6,Pols[4], Main.Pols[4], Main.Clock, Main.A0, DEF,HHJK)");
}