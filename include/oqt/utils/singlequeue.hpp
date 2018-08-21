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

#ifndef UTILS_SINGLEQUEUE_HPP
#define UTILS_SINGLEQUEUE_HPP

#include <mutex>
#include <condition_variable>
#include <exception>
#include <atomic>
#include <iostream>

namespace oqt {

template<class T>
class single_queue {
    std::shared_ptr<T> obj;
    
    std::atomic<int> pending_writers;
    std::mutex wmut;
    
    std::condition_variable ready_to_read;
    std::condition_variable ready_to_write;
    std::exception_ptr ex;

    public:
        single_queue() : pending_writers(1) {};
        single_queue(size_t num_writers_) : pending_writers(num_writers_) {}

        bool valid() {
            return pending_writers>0;
        }

        void wait_and_push(std::shared_ptr<T> newval) {
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_write.wait(lk,[this]()->bool{return !obj;});
            if (!valid()) {
                if (ex) {
                    std::rethrow_exception(ex);
                } else {
                    throw std::domain_error("queue finished");
                }
            }
            obj = newval;
            ready_to_read.notify_all();
        }

        void wait_and_finish() {
            if (pending_writers>1) {
                pending_writers--;
                return;
            }
            
            
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_write.wait(lk,[this]()->bool{return !obj;});
            if (pending_writers>0) { pending_writers--; }
            
            ready_to_read.notify_one();
        }

        std::shared_ptr<T> wait_and_pop() {
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_read.wait(lk,[this]()->bool{return (pending_writers==0) || obj;});
            std::shared_ptr<T> ans=obj;
            obj.reset();
            ready_to_write.notify_one();
            return ans;
        }

        void cancel() {
            pending_writers=0;
            obj.reset();
            ready_to_write.notify_one();

        }
        void set_exception(std::exception_ptr ex_) {
            std::cout << "received exception" << std::endl;
            ex = ex_;

            ready_to_write.notify_one();
        }
        
        
        
};



template<class T>
class single_queue_unique {
    std::unique_ptr<T> obj;
    
    std::atomic<int> pending_writers;
    std::mutex wmut;
    
    std::condition_variable ready_to_read;
    std::condition_variable ready_to_write;
    std::exception_ptr ex;

    public:
        single_queue_unique() : pending_writers(1) {};
        single_queue_unique(size_t num_writers_) : pending_writers(num_writers_) {}

        bool valid() {
            return pending_writers>0;
        }

        void wait_and_push(std::unique_ptr<T> newval) {
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_write.wait(lk,[this]()->bool{return !obj;});
            if (!valid()) {
                if (ex) {
                    std::rethrow_exception(ex);
                } else {
                    throw std::domain_error("queue finished");
                }
            }
            obj = std::move(newval);
            ready_to_read.notify_all();
        }

        void wait_and_finish() {
            if (pending_writers>1) {
                pending_writers--;
                return;
            }
            
            
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_write.wait(lk,[this]()->bool{return !obj;});
            if (pending_writers>0) { pending_writers--; }
            
            ready_to_read.notify_one();
        }

        std::unique_ptr<T> wait_and_pop() {
            std::unique_lock<std::mutex> lk(wmut);
            ready_to_read.wait(lk,[this]()->bool{return (pending_writers==0) || obj;});
            std::unique_ptr<T> ans=std::move(obj);
            obj.reset();
            ready_to_write.notify_one();
            return std::move(ans);
        }

        void cancel() {
            pending_writers=0;
            obj.reset();
            ready_to_write.notify_one();

        }
        void set_exception(std::exception_ptr ex_) {
            std::cout << "received exception" << std::endl;
            ex = ex_;

            ready_to_write.notify_one();
        }
};
}

#endif
