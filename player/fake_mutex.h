/* fake_mutex.h                  -*-C++-*-
 *
 *************************************************************************
 *                         INTEL CONFIDENTIAL
 *
 * Copyright (C) 2011 Intel Corporation.  All Rights Reserved.
 *
 *  The source code contained or described herein and all documents related
 *  to the source code ("Material") are owned by Intel Corporation or its
 *  suppliers or licensors.  Title to the Material remains with Intel
 *  Corporation or its suppliers and licensors.  The Material is protected
 *  by worldwide copyright laws and treaty provisions.  No part of the
 *  Material may be used, copied, reproduced, modified, published, uploaded,
 *  posted, transmitted, distributed, or disclosed in any way without
 *  Intel's prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other
 *  intellectual property right is granted to or conferred upon you by
 *  disclosure or delivery of the Materials, either expressly, by
 *  implication, inducement, estoppel or otherwise.  Any license under such
 *  intellectual property rights must be express and approved by Intel in
 *  writing.
 *
 **************************************************************************
 *
 * Cilkscreen fake mutexes are provided to indicate to the Cilkscreen race
 * detector that a race should be ignored.
 *
 * NOTE: This class does not provide mutual exclusion.  You should use the
 * mutual exclusion constructs provided by TBB or your operating system to
 * protect against real data races.
 */

#ifndef FAKE_MUTEX_H_INCLUDED
#define FAKE_MUTEX_H_INCLUDED

#include "/afs/csail/proj/courses/6.172/cilkutil/include/cilktools/cilkscreen.h"

// If this is Windows, specify the linkage
#ifdef _WIN32
#define CILKSCREEN_CDECL __cdecl
#else
#define CILKSCREEN_CDECL
#endif // _WIN32

namespace cilkscreen
{
    class fake_mutex
    {
    public:

        // Wait until mutex is available, then enter
        virtual void lock()
        {
            __cilkscreen_acquire_lock(&lock_val);
        }

        // A fake mutex is always available
        virtual bool try_lock() { lock(); return true; }

        // Releases the mutex
        virtual void unlock()
        {
            __cilkscreen_release_lock(&lock_val);
        }

    private:
        int lock_val;
    };

    // Factory function for fake mutex
    inline
    fake_mutex *CILKSCREEN_CDECL create_fake_mutex() { return new fake_mutex(); }

    // Destructor function for fake mutex - The mutex cannot be used after
    // calling this function
    inline
    void CILKSCREEN_CDECL destroy_fake_mutex(fake_mutex *m) { delete m; }

} // namespace cilk

#endif  // FAKE_MUTEX_H_INCLUDED
