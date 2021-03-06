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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace navitia {

// Encapsulate a unary function, and provide a least recently used
// cache.  The function must be pure (same argument => same result),
// and a Lru object must not be shared across threads.
template<typename F>
class Lru {
private:
    typedef typename boost::remove_cv<typename boost::remove_reference<
                                          typename F::argument_type>::type
                                      >::type key_type;
    typedef typename boost::remove_cv<typename boost::remove_reference<
                                          typename F::result_type>::type
                                      >::type mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef boost::multi_index_container<
        value_type,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::member<value_type, const key_type, &value_type::first> > >
        > Cache;

    // the encapsulate function
    F f;

    // maximal cached values
    size_t max_cache;

    // the cache, mutable because side effect are not visible from the
    // exterior because of the purity of f
    mutable Cache cache;

public:
    typedef mapped_type const& result_type;
    typedef typename F::argument_type argument_type;

    Lru(F fun, size_t max = 10): f(std::move(fun)), max_cache(max) {}

    result_type operator()(argument_type arg) const {
        auto& list = cache.template get<0>();
        auto& map = cache.template get<1>();
        const auto search = map.find(arg);
        if (search != map.end()) {
            // put the cached value at the begining of the cache
            list.relocate(list.begin(), cache.template project<0>(search));
            return search->second;
        } else {
            // insert the new value at the begining of the cache
            const auto ins = list.push_front(std::make_pair(arg, f(arg)));
            // clean the cache by the end (where the entries are the
            // older ones) until the requested size
            while (list.size() > max_cache) { list.pop_back(); }
            return ins.first->second;
        }
    }
};
template<typename F> inline Lru<F> make_lru(const F& fun, size_t max = 10) {
    return Lru<F>(fun, max);
}

} // namespace navitia
