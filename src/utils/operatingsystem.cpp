/*****************************************************************************
 *
 * This file is part of osmquadtree
 *
 * Copyright (C) 2018 James Harris
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include "oqt/utils/operatingsystem.hpp"
#include <experimental/filesystem>
#include <malloc.h>
#include <fstream>
#include <iostream>
namespace oqt {



int64 file_size(const std::string& fn) {
    namespace fs = std::experimental::filesystem;
    fs::path p{fn};
    p = fs::canonical(p);
    return fs::file_size(p);
    
}



std::string getmem(size_t pid) {
    
    
    std::ifstream status("/proc/"+std::to_string(pid)+"/status", std::ios::in);
    std::string ln;
    while (status.good()) {
        std::getline(status, ln);
        if (ln.find("VmRSS") != std::string::npos) {
            return ln;
        }
    }
    return "??";
}

int64 getmemval(size_t pid) {
    std::ifstream statm("/proc/"+std::to_string(pid)+"/statm", std::ios::in);
    std::string str{std::istreambuf_iterator<char>(statm), std::istreambuf_iterator<char>()};
    size_t a = str.find(' ',0);
    size_t b = str.find(' ',a+1);
    
    return std::stoll(str.substr(a,b-a))*4096;
}

void checkstats() {
    size_t pid=getpid();
    std::cout << "pid = " << pid << " " << getmem(pid) << std::endl;
}

bool trim_memory() {
    return malloc_trim(0)==1;
}
    
}
