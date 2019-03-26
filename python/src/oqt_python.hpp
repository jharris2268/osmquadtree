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

#ifndef OQT_PYTHON_HPP
#define OQT_PYTHON_HPP

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>


#include "oqt/count.hpp"
#include "oqt/sorting/qttreegroups.hpp"
#include "oqt/elements/combineblocks.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/sorting/tempobjs.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/logger.hpp"

#include "oqt/update/xmlchange.hpp"
#include "gzstream.hpp"
#include "oqt/update/update.hpp"

#include "oqt/calcqts/calcqts.hpp"
#include "oqt/sorting/mergechanges.hpp"
#include "oqt/sorting/sortblocks.hpp"

#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/geometry/addwaynodes.hpp"
#include "oqt/geometry/makegeometries.hpp"
#include "oqt/geometry/addparenttags.hpp"
#include "oqt/geometry/handlerelations.hpp"
#include "oqt/geometry/multipolygons.hpp"
#include "oqt/geometry/postgiswriter.hpp"
#include <algorithm>
#include <memory>
#include <tuple>


namespace py = pybind11;

template <class RetType, class ArgType>
std::function<RetType(ArgType)> wrap_callback(std::function<RetType(ArgType)> callback) {
    if (!callback) {
        return nullptr;
    }
    return [callback](ArgType arg) {
        py::gil_scoped_acquire aq;
        return callback(arg);
    };
}

//void read_defs(py::module& m);
void block_defs(py::module& m);
void core_defs(py::module& m);
void change_defs(py::module& m);
void geometry_defs(py::module& m);
void postgis_defs(py::module& m);
void utils_defs(py::module& m);


template <class BlockType>
class collect_blocks {
    public:
        collect_blocks(
            std::function<bool(std::vector<std::shared_ptr<BlockType>>)> callback_,
            size_t numblocks_) : callback(callback_), numblocks(numblocks_), tot(0) {}

        void call(std::shared_ptr<BlockType> block) {
            
            if (block) {
                pending.push_back(block);
            }
            if ((pending.size() == numblocks) || (!block && !pending.empty())) {
                std::vector<std::shared_ptr<BlockType>> tosend;
                tosend.swap(pending);
                tot+=tosend.size();
                
                if (!callback) {
                    oqt::Logger::Message() << "None(" << tosend.size() << " blocks" << ")";
                } else {
                        
                    bool r = callback(tosend);
                    if (!r) {
                        throw std::domain_error("python callback failed");
                    }
                }

            }
        }

        size_t total() { return tot; }

    private:
        std::function<bool(std::vector<std::shared_ptr<BlockType>>)> callback;
        size_t numblocks;
        size_t tot;
        std::vector<std::shared_ptr<BlockType>> pending;
};

typedef std::function<bool(std::vector<oqt::PrimitiveBlockPtr>)> external_callback;
typedef std::function<void(oqt::PrimitiveBlockPtr)> block_callback;

inline block_callback prep_callback(external_callback cb, size_t numblocks) {
    if (!cb) { return nullptr; }
    
    auto collect = std::make_shared<collect_blocks<oqt::PrimitiveBlock>>(wrap_callback(cb),numblocks);
    return [collect](oqt::PrimitiveBlockPtr bl) { collect->call(bl); };
}

#endif
