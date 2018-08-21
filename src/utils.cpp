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

#include "oqt/utils.hpp"
#include <experimental/filesystem>
#include <fstream>
#include <string>
#include "oqt/simplepbf.hpp"

#include <malloc.h>

namespace oqt {
    


size_t write_uint32(std::string& data, size_t pos, uint32_t i) {
    union {
        uint32_t si;
        char sc[4];
    };
    si=i;
    data[pos+3] = sc[0];
    data[pos+2] = sc[1];
    data[pos+1] = sc[2];
    data[pos] = sc[3];
    return pos+4;
}

size_t write_uint32_le(std::string& data, size_t pos, uint32_t i) {
    union {
        uint32_t si;
        char sc[4];
    };
    si=i;
    data[pos] = sc[0];
    data[pos+1] = sc[1];
    data[pos+2] = sc[2];
    data[pos+3] = sc[3];
    return pos+4;
}


uint32_t read_uint32_le(const std::string& data, size_t pos) {
    union {
        uint32_t si;
        char sc[4];
    };
    sc[0] = data[pos];
    sc[1] = data[pos+1];
    sc[2] = data[pos+2];
    sc[3] = data[pos+3];
    
    return si;
}

size_t write_double(std::string& data, size_t pos, double d) {
    union {
        double sd;
        char sc[8];
    };
    sd=d;
    data[pos+7] = sc[0];
    data[pos+6] = sc[1];
    data[pos+5] = sc[2];
    data[pos+4] = sc[3];
    data[pos+3] = sc[4];
    data[pos+2] = sc[5];
    data[pos+1] = sc[6];
    data[pos] = sc[7];
    return pos+8;
}

size_t write_double_le(std::string& data, size_t pos, double d) {
    union {
        double sd;
        char sc[8];
    };
    sd=d;
    data[pos] = sc[0];
    data[pos+1] = sc[1];
    data[pos+2] = sc[2];
    data[pos+3] = sc[3];
    data[pos+4] = sc[4];
    data[pos+5] = sc[5];
    data[pos+6] = sc[6];
    data[pos+7] = sc[7];
    return pos+8;
}


std::string date_str(int64 date) {
    time_t datet = time_t(date);

    tm * datetm = gmtime(&datet);
    std::string res(20,0);
    size_t l=strftime(&res[0],20,"%Y-%m-%dT%H:%M:%S",datetm);
    return res.substr(0,l);
}
int64 read_date(std::string in) {

    std::tm timeinfo={};
    if ((in.size()==20) && (in[19]=='Z')) {
        in=in.substr(0,19);
    }
    if (in.size()==8) {
        if(strptime(in.c_str(), "%Y%m%d", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    } else if (in.size()==10) {

        if(strptime(in.c_str(), "%Y-%m-%d", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    } else if (in.size()==19) {

        if (strptime(in.c_str(), "%Y-%m-%dT%H-%M-%S", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        } else if (
            strptime(in.c_str(), "%Y-%m-%dT%H:%M:%S", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    }

    return 0;
}


std::string compress(const std::string& data, int level) {
    std::string out(data.size()+100,0);

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // setup "a" as the input and "b" as the compressed output
    defstream.avail_in = data.size(); // size of input, string + terminator
    defstream.next_in = (Bytef *)&data[0]; // input char array

    defstream.avail_out = (uInt)out.size(); // size of output
    defstream.next_out = (Bytef *)&out[0]; // output char array

    // the actual compression work.
    //deflateInit(&defstream, Z_BEST_COMPRESSION);
    //deflateInit(&defstream,Z_DEFAULT_COMPRESSION);
    deflateInit(&defstream,level);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    out.resize(defstream.total_out);

    return out;

}
std::string decompress(const std::string& data, size_t size) {
    if (size==0) {
        return data;
    }
    std::string out(size,0);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)(data.size()); // size of input
    infstream.next_in = (Bytef *)(&data[0]); // input char array
    infstream.avail_out = (uInt)(size); // size of output
    infstream.next_out = (Bytef *)(&out[0]); // output char array

    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    return out;
}
std::string compress_gzip(const std::string& fn, const std::string& data, int level) {
    std::string comp = compress(data,level);
    size_t ln= 10;
    if (!fn.empty()) {
        ln += 1;
        ln += fn.size();
    }
    size_t comp_pos=ln;
    ln += comp.size()-6;
    size_t tail_pos=ln;
    ln += 8;
    
    std::string output(ln,'\0');
    
    output[0] = '\037'; //magic
    output[1] = '\213'; //magic
    output[2] = '\010'; //compression method
    
    uint32_t flags = 0;
    if (!fn.empty()) {
        flags |= 8;
    }
    
    output[3] = (char) flags;
    
    //uint32_t mtime=time(0);
    uint32_t mtime = 1514764800; //2018-01-01T00:00:00Z (so repeated calls produce same result)
    
    write_uint32_le(output, 4, mtime);
    
    output[8] = '\002';
    output[9] = '\377';
    if (!fn.empty()) {
        std::copy(fn.begin(),fn.end(),output.begin()+10);
        output[11+fn.size()] = '\0';
    }
    
    std::copy(comp.begin()+2, comp.end()-4, output.begin()+comp_pos);
    
    uint32_t crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)&data[0], data.size());
        
    
    write_uint32_le(output, tail_pos, crc);
    write_uint32_le(output, tail_pos+4, data.size());
    return output;
}
std::pair<std::string,size_t> gzip_info(const std::string& data) {
    if (
            (data[0] != '\037')
         || (data[1] != '\213')
         || (data[2] != '\010')
         || (data[8] != '\002')
         || (data[9] != '\377')) {
        throw std::domain_error("not a gzipped string");
    }
    
    std::string fn;
    if (data[3] == '\010') {
        fn = std::string(&data[10]);
    }
    size_t mtime = read_uint32_le(data, 4);
    return std::make_pair(fn, mtime);
}
        
    
    
std::string decompress_gzip(const std::string& data) {
    size_t size = read_uint32_le(data, data.size()-4);
    std::string out(size,0);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)(data.size()); // size of input
    infstream.next_in = (Bytef *)(&data[0]); // input char array
    infstream.avail_out = (uInt)(size); // size of output
    infstream.next_out = (Bytef *)(&out[0]); // output char array

    // the actual DE-compression work.
    inflateInit2(&infstream, 15+MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    return out;
}   

class logger_impl : public logger {
    public:
        logger_impl() : msgw(0) {}

        void message(const std::string& msg) {
            
            if (msgw!=0) {
                std::cout << std::endl;
            }
            std::cout << msg << std::endl;
            msgw=0;
        }
        void progress(double percent, const std::string& msg) {
            if (msg.size()>msgw) { msgw=msg.size(); }
            std::cout << "\r" << std::fixed << std::setprecision(1) << std::setw(5) << percent << " " << std::setw(msgw) << msg << std::flush;
        }

        ~logger_impl() {
            if (msgw!=0) {
                std::cout << std::endl;
            }
        }
    private:
        size_t msgw;
};

std::shared_ptr<logger> make_default_logger() { return std::make_shared<logger_impl>(); }


std::shared_ptr<logger> logger_;


std::shared_ptr<logger> get_logger() {
    if (!logger_) {
        logger_ = make_default_logger();
    }
    return logger_;
}

void set_logger(std::shared_ptr<logger> nlog) {
    logger_ = nlog;
}



int64 file_size(const std::string& fn) {
    namespace fs = std::experimental::filesystem;
    fs::path p{fn};
    p = fs::canonical(p);
    return fs::file_size(p);
    
}



bool point_in_poly(const std::vector<lonlat>& verts, const lonlat& test) {
    /*
       from http://www.ecse.rpi.edu/~wrf/Research/Short_Notes/pnpoly.html

       Copyright (c) 1970-2003, Wm. Randolph Franklin

       Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

           1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
           2. Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided with the distribution.
           3. The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific prior written permission.

       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

       int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
       {
         int i, j, c = 0;
         for (i = 0, j = nvert-1; i < nvert; j = i++) {
           if ( ((verty[i]>testy) != (verty[j]>testy)) &&
         (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
              c = !c;
         }
         return c;
       }*/
    if (verts.size()<3) { return false; }
/*
def pnpoly(vertx, verty, testx, testy):
    j, c = len(vertx)-1,False
    for i in range(len(vertx)):
        if ( ((verty[i]>testy) != (verty[j]>testy)) and (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) ):
            c = not c
        j=i
    return c
*/
    size_t j = verts.size()-1;
    bool c= false;
    double testlon = test.lon, testlat = test.lat;
    for (size_t i=0; i < verts.size(); i++) {

        double loni = verts.at(i).lon, lati = verts.at(i).lat;
        double lonj = verts.at(j).lon, latj = verts.at(j).lat;

        if ( ((lati>testlat)!=(latj>testlat)) &&
            (testlon < (lonj - loni) * (testlat- lati) / (latj  - lati) + loni)) {
            c = !c;
        }

        j = i;
    }
    return c;
}

int segment_side(const lonlat& p1, const lonlat& p2, const lonlat& q) {
    int64 side = (q.lon - p1.lon) * (p2.lat - p1.lat) - (p2.lon - p1.lon) * (q.lat - p1.lat);
    if (side>0) { return 1; }
    if (side<0) { return -1; }
    return 0;
}
    

bool segment_intersects(const lonlat& p1, const lonlat& p2, const lonlat& q1, const lonlat& q2) {
    int pq1 = segment_side(p1,p2,q1);
    int pq2 = segment_side(p1,p2,q2);
    if (pq1==pq2) { return false; }
    
    int qp1 = segment_side(q1,q2,p1);
    int qp2 = segment_side(q1,q2,p2);
    if (qp1==qp2) { return false; }
    return true;
}
    
    
bool line_intersects(const lonlatvec& line1, const lonlatvec& line2) {
    if ((line1.size()<2) || (line2.size()<2)) { return false; }
    
    for (size_t i=0; i < (line1.size()-1); i++) {
        for (size_t j=0; i < (line2.size()-1); j++) {
            if (segment_intersects(line1[i],line1[i+1],line2[i],line2[j+1])) {
                return true;
            }
        }
    }
    return false;
}
    
    
bool line_box_intersects(const lonlatvec& line, const bbox& box) {
    if (line.size()<2) { return false; }
    
    lonlat a{box.minx,box.miny};
    lonlat b{box.maxx,box.miny};
    lonlat c{box.maxx,box.maxy};
    lonlat d{box.minx,box.maxy};
 
    
    for (size_t i=0; i < (line.size()-1); i++) {
        if (segment_intersects(line[i],line[i+1],a,b)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],b,c)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],c,d)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],d,a)) {
            return true;
        }
        
        
    }
    return false;
}

bool polygon_box_intersects(const lonlatvec& line, const bbox& box) {
    if (line.size()<3) { return false; }
    if (box_empty(box)) { return false; }
    
    if (line_box_intersects(line,box)) { return true; }
    
    if (contains_point(box, line[0].lon, line[0].lat)) {
        return true;
    }
    
    if (point_in_poly(line, lonlat{box.minx,box.miny})) {
        return true;
    }
    
    return false;
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
