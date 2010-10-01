#ifndef __KERNEL_MODE_IPP
#define __KERNEL_MODE_IPP

#include "memory/Map.h"

namespace gmac {

inline void Mode::init()
{
    gmac::util::Private<Mode>::init(key);
}

inline void Mode::initThread()
{
    key.set(NULL);
}

inline bool
Mode::hasCurrent()
{
    return key.get() != NULL;
}

inline void Mode::inc()
{
    count_++;
}

inline void Mode::destroy()
{
    count_--;
    if(count_ == 0) delete this;
}

inline unsigned Mode::id() const
{
    return id_;
}

inline unsigned Mode::accId() const
{
    return acc_->id();
}

inline void
Mode::addObject(memory::Object *obj)
{
    map_->insert(obj);
}

#ifndef USE_MMAP
inline void
Mode::addReplicatedObject(memory::Object *obj)
{
    map_->insertReplicated(obj);
}

inline void
Mode::addCentralizedObject(memory::Object *obj)
{
    map_->insertCentralized(obj);
}

#endif

inline void
Mode::removeObject(memory::Object *obj)
{
    map_->remove(obj);
}

inline const memory::Object *
Mode::getObjectRead(const void *addr) const
{
    const memory::Object *obj = map_->getObjectRead(addr);
    return obj;
}

inline memory::Object *
Mode::getObjectWrite(const void *addr)
{
    memory::Object *obj = map_->getObjectWrite(addr);
    return obj;
}

inline void
Mode::putObject(const memory::Object &obj)
{
    map_->putObject(obj);
}

inline const memory::Map &
Mode::objects()
{
    return *map_;
}

inline gmacError_t
Mode::error() const
{
    return error_;
}

inline void
Mode::error(gmacError_t err)
{
    error_ = err;
}

#ifdef USE_VM
inline memory::vm::Bitmap &
Mode::dirtyBitmap()
{
    return *_bitmap;
}

inline const memory::vm::Bitmap &
Mode::dirtyBitmap() const
{
    return *_bitmap;
}
#endif

inline bool
Mode::releasedObjects() const
{
    return releasedObjects_;
}

inline void
Mode::releaseObjects()
{
    releasedObjects_ = true;
}

inline void
Mode::acquireObjects()
{
    releasedObjects_ = false;
}

inline Process &
Mode::process()
{
    return proc_;
}

inline const Process &
Mode::process() const
{
    return proc_;
}

}

#endif
