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

#ifndef GEOMETRY_FINDMINZOOM_HPP
#define GEOMETRY_FINDMINZOOM_HPP

#include "oqt/elements/element.hpp"

namespace oqt {
namespace geometry {


class FindMinZoom {
    public:
        virtual int64 calculate(ElementPtr ele)=0;
        virtual ~FindMinZoom() {}
};


double res_zoom(double res);

typedef std::vector<std::tuple<int64,std::string,std::string,int64>> tag_spec;
std::shared_ptr<FindMinZoom> make_findminzoom_onetag(const tag_spec& spec, double minlen, double minarea);

}
}

#endif
