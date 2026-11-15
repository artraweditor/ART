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

#include "wire.h"
#include <cstdlib>

namespace art { namespace engine { namespace remote { namespace wire {

namespace {

// a header line, without its newline (at most 40 bytes long)
bool read_line(ipc::Connection &c, std::string &line)
{
    line.clear();
    char ch;
    while (line.size() < 40) {
        if (!c.read(&ch, 1)) {
            return false;
        }
        if (ch == '\n') {
            return true;
        }
        line.push_back(ch);
    }
    return false;
}


// "<tag> <number>" -> number
bool parse_header(const std::string &line, const std::string &tag, size_t &n)
{
    if (line.size() < tag.size() + 2 || line.compare(0, tag.size(), tag) != 0 ||
        line[tag.size()] != ' ') {
        return false;
    }
    const std::string num = line.substr(tag.size() + 1);
    if (num.find_first_not_of("0123456789") != std::string::npos ||
        num.size() > 12) {
        return false;
    }
    n = size_t(strtoull(num.c_str(), nullptr, 10));
    return true;
}

} // namespace


bool writeRequest(ipc::Connection &c, const Request &r)
{
    std::string payload = r.cwd;
    payload.push_back('\0');
    payload += r.mark ? "1" : "0";
    payload.push_back('\0');
    for (auto &a : r.args) {
        payload += a;
        payload.push_back('\0');
    }
    if (payload.size() > MAX_REQUEST) {
        return false;
    }
    const std::string head = "ART1 " + std::to_string(payload.size()) + "\n";
    return c.write(head.data(), head.size()) &&
           c.write(payload.data(), payload.size());
}


bool readRequest(ipc::Connection &c, Request &r, std::string &err)
{
    std::string line;
    size_t n = 0;
    if (!read_line(c, line)) {
        err = "no request received";
        return false;
    }
    if (!parse_header(line, "ART1", n)) {
        err = "unsupported protocol";
        return false;
    }
    if (n > MAX_REQUEST) {
        err = "request too large";
        return false;
    }
    std::string payload(n, '\0');
    if (n && !c.read(&payload[0], n)) {
        err = "incomplete request";
        return false;
    }
    std::vector<std::string> fields;
    size_t start = 0;
    while (start < payload.size()) {
        size_t end = payload.find('\0', start);
        if (end == std::string::npos) {
            err = "malformed request";
            return false;
        }
        fields.push_back(payload.substr(start, end - start));
        start = end + 1;
    }
    if (fields.size() < 2 || (fields[1] != "0" && fields[1] != "1")) {
        err = "malformed request";
        return false;
    }
    r.cwd = fields[0];
    r.mark = fields[1] == "1";
    r.args.assign(fields.begin() + 2, fields.end());
    return true;
}


bool writeFrame(ipc::Connection &c, char kind, const std::string &data)
{
    if (data.empty()) {
        return true;
    }
    const std::string head =
        std::string(1, kind) + " " + std::to_string(data.size()) + "\n";
    return c.write(head.data(), head.size()) &&
           c.write(data.data(), data.size());
}


bool writeStatus(ipc::Connection &c, int status)
{
    const std::string s = "X " + std::to_string(status) + "\n";
    return c.write(s.data(), s.size());
}


bool readResponse(ipc::Connection &c,
                  const std::function<void(char, const std::string &)> &onData,
                  int &status, std::string &err)
{
    std::string line;
    while (true) {
        if (!read_line(c, line) || line.empty()) {
            err = "the connection to the server was lost";
            return false;
        }
        const char kind = line[0];
        if (kind == 'X') {
            size_t n = 0;
            if (!parse_header(line, "X", n)) {
                err = "protocol error";
                return false;
            }
            status = int(n);
            return true;
        }
        size_t n = 0;
        if ((kind != 'O' && kind != 'E') ||
            !parse_header(line, std::string(1, kind), n) || n > MAX_FRAME) {
            err = "protocol error";
            return false;
        }
        std::string data(n, '\0');
        if (n && !c.read(&data[0], n)) {
            err = "the connection to the server was lost";
            return false;
        }
        onData(kind, data);
    }
}

}}}} // namespace art::engine::remote::wire
