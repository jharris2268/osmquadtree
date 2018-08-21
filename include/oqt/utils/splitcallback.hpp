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

#ifndef UTILS_SPLITCALLBACK_HPP
#define UTILS_SPLITCALLBACK_HPP

#include "oqt/common.hpp"
namespace oqt {

template <class T>
class split_callback {
    public:
        typedef std::function<void(std::shared_ptr<T>)> callback_func;
        split_callback(std::vector<callback_func> funcs_) : funcs(funcs_), index(0) {}
        
        void call(std::shared_ptr<T> block) {
            if (!block) {
                for (auto& f: funcs) {
                    f(block);
                }
                return;
            }
            funcs[index%funcs.size()](block);
            index++;
        }
        
        static callback_func make(std::vector<callback_func> funcs) {
            auto sc = std::make_shared<split_callback<T>>(funcs);
            return [sc](std::shared_ptr<T> block) { sc->call(block); };
        }
    private:
        std::vector<callback_func> funcs;
        size_t index;
};
}

#endif
