/* -*- C++ -*-
 *
 *  This file is part of ART.
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

// A JSON value for the replies of the command mode: built by the verbs and
// written out as one compact line. A thin owning wrapper over cJSON (which the
// engine already uses), with value semantics and strings made valid UTF-8.
// There is no parser (the clients are scripts).

#include "../cJSON.h"
#include <glib.h>
#include <glibmm/ustring.h>
#include <string>
#include <utility>

namespace art { namespace engine { namespace remote { namespace json {

class Value {
public:
    Value(): p_(cJSON_CreateNull()) {}
    Value(bool b): p_(cJSON_CreateBool(b)) {}
    Value(int i): p_(cJSON_CreateNumber(i)) {}
    Value(long i): p_(cJSON_CreateNumber(static_cast<double>(i))) {}
    Value(long long i): p_(cJSON_CreateNumber(static_cast<double>(i))) {}
    Value(unsigned long i): p_(cJSON_CreateNumber(static_cast<double>(i))) {}
    // NaN and infinity are written as null (JSON has neither)
    Value(double d): p_(cJSON_CreateNumber(d)) {}
    Value(const char *s): p_(make_string(s ? s : "")) {}
    Value(const std::string &s): p_(make_string(s)) {}
    Value(const Glib::ustring &s): p_(make_string(s.raw())) {}

    Value(const Value &o): p_(cJSON_Duplicate(o.p_, true)) {}
    Value(Value &&o) noexcept: p_(o.p_) { o.p_ = nullptr; }
    Value &operator=(Value o) noexcept
    {
        std::swap(p_, o.p_);
        return *this;
    }
    ~Value() { cJSON_Delete(p_); }

    static Value array() { return Value(cJSON_CreateArray()); }
    static Value object() { return Value(cJSON_CreateObject()); }

    // append to an array
    Value &push(Value v)
    {
        cJSON_AddItemToArray(p_, v.release());
        return *this;
    }
    // add a member to an object (members keep their insertion order)
    Value &set(const std::string &key, Value v)
    {
        gchar *k = g_utf8_make_valid(key.c_str(), key.size());
        cJSON_AddItemToObject(p_, k, v.release());
        g_free(k);
        return *this;
    }

    // the value as compact JSON (UTF-8, a single line)
    std::string dump() const
    {
        char *t = cJSON_PrintUnformatted(p_);
        std::string ret = t ? t : "null";
        cJSON_free(t);
        return ret;
    }

private:
    explicit Value(cJSON *p): p_(p) {}

    // JSON text must be valid UTF-8, which cJSON does not check
    static cJSON *make_string(const std::string &s)
    {
        gchar *valid = g_utf8_make_valid(s.c_str(), s.size());
        cJSON *ret = cJSON_CreateString(valid);
        g_free(valid);
        return ret;
    }

    cJSON *release()
    {
        cJSON *p = p_;
        p_ = nullptr;
        return p;
    }

    cJSON *p_;
};

}}}} // namespace art::engine::remote::json
