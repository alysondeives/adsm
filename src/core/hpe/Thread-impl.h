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
TLS::getCurrentThread()
{
    ASSERTION(CurrentThread_ != NULL);
    return *CurrentThread_;
}

inline
Thread::Thread(Process &proc) :
    process_(proc)
{
    ASSERTION(TLS::CurrentThread_ == NULL);
    TLS::CurrentThread_ = this;
}

inline
Thread::~Thread()
{
    TLS::CurrentThread_ = NULL;
    TLS::CurrentMode_ = NULL;
}

inline
bool
Thread::hasCurrentMode()
{
    return TLS::CurrentMode_ != NULL;
}

inline
Mode &
Thread::getCurrentMode()
{
    if(TLS::CurrentMode_ != NULL) return *TLS::CurrentMode_;
    TLS::CurrentMode_ = TLS::getCurrentThread().process_.createMode();
    return *TLS::CurrentMode_;
}

inline
void
Thread::setCurrentMode(Mode *mode)
{
    TLS::CurrentMode_ = mode;
}

}}}

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
