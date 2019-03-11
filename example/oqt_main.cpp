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


#include "oqt/count.hpp"
#include "oqt/sorting/qttreegroups.hpp"
#include <regex>



#include "oqt/calcqts/calcqts.hpp"

#include "oqt/calcqts/calcqtsinmem.hpp"
#include <fstream>
#include "oqt/update/applychange.hpp"
#include "oqt/sorting/mergechanges.hpp"
#include "oqt/sorting/sortblocks.hpp"

#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/utils/date.hpp"


using namespace oqt;
bbox read_bbox(const std::string& str) {
    try {
        
        std::regex r{"(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)", std::regex::extended};
        
        std::smatch p;
        if (!std::regex_match(str, p, r)) {
            throw std::domain_error("not a bbox");
        }

        bbox ans;
        ans.minx = atoi(p.str(1).c_str());
        ans.miny = atoi(p.str(2).c_str());
        ans.maxx = atoi(p.str(3).c_str());
        ans.maxy = atoi(p.str(4).c_str());
        return ans;
    } catch(std::regex_error& err) {
        Logger::Message() << "regex " << err.what();
        throw err;
    }

}



LonLat readpoly_coord(const std::string& line) {

    size_t p=0;
    double lon = std::stod(line, &p);
    if (p==0) { throw std::domain_error("not a number"); }
    if (p==line.size()) {
        throw std::domain_error("only one");
    }
    double lat = std::stod(line.substr(p, line.size()-p));
    
    return LonLat{coordinate_as_integer(lon),coordinate_as_integer(lat)};
}

std::vector<LonLat> read_poly_file(const std::string& fn) {
    
    std::vector<LonLat> poly;
    std::ifstream file(fn, std::ios::in);
    if (!file.good()) {
        throw std::domain_error("can't open file");
    }
    
    std::string line;
    
    
    
    bool hasname=false;
    bool inring=false;
    bool finished=false;
    while(std::getline(file, line)) {
        
        if (!hasname) {
            hasname=true;
        } else if (line=="END") {
            if (inring) {
                inring=false;
            } else {
                finished=true;
            }
        } else if (!inring) {
            if (finished) {
                throw std::domain_error("multiple rings??");
            }
            inring=true;
        } else {
            poly.push_back(readpoly_coord(line));
        }
    }
    if (!finished) {
        throw std::domain_error("not finished??");
    }
        
    return poly;
}

bbox poly_bounds(const std::vector<LonLat>& poly) {
    bbox res{1800000000,900000000,-1800000000,-900000000};
    for (const auto& ll: poly) {
        if (ll.lon < res.minx) { res.minx=ll.lon; }
        if (ll.lat < res.miny) { res.miny=ll.lat; }
        if (ll.lon > res.maxx) { res.maxx=ll.lon; }
        if (ll.lat > res.maxy) { res.maxy=ll.lat; }
    }
    return res;
}

class Logger_stdout : public Logger {
    
    public:
        Logger_stdout(): pm(false) {}
        
        virtual ~Logger_stdout() {}
        
        void message(const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex);
            if (pm) {
                std::cout << std::endl;
            }
            std::cout << "[" << TmStr{st.since(), 9,1} << "]: " << msg << std::endl;
            pm=false;
            
        }
        void progress(double percent, const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex);
            if ((percent>=100.0) || (lp.since()>1)) {
                std::cout << "\r[" << TmStr{st.since(), 9,1} << "" << std::fixed << std::setprecision(1) << std::setw(5) << percent << "%]: " << msg << std::flush;
                lp.reset();
                if (percent>=100.0) {
                    std::cout << std::endl;
                    pm=false;
                } else {
                    pm=true;
                }
            }
        } 
    private:
        TimeSingle st;
        TimeSingle lp;
        std::mutex mutex;
        bool pm;
    
};

int main(int argc, char** argv) {
    
    Logger::Set(std::make_shared<Logger_stdout>());
    
    
    if (argc < 3) {
        Logger::Message() << "specify operation and file name";
    }
    std::string operation(argv[1]);

    std::string origfn(argv[2]);


    if (!std::ifstream(origfn, std::ios::in | std::ios::binary).good()) {
        Logger::Message() << "failed to open " << origfn;
        return 1;
    }

    size_t numchan=std::thread::hardware_concurrency();
    std::string qtsfn=origfn.substr(0,origfn.size()-4)+std::string("-qts.pbf");
    std::string outfn=origfn.substr(0,origfn.size()-4)+std::string("-sorted.pbf");
    std::string tempfn="";
    int64 timestamp = 0;

    bool rollup=false;
    int64 targetsize=8000;
    int64 minsize   =4000;
    size_t grptiles = 0;
    double buffer = 0.05;
    size_t max_depth=17;

    bool sort_objs=false;
    bool sortfile=true;
    bool filter_objs=false;
    bbox filter_box{-1800000000,-900000000,1800000000,900000000};
    bool inmem=false;
    std::vector<std::string> changes;
    bool countgeom=false;
    bool splitways=true;
    std::vector<LonLat> poly;
    size_t countflags=15;
    bool use_tree=false;
    bool use_48bit_quadtrees=false;
    bool fixstrs=false;
    bool seperate_filelocs=true;
    //bool usefindgroupscopy=false;
    if (argc>3) {
        for (int i=3; i < argc; i++) {
            std::string arg(argv[i]);

            std::string key = arg;
            std::string val = "";
            auto eqp = arg.find("=");
            if (eqp < std::string::npos) {
                key=arg.substr(0,eqp+1);
                val=arg.substr(eqp+1,arg.size());
            }

            if (key=="qtsfn=") {
                qtsfn = val;
            } else if (key=="outfn=") {
                outfn = val;

            } else if (key=="timestamp=") {

                timestamp = read_date(val);
                Logger::Message() << "read timestamp \"" << val << "\" as " << timestamp;
            } else if (key=="numchan=") {
                numchan = atoi(val.c_str());
            } else if (key=="rollup") {
                rollup=true;
                Logger::Message() << "rollup=true";
            } else if (key=="targetsize=") {
                targetsize = atoi(val.c_str());
                Logger::Message() << "targetsize=" << targetsize;
            } else if (key=="minsize=") {
                minsize = atoi(val.c_str());
                Logger::Message() << "minsize=" << minsize;
            } else if (key=="tempfn=") {
                tempfn = val;
                Logger::Message() << "tempfn=" << tempfn;
            } else if (key=="sort") {
                sort_objs=true;
            } else if (key=="dontsortfile") {
                sortfile=false;
            } else if (key == "filter=") {
                if (val=="planet") {
                    //leave as default
                } else if (val=="nwkent") {
                    filter_box=bbox{750000,512000000,3000000,514000000};
                } else if (ends_with(val, ".poly")) {
                    poly = read_poly_file(val);
                    filter_box = poly_bounds(poly);
                                        
                } else {
                    filter_box = read_bbox(val);
                }

                Logger::Message() << "read filter " << val << " as " << filter_box;
            } else if (key=="filterobjs") {
                filter_objs=true;
            } else if (key=="grptiles=") {
                grptiles = atoi(val.c_str());
                Logger::Message() << "grptiles=" << grptiles;
            
            } else if (key=="inmem") {
                inmem=true;
                Logger::Message() << "inmem";
            } else if (key=="change=") {
                changes.push_back(val);
            } else if (key=="countgeom") {
                countgeom=true;
            } else if (key=="buffer=") {
                buffer = atof(val.c_str());
            } else if (key=="max_depth=") {
                max_depth=atoi(val.c_str());
            } else if (key=="dontsplitways") {
                splitways=false;
                Logger::Message() << "splitways=false";
            } else if (key=="countflags=") {
                countflags = std::stoull(val);  
            } else if (key=="usetree") {
                use_tree=true;
            } else if (key=="48bitqt") {
                use_48bit_quadtrees=true;
                Logger::Message() << "use_48bit_quadtrees=true";
            } else if (key=="fixstrs") {
                fixstrs=true;
            } else if (key=="notseperatefilelocs") {
                seperate_filelocs=false;
            } else {
               Logger::Message() << "unrecongisned argument " << arg;
               return 1;
            } 
        }
    }
    if (tempfn.empty()) {
        tempfn = outfn+"-interim";
    }
    
    
    
    
    Logger::Message() << "numchan=" << numchan;

    //auto lg = make_default_logger();

    int resp=0;
    if (operation == "count") {
        
        //bool useminimal = (countflags&32)==0;
        Logger::Message() <<  "count fn=" << origfn << ", numchan=" << numchan << ", countgeom=" << countgeom << ", countflags=" << countflags;// << ", useminimal=" << useminimal;
        auto res = run_count(origfn, numchan,false, countgeom, countflags, true);//useminimal);
        if (!res) { return 1; }
        Logger::Message() << "\n"<<res->long_str();
    } else if (operation == "count_full") {
        auto res = run_count(origfn, numchan,false, countgeom, countflags, false);
        if (!res) { return 1; }
        Logger::Message() << "\n"<<res->long_str();
    } else if (operation == "calcqts") {
        if (inmem) {
            run_calcqts_inmem(origfn, qtsfn, numchan, true);
        } else {
            run_calcqts(origfn, qtsfn, numchan, splitways, true, buffer, max_depth, use_48bit_quadtrees);
        }

    } else if (operation=="sortblocks") {
        std::shared_ptr<QtTree> groups;
        //if (usefindgroupscopy) {
        
        if (true) {
            auto tree = make_qts_tree_maxlevel(qtsfn=="NONE" ? origfn : qtsfn, numchan, max_depth);
            Logger::Get().time("load qts");
            if (rollup) {
                tree_rollup(tree,minsize);
                Logger::Get().time("rollup tree");
            }
            if (use_tree) {
                if (max_depth>15) { throw std::domain_error("use_tree with too large a max_depth??"); }
                groups=tree;
            } else {
                groups = find_groups_copy(tree,targetsize,minsize);
                Logger::Message() << "find groups";
                Logger::Get().time("find groups");
                tree.reset();
            }
        //} else {
        //    groups = run_findgroups(qtsfn,numchan,rollup,targetsize, minsize);
        }
        
        checkstats();
        if (inmem) {
            resp = run_sortblocks_inmem(origfn,qtsfn,outfn,timestamp, numchan, groups, fixstrs);
        } else {
            
            resp = run_sortblocks(origfn,qtsfn,outfn,timestamp,  numchan, groups, tempfn, grptiles, fixstrs, seperate_filelocs);
        }
        

    } else if (operation=="mergechanges") {
        if (grptiles==0) { grptiles=500; }
        run_mergechanges(origfn, outfn, numchan, sort_objs, filter_objs, filter_box, poly, timestamp, tempfn, grptiles, sortfile,inmem);
        
    } else if (operation=="applychange") {
        run_applychange(origfn, outfn, numchan, changes);
    } else if (operation=="readfile") {
        
        std::ifstream file(origfn, std::ios::binary | std::ios::in);
        
        int64 idx=0;
        int64 tl=0;
        auto bl = read_file_block(idx, file);
        while (bl) {
            auto dd = decompress(bl->data,bl->uncompressed_size);
            tl+=dd.size();
            idx++;
            bl = read_file_block(idx, file);
        }
        file.close();
        Logger::Message() << "uncompressed size: " << tl;
    }
    
            
        
        
    checkstats();
    Logger::Get().timing_messages();
    return resp;
}
