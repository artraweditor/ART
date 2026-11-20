/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2017 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "refreshmap.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace art { namespace engine {


class ProcEventMapper {
public:
    static ProcEventMapper *getInstance();
    art::engine::ProcEvent newEvent(int action,
                                 const std::string &history_msg = "");
    art::engine::ProcEvent newAnonEvent(int action);
    std::string getHistoryMsg(const art::engine::ProcEvent &event) const;

private:
    ProcEventMapper();

    std::unordered_set<std::string> history_msgs_;
    std::unordered_map<int, const char *> msgmap_;
};


} } // namespace art::engine
