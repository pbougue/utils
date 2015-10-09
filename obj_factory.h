/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#pragma once
#include <vector>
#include <unordered_map>
#include "utils/functions.h"
#include "utils/idx_map.h"

namespace navitia {

/*
 * Factory handling all collections for one object
 * Object must inherit from navitia::type::Header
 */
template<typename ObjType>
class ObjFactory {
private:
    typedef typename std::vector<std::unique_ptr<ObjType>> inner_vector;
    typedef typename std::unordered_map<std::string, ObjType*> inner_map;

    inner_vector vec;
    inner_map map;

public:
    typedef typename inner_vector::iterator iterator;
    typedef typename inner_vector::const_iterator const_iterator;

    template<typename ...Args>
    ObjType* emplace(const std::string& uri, Args&& ...args) {
        if (navitia::contains(map, uri)) {
            throw(std::logic_error(std::string("In ") + typeid(*this).name() +
                                   ": Object with same uri (" + uri + ") already stored."));
        }
        vec.push_back(std::make_unique<ObjType>(std::forward<Args>(args)...));
        ObjType* obj = vec.back().get();
        obj->idx = vec.size() - 1;
        obj->uri = uri;
        map[obj->uri] = obj;
        return obj;
    }

    template<typename ...Args>
    ObjType* get_or_create(const std::string& uri, Args&& ...args) {
        ObjType* obj = get_mut(uri);
        if (obj == nullptr) {
            obj = emplace(uri, args...);
        }
        return obj;
    }

    ObjType* insert(const std::string& uri, ObjType&& obj) {
        return emplace(uri, obj);
    }

    const ObjType* operator[] (const std::string& uri) const {
        const auto obj_it = map.find(uri);
        if (obj_it == std::end(map)) {
            return nullptr;
        }
        return obj_it->second;
    }

    const ObjType* operator[] (const Idx<ObjType>& idx) const {
        if (idx.val >= vec.size()) {
            return nullptr;
        }
        return vec[idx.val];
    }

    ObjType* get_mut(const std::string& uri) {
        auto obj_it = map.find(uri);
        if (obj_it == std::end(map)) {
            return nullptr;
        }
        return obj_it->second;
    }

    ObjType* get_mut(const Idx<ObjType>& idx) {
        if (idx.val >= vec.size()) {
            return nullptr;
        }
        return vec[idx.val];
    }

    iterator begin() { return std::begin(vec); }
    const_iterator begin() const { return std::begin(vec); }
    iterator end() { return std::end(vec); }
    const_iterator end() const { return std::end(vec); }

    size_t size() const noexcept { return vec.size(); }

    template<class Archive> void serialize(Archive & ar, const unsigned int) { ar & vec & map; }
};

} // namespace navitia
