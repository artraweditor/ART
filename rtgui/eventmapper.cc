/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2017 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eventmapper.h"

ProcEventMapper::ProcEventMapper()
{
    for (int event = 0; event < rtengine::NUMOFEVENTS; ++event) {
        auto it =
            history_msgs_.insert("HISTORY_MSG_" + std::to_string(event + 1));
        msgmap_[event] = it.first->c_str();
    }
}

ProcEventMapper *ProcEventMapper::getInstance()
{
    static ProcEventMapper instance;
    return &instance;
}

rtengine::ProcEvent ProcEventMapper::newEvent(int action,
                                              const std::string &history_msg)
{
    auto event = rtengine::RefreshMapper::getInstance()->newEvent();
    rtengine::RefreshMapper::getInstance()->mapEvent(event, action);

    auto it =
        history_msg.empty()
            ? history_msgs_.insert("HISTORY_MSG_" + std::to_string(event + 1))
            : history_msgs_.insert(history_msg);
    event.set_message(it.first->c_str());

    return event;
}

rtengine::ProcEvent ProcEventMapper::newAnonEvent(int action)
{
    auto event = rtengine::RefreshMapper::getInstance()->newEvent();
    rtengine::RefreshMapper::getInstance()->mapEvent(event, action);

    auto it = history_msgs_.insert("");
    event.set_message(it.first->c_str());

    return event;
}

std::string
ProcEventMapper::getHistoryMsg(const rtengine::ProcEvent &event) const
{
    static std::string empty;
    auto msg = event.get_message();
    if (msg) {
        return std::string(msg);
    } else {
        auto it = msgmap_.find(event);
        if (it == msgmap_.end()) {
            return empty;
        } else {
            return it->second;
        }
    }
}
