/*
 * <+ DESCRIPTION +>
 *
 * Copyright (C) 2011, Javier Cabezas <jcabezas in ac upc edu> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * }}}
 */

#ifndef GMAC_CORE_THREAD_HPE_IMPL_H_
#define GMAC_CORE_THREAD_HPE_IMPL_H_

#include "hpe/init.h"

#include "core/hpe/Process.h"

#include "util/Private.h"

namespace __impl { namespace core { namespace hpe {

inline
Thread &
Thread::getCurrentThread()
{
    return static_cast<Thread &>(TLS::getCurrentThread());
}

inline
VirtualDeviceTable &
Thread::getCurrentVirtualDeviceTable()
{
    return getCurrentThread().vDeviceTable_;
}

inline
Thread::Thread(Process &proc) :
    core::Thread(),
    process_(proc),
    currentVirtualDevice_(NULL)
{
}

inline
Mode &
Thread::getCurrentVirtualDevice()
{
    Mode *ret = getCurrentThread().currentVirtualDevice_;
    if(ret != NULL) return *ret;
    ret = getCurrentThread().process_.createMode();
    getCurrentThread().currentVirtualDevice_ = ret;
    
    return *ret;
}

inline
bool
Thread::hasCurrentVirtualDevice()
{
    return getCurrentThread().currentVirtualDevice_ != NULL;
}

inline
void
Thread::setCurrentVirtualDevice(Mode &mode)
{
    getCurrentThread().currentVirtualDevice_ = &mode;
}

}}}

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
