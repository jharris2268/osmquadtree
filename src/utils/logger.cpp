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

#include "oqt/utils/logger.hpp"
#include <iostream>
#include <iomanip>

namespace oqt {






class logger_impl : public logger {
    public:
        logger_impl() : msgw(0) {}

        void message(const std::string& msg) {
            
            if (msgw!=0) {
                std::cout << std::endl;
            }
            std::cout << msg << std::endl;
            msgw=0;
        }
        void progress(double percent, const std::string& msg) {
            if (msg.size()>msgw) { msgw=msg.size(); }
            std::cout << "\r" << std::fixed << std::setprecision(1) << std::setw(5) << percent << " " << std::setw(msgw) << msg << std::flush;
        }

        ~logger_impl() {
            if (msgw!=0) {
                std::cout << std::endl;
            }
        }
    private:
        size_t msgw;
};

std::shared_ptr<logger> make_default_logger() { return std::make_shared<logger_impl>(); }


std::shared_ptr<logger> logger_;


std::shared_ptr<logger> get_logger() {
    if (!logger_) {
        logger_ = make_default_logger();
    }
    return logger_;
}

void set_logger(std::shared_ptr<logger> nlog) {
    logger_ = nlog;
}
    
    
}
