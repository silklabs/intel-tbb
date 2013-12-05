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

#if __TBB_CPF_BUILD
// undefine __TBB_CPF_BUILD to simulate user's setup
#undef __TBB_CPF_BUILD

#define TBB_PREVIEW_TASK_ARENA 1

#include "tbb/task_arena.h"
#include "tbb/task_scheduler_observer.h"
#include "tbb/task_scheduler_init.h"
#include <cstdlib>
#include "harness_assert.h"

#include <cstdio>

#include "harness.h"
#include "harness_barrier.h"
#include "harness_concurrency_tracker.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"

#if _MSC_VER
// plays around __TBB_NO_IMPLICIT_LINKAGE. __TBB_LIB_NAME should be defined (in makefiles)
    #pragma comment(lib, __TBB_STRING(__TBB_LIB_NAME))
#endif

typedef tbb::blocked_range<int> Range;

Harness::SpinBarrier our_barrier;

static tbb::enumerable_thread_specific<int> local_id, old_id, slot_id(-1);
void ResetTLS() {
    local_id.clear();
    old_id.clear();
    slot_id.clear();
}

class ConcurrencyTrackingBody {
public:
    void operator() ( const Range& ) const {
        ASSERT(slot_id.local() == tbb::task_arena::current_slot(), NULL);
        Harness::ConcurrencyTracker ct;
        for ( volatile int i = 0; i < 100000; ++i )
            ;
    }
};

class ArenaObserver : public tbb::task_scheduler_observer {
    int myId;
    tbb::atomic<int> myTrappedSlot;
    /*override*/
    void on_scheduler_entry( bool is_worker ) {
        REMARK("a %s #%p is entering arena %d from %d\n", is_worker?"worker":"master", &local_id.local(), myId, local_id.local());
        ASSERT(!old_id.local(), "double-call to on_scheduler_entry");
        old_id.local() = local_id.local();
        ASSERT(old_id.local() != myId, "double-entry to the same arena");
        local_id.local() = myId;
        slot_id.local() = tbb::task_arena::current_slot();
        if(is_worker) ASSERT(tbb::task_arena::current_slot()>0, NULL);
        else ASSERT(tbb::task_arena::current_slot()==0, NULL);
    }
    /*override*/
    void on_scheduler_exit( bool is_worker ) {
        REMARK("a %s #%p is leaving arena %d to %d\n", is_worker?"worker":"master", &local_id.local(), myId, old_id.local());
        //ASSERT(old_id.local(), "call to on_scheduler_exit without prior entry");
        ASSERT(local_id.local() == myId, "nesting of arenas is broken");
        ASSERT(slot_id.local() == tbb::task_arena::current_slot(), NULL);
        slot_id.local() = -1;
        local_id.local() = old_id.local();
        old_id.local() = 0;
    }
    /*override*/
    bool on_scheduler_leaving() {
        ASSERT(slot_id.local() == tbb::task_arena::current_slot(), NULL);
        return tbb::task_arena::current_slot() >= myTrappedSlot;
    }
public:
    ArenaObserver(tbb::task_arena &a, int id, int trap = 0) : tbb::task_scheduler_observer(a) {
        observe(true);
        ASSERT(id, NULL);
        myId = id;
        myTrappedSlot = trap;
    }
    ~ArenaObserver () {
        ASSERT(!old_id.local(), "inconsistent observer state");
    }
};

struct AsynchronousWork : NoAssign {
    Harness::SpinBarrier &my_barrier;
    bool my_is_blocking;
    AsynchronousWork(Harness::SpinBarrier &a_barrier, bool blocking = true)
    : my_barrier(a_barrier), my_is_blocking(blocking) {}
    void operator()() const {
        ASSERT(local_id.local() != 0, "not in explicit arena");
        tbb::parallel_for(Range(0,500), ConcurrencyTrackingBody(), tbb::simple_partitioner());
        if(my_is_blocking) my_barrier.timed_wait(10); // must be asynchronous to master thread
        else my_barrier.signal_nowait();
    }
};

void TestConcurrentArenas(int p) {
    //Harness::ConcurrencyTracker::Reset();
    tbb::task_arena a1(1);
    ArenaObserver o1(a1, p*2+1);
    tbb::task_arena a2(2);
    ArenaObserver o2(a2, p*2+2);
    Harness::SpinBarrier barrier(2);
    AsynchronousWork work(barrier);
    a1.enqueue(work); // put async work
    barrier.timed_wait(10);
    a2.enqueue(work); // another work
    a2.execute(work); // my_barrier.timed_wait(10) inside
    a1.wait_until_empty();
    a2.wait_until_empty();
}

class MultipleMastersBody : NoAssign {
    tbb::task_arena &my_a;
    Harness::SpinBarrier &my_b;
public:
    MultipleMastersBody(tbb::task_arena &a, Harness::SpinBarrier &b)
    : my_a(a), my_b(b) {}
    void operator()(int) const {
        my_a.execute(AsynchronousWork(my_b, /*blocking=*/false));
        my_a.wait_until_empty();
    }
};

void TestMultipleMasters(int p) {
    REMARK("multiple masters\n");
    tbb::task_arena a(1);
    ArenaObserver o(a, 1);
    Harness::SpinBarrier barrier(p+1);
    NativeParallelFor( p, MultipleMastersBody(a, barrier) );
    a.wait_until_empty();
    barrier.timed_wait(10);
}


int TestMain () {
    // TODO: a workaround for temporary p-1 issue in market
    tbb::task_scheduler_init init_market_p_plus_one(MaxThread+1);
    for( int p=MinThread; p<=MaxThread; ++p ) {
        REMARK("testing with %d threads\n", p );
        NativeParallelFor( p, &TestConcurrentArenas );
        ResetTLS();
        TestMultipleMasters( p );
        ResetTLS();
    }
    return Harness::Done;
}
#else // __TBB_CPF_BUILD
#include "harness.h"
int TestMain () {
    return Harness::Skipped;
}
#endif
