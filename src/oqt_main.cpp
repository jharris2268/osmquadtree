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
#include "oqt/quadtreegroups.hpp"
#include <regex>

#include <fstream>

#include "oqt/calcqts/calcqts.hpp"
#include "oqt/update/applychange.hpp"
#include "oqt/sorting/mergechanges.hpp"
#include "oqt/sorting/sortblocks.hpp"

#include "oqt/pbfformat/readfileblocks.hpp"



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
        logger_message() << "regex " << err.what();
        throw err;
    }

}



lonlat readpoly_coord(const std::string& line) {

    size_t p=0;
    double lon = std::stod(line, &p);
    if (p==0) { throw std::domain_error("not a number"); }
    if (p==line.size()) {
        throw std::domain_error("only one");
    }
    double lat = std::stod(line.substr(p, line.size()-p));
    
    return lonlat{toInt(lon),toInt(lat)};
}

lonlatvec read_poly_file(const std::string& fn) {
    
    lonlatvec poly;
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

bbox poly_bounds(const lonlatvec& poly) {
    bbox res{1800000000,900000000,-1800000000,-900000000};
    for (const auto& ll: poly) {
        if (ll.lon < res.minx) { res.minx=ll.lon; }
        if (ll.lat < res.miny) { res.miny=ll.lat; }
        if (ll.lon > res.maxx) { res.maxx=ll.lon; }
        if (ll.lat > res.maxy) { res.maxy=ll.lat; }
    }
    return res;
}

class logger2 : public logger {
    
    public:
        logger2(): pm(false) {}
        
        virtual ~logger2() {}
        
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
        time_single st;
        time_single lp;
        std::mutex mutex;
        bool pm;
    
};

int main(int argc, char** argv) {
    
    set_logger(std::make_shared<logger2>());
    
    
    if (argc < 3) {
        logger_message() << "specify operation and file name";
    }
    std::string operation(argv[1]);

    std::string origfn(argv[2]);


    if (!std::ifstream(origfn, std::ios::in | std::ios::binary).good()) {
        logger_message() << "failed to open " << origfn;
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
    size_t max_depth=18;

    bool sort_objs=false;
    bool sortfile=true;
    bool filter_objs=false;
    bbox filter_box{-1800000000,-900000000,1800000000,900000000};
    bool inmem=false;
    std::vector<std::string> changes;
    bool countgeom=false;
    bool splitways=true;
    lonlatvec poly;
    size_t countflags=15;
    bool use_tree=false;
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
                logger_message() << "read timestamp \"" << val << "\" as " << timestamp;
            } else if (key=="numchan=") {
                numchan = atoi(val.c_str());
            } else if (key=="rollup") {
                rollup=true;
                logger_message() << "rollup=true";
            } else if (key=="targetsize=") {
                targetsize = atoi(val.c_str());
                logger_message() << "targetsize=" << targetsize;
            } else if (key=="minsize=") {
                minsize = atoi(val.c_str());
                logger_message() << "minsize=" << minsize;
            } else if (key=="tempfn=") {
                tempfn = val;
                logger_message() << "tempfn=" << tempfn;
            } else if (key=="sort") {
                sort_objs=true;
            } else if (key=="dontsortfile") {
                sortfile=false;
            } else if (key == "filter=") {
                if (val=="planet") {
                    //leave as default
                } else if (val=="nwkent") {
                    filter_box=bbox{750000,512000000,3000000,514000000};
                } else if (EndsWith(val, ".poly")) {
                    poly = read_poly_file(val);
                    filter_box = poly_bounds(poly);
                                        
                } else {
                    filter_box = read_bbox(val);
                }

                logger_message() << "read filter " << val << " as " << filter_box;
            } else if (key=="filterobjs") {
                filter_objs=true;
            } else if (key=="grptiles=") {
                grptiles = atoi(val.c_str());
                logger_message() << "grptiles=" << grptiles;
            
            } else if (key=="inmem") {
                inmem=true;
                logger_message() << "inmem";
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
                logger_message() << "splitways=false";
            } else if (key=="countflags=") {
                countflags = std::stoull(val);  
            } else if (key=="usetree") {
                use_tree=true;
            } else {
               logger_message() << "unrecongisned argument " << arg;
               return 1;
            } 
        }
    }
    if (tempfn.empty()) {
        tempfn = outfn+"-interim";
    }
    
    
    
    
    logger_message() << "numchan=" << numchan;

    //auto lg = make_default_logger();

    int resp=0;
    if (operation == "count") {
        auto res = run_count(origfn, numchan,false, countgeom, countflags);
        logger_message() << "\n"<<res.long_str();
    } else if (operation == "calcqts") {
        if (inmem) {
            run_calcqts_inmem(origfn, qtsfn, numchan, true);
        } else {
            run_calcqts(origfn, qtsfn, numchan, splitways, true, buffer, max_depth);
        }

    } else if (operation=="sortblocks") {
        std::shared_ptr<qttree> groups;
        //if (usefindgroupscopy) {
        
        if (true) {
            auto tree = make_qts_tree_maxlevel(qtsfn=="NONE" ? origfn : qtsfn, numchan, max_depth);
            get_logger()->time("load qts");
            if (rollup) {
                tree_rollup(tree,minsize);
                get_logger()->time("rollup tree");
            }
            if (use_tree) {
                if (max_depth>15) { throw std::domain_error("use_tree with too large a max_depth??"); }
                groups=tree;
            } else {
                groups = find_groups_copy(tree,targetsize,minsize);
                logger_message() << "find groups";
                get_logger()->time("find groups");
                tree.reset();
            }
        //} else {
        //    groups = run_findgroups(qtsfn,numchan,rollup,targetsize, minsize);
        }
        
        checkstats();
        if (inmem) {
            resp = run_sortblocks_inmem(origfn,qtsfn,outfn,timestamp, numchan, groups);
        } else {
            
            resp = run_sortblocks(origfn,qtsfn,outfn,timestamp,  numchan, groups, tempfn, grptiles);
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
        auto bl = readFileBlock(idx, file);
        while (bl) {
            auto dd = decompress(bl->data,bl->uncompressed_size);
            tl+=dd.size();
            idx++;
            bl = readFileBlock(idx, file);
        }
        file.close();
        logger_message() << "uncompressed size: " << tl;
    }
    
            
        
        
    checkstats();
    get_logger()->timing_messages();
    return resp;
}
