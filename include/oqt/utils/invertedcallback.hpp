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

#ifndef UTILS_INVERTEDCALLBACK_HPP
#define UTILS_INVERTEDCALLBACK_HPP

#include "oqt/common.hpp"
#include "oqt/utils/singlequeue.hpp"

#include <future>
#include <thread>
namespace oqt {


template <class T>
class inverted_callback {
    public:
        typedef std::function<void(std::function<void(std::shared_ptr<T>)>)> caller_function;
        typedef std::function<std::shared_ptr<T>(void)> inverted_function;
    
        inverted_callback(caller_function caller) {
            
            queue = std::make_shared<single_queue<T>>();
            auto this_queue=queue;
            fut = std::async(std::launch::async, [caller,this_queue]() {
                caller([this_queue](std::shared_ptr<T> bl) {
                    if (bl) {
                        this_queue->wait_and_push(bl);
                    } else {
                        this_queue->wait_and_finish();
                    }
                });
                
            }
            );
        }
        void cancel() {
            queue->cancel();
            fut.wait();
        }
        
        ~inverted_callback() {
            cancel();
        }
        
        std::shared_ptr<T> next() {
            return queue->wait_and_pop();
        }
        
        static inverted_function make(caller_function func) {
            auto ic = std::make_shared<inverted_callback<T>>(func);
            return [ic]() { return ic->next(); };
        }
    private:
        std::shared_ptr<single_queue<T>> queue;
        std::future<void> fut;
};
}
            

#endif
