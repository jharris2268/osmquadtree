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

#ifndef CALCQTS_QTTREEGROUPS_HPP
#define CALCQTS_QTTREEGROUPS_HPP

#include "oqt/calcqts/qttree.hpp"

namespace oqt {
std::shared_ptr<qttree> find_groups_copy(std::shared_ptr<qttree> tree, int64 target, int64 minsize);
void tree_rollup(std::shared_ptr<qttree> tree, int64 minsize);
std::shared_ptr<qttree> tree_round_copy(std::shared_ptr<qttree> tree, int64 maxlevel);

}
#endif
