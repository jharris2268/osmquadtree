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

#ifndef UTILS_MULTITHREADEDCALLBACK_HPP
#define UTILS_MULTITHREADEDCALLBACK_HPP

#include "oqt/common.hpp"
#include "oqt/utils/singlequeue.hpp"

#include <future>
#include <thread>

#include <iostream>
namespace oqt {
template <class T>
class multi_threaded_callback {
    public:
        typedef std::function<void(std::shared_ptr<T>)> callback_func;
        multi_threaded_callback(callback_func func, size_t numchan) :
            queues(numchan), rem(numchan), error(false) {

            for (size_t i=0; i < numchan; i++) {
                queues[i] = std::make_shared<single_queue<T>>();
            }
            fut = std::async(std::launch::async, add_all_queue, queues, func, std::ref(error));
        }
        virtual ~multi_threaded_callback() {}

        static std::vector<callback_func> make(callback_func func, size_t numchan) {
            auto mtc = std::make_shared<multi_threaded_callback<T>>(func, numchan);
            std::vector<callback_func> result;
            for (size_t i=0; i < numchan; i++) {
                result.push_back([mtc,i](std::shared_ptr<T> bl) { mtc->call(i, bl); });
            }
            return result;
        }



    private:
        static std::exception_ptr add_all_queue(std::vector<std::shared_ptr<single_queue<T>>> queues, callback_func func, std::atomic<bool>& error) {
            size_t ii=0;
            for (auto fb = queues[ii%queues.size()]->wait_and_pop(); fb; fb=queues[ii%queues.size()]->wait_and_pop()) {
                try {
                    func(fb);
                    ii++;
                } catch (...) {
                    std::cout << "callback failed [mtc ii=" << ii << "]" << std::endl;
                    error=true;
                    for (auto& q : queues) {
                        //q->set_exception(std::current_exception());
                        q->cancel();
                    }
                    return std::current_exception();
                    
                }
            }
            func(std::shared_ptr<T>());
            return std::exception_ptr();
        }
        void call(size_t which, std::shared_ptr<T> fb) {
            if (error) {
                if (!except) {
                    
                    std::cout << "have error [mtc " << which << "]" << std::endl;
                    fut.wait();
                    //error=false;
                    except = fut.get();
                }
                
                std::rethrow_exception(except);
            }               
            
            try {
                if (fb) {
                    queues[which]->wait_and_push(fb);
                        
                } else {
                    queues[which]->wait_and_finish();
                    rem--;
                    if (rem<=0) {
                        fut.wait();
                    }
                }
            } catch (...) {
                if (error) {
                    if (!except) {
                        std::cout << "queue failed//have error [mtc " << which << "]" << std::endl;
                        fut.wait();
                        //error=false;
                        except = fut.get();
                    }
                    std::rethrow_exception(except);
                }
                except = std::current_exception();
                std::rethrow_exception(except);
            }
        }


        std::vector<std::shared_ptr<single_queue<T>>> queues;
        std::future<std::exception_ptr> fut;
        std::exception_ptr except;
        std::atomic<int> rem;
        std::atomic<bool> error;
};
}
#endif
