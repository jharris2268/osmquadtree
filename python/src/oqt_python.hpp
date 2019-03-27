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


#include "oqt/elements/block.hpp"
#include "oqt/utils/logger.hpp"

#include <algorithm>
#include <memory>
#include <tuple>


namespace py = pybind11;




void calcqts_defs(py::module& m);
void count_defs(py::module& m);
void elements_defs(py::module& m);
void geometry_defs(py::module& m);
void pbfformat_defs(py::module& m);
void postgis_defs(py::module& m);
void sorting_defs(py::module& m);
void update_defs(py::module& m);
void utils_defs(py::module& m);



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
