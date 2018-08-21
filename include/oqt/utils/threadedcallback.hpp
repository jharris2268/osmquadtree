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

#ifndef UTILS_THREADEDCALLBACK_HPP
#define UTILS_THREADEDCALLBACK_HPP

#include "oqt/common.hpp"
#include "oqt/utils/singlequeue.hpp"

#include <future>
#include <thread>

#include <iostream>

namespace oqt {

template <class T>
class threaded_callback {
    public:
        typedef std::function<void(std::shared_ptr<T>)> callback_func;
        threaded_callback(callback_func func) :

            queue(std::make_shared<single_queue<T>>()), error(false), rem(1) {
            
            fut = std::async(std::launch::async, add_all_queue, queue, func, std::ref(error));

        }
        
        threaded_callback(callback_func func, size_t numchan) :

            queue(std::make_shared<single_queue<T>>()), error(false), rem(numchan) {
            
            fut = std::async(std::launch::async, add_all_queue, queue, func, std::ref(error));

        }



        static callback_func make(callback_func func) {
            auto tc = std::make_shared<threaded_callback<T>>(func);
            return [tc](std::shared_ptr<T> b) { tc->call(b); };
        }
        static callback_func make(callback_func func, size_t numchan) {
            auto tc = std::make_shared<threaded_callback<T>>(func,numchan);
            return [tc](std::shared_ptr<T> b) { tc->call(b); };
        }


    private:
        static std::exception_ptr add_all_queue(std::shared_ptr<single_queue<T>> queue, std::function<void(std::shared_ptr<T>)> func, std::atomic<bool>& error) {
            for (auto fb = queue->wait_and_pop(); fb; fb=queue->wait_and_pop()) {
                try {
                    func(fb);
                } catch (...) {
                    std::cout << "callback failed [tc]" << std::endl;
                    error=true;
                    //queue->set_exception(std::current_exception());
                    queue->cancel();
                    //func(std::shared_ptr<T>());
                    return std::current_exception();
                }
            }
            func(std::shared_ptr<T>());
            return std::exception_ptr();
        }

        void call(std::shared_ptr<T> fb) {
            
            if (error) {
                std::cout << "have error [tc]" << std::endl;
                fut.wait();
                auto except = fut.get();
                std::rethrow_exception(except);
                return;
            } 
            
            try {
                if (fb) {
                    queue->wait_and_push(fb);
                } else {
                    rem--;
                    if (rem<=0) {
                        queue->wait_and_finish();
                        fut.wait();
                    }
                }
            } catch (...) {
                if (error) {
                    std::cout << "queue failed//have error [tc]" << std::endl;
                    fut.wait();
                    auto except = fut.get();
                    std::rethrow_exception(except);
                }
                
                auto except = std::current_exception();
                std::rethrow_exception(except);
            }
            
        }


        std::shared_ptr<single_queue<T>> queue;
        std::future<std::exception_ptr> fut;
        std::atomic<bool> error;
        std::atomic<int> rem;

};




template <class T>
class threaded_callback_unique {
    public:
        typedef std::function<void(std::unique_ptr<T>)> callback_func;
        threaded_callback_unique(callback_func func) :

            queue(std::make_shared<single_queue_unique<T>>()), error(false), rem(1) {
            
            fut = std::async(std::launch::async, add_all_queue, queue, func, std::ref(error));

        }
        
        threaded_callback_unique(callback_func func, size_t numchan) :

            queue(std::make_shared<single_queue_unique<T>>()), error(false), rem(numchan) {
            
            fut = std::async(std::launch::async, add_all_queue, queue, func, std::ref(error));

        }



        static callback_func make(callback_func func) {
            auto tc = std::make_shared<threaded_callback_unique<T>>(func);
            return [tc](std::unique_ptr<T>&& b) { tc->call(std::move(b)); };
        }
        static callback_func make(callback_func func, size_t numchan) {
            auto tc = std::make_shared<threaded_callback_unique<T>>(func,numchan);
            return [tc](std::shared_ptr<T>&& b) { tc->call(std::move(b)); };
        }


    private:
        static std::exception_ptr add_all_queue(std::shared_ptr<single_queue_unique<T>> queue, callback_func func, std::atomic<bool>& error) {
            for (auto fb = std::move(queue->wait_and_pop()); fb; fb=std::move(queue->wait_and_pop())) {
                try {
                    func(std::move(fb));
                } catch (...) {
                    std::cout << "callback failed [tc]" << std::endl;
                    error=true;
                    //queue->set_exception(std::current_exception());
                    queue->cancel();
                    //func(std::shared_ptr<T>());
                    return std::current_exception();
                }
            }
            func(nullptr);
            return std::exception_ptr();
        }

        void call(std::unique_ptr<T>&& fb) {
            
            if (error) {
                std::cout << "have error [tc]" << std::endl;
                fut.wait();
                auto except = fut.get();
                std::rethrow_exception(except);
                return;
            } 
            
            try {
                if (fb) {
                    queue->wait_and_push(std::move(fb));
                } else {
                    rem--;
                    if (rem<=0) {
                        queue->wait_and_finish();
                        fut.wait();
                    }
                }
            } catch (...) {
                if (error) {
                    std::cout << "queue failed//have error [tc]" << std::endl;
                    fut.wait();
                    auto except = fut.get();
                    std::rethrow_exception(except);
                }
                
                auto except = std::current_exception();
                std::rethrow_exception(except);
            }
            
        }


        std::shared_ptr<single_queue_unique<T>> queue;
        std::future<std::exception_ptr> fut;
        std::atomic<bool> error;
        std::atomic<int> rem;

};
}
#endif
