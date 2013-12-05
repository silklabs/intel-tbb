/*
    Copyright 2005-2012 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

// type that checks construction and destruction.

static tbb::atomic<int> check_type_counter;

template<class Counter>
class check_type : Harness::NoAfterlife {
    Counter id;
    bool am_ready;
public:

    check_type(int _n = 0) : id(_n), am_ready(false) {
        ++check_type_counter;
    }

    check_type(const check_type& other) : Harness::NoAfterlife(other) {
        other.AssertLive();
        AssertLive();
        id = other.id;
        am_ready = other.am_ready;
        ++check_type_counter;
    }

    operator int() const { return (int)my_id(); }

    ~check_type() { 
        AssertLive(); 
        --check_type_counter;
        ASSERT(check_type_counter >= 0, "too many destructions");
    }

    check_type &operator=(const check_type &other) {
        other.AssertLive();
        AssertLive();
        id = other.id;
        am_ready = other.am_ready;
        return *this;
    }

    unsigned int my_id() const { AssertLive(); return id; }
    bool is_ready() { AssertLive(); return am_ready; }
    void function() {
        AssertLive();
        if( id == 0 ) {
            id = 1;
            am_ready = true;
        }
    }

};

// provide a default check method that just returns true.
template<class MyClass>
struct Check {
    static bool check_destructions() {return true; }
};

template<class Counttype>
class Check<check_type< Counttype > > {
    static bool check_destructions () { 
            return check_type_counter == 0;
        }
};

