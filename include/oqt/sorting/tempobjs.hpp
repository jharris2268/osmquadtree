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

#ifndef SORTING_TEMPOBJS_HPP
#define SORTING_TEMPOBJS_HPP

#include "oqt/sorting/common.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"

namespace oqt {
class TempObjs {
    public:
        virtual primitiveblock_callback add_func(size_t ch)=0;
        virtual void finish()=0;
        virtual void read(std::vector<primitiveblock_callback> outs)=0;
        virtual ~TempObjs() {}
};

typedef std::function<void(std::shared_ptr<KeyedBlob>)> keyedblob_callback;

class BlobStore {
    public:
        virtual void add(keystring_ptr)=0;
        virtual void read(std::vector<keyedblob_callback> outs)=0;
        virtual ~BlobStore() {}
};

std::shared_ptr<BlobStore> make_blobstore_file(std::string tempfn, bool sortfile);
std::shared_ptr<BlobStore> make_blobstore_filesplit(std::string tempfn, int64 splitat);


std::shared_ptr<TempObjs> make_tempobjs(std::shared_ptr<BlobStore> blobstore, size_t numchan);

}
#endif //SORTING_TEMPOBJS_HPP
