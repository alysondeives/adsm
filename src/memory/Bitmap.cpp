#ifdef USE_VM

#include <cmath>
#include <cstring>

#include "core/Mode.h"

#include "Bitmap.h"
#include "Memory.h"

namespace __impl { namespace memory { namespace vm {

const unsigned &Bitmap::BitmapLevels_ = util::params::ParamBitmapLevels;
const unsigned &Bitmap::L1Entries_ = util::params::ParamBitmapL1Entries;
const unsigned &Bitmap::L2Entries_ = util::params::ParamBitmapL2Entries;
const unsigned &Bitmap::L3Entries_ = util::params::ParamBitmapL3Entries;
const size_t &Bitmap::BlockSize_ = util::params::ParamBlockSize;
const unsigned &Bitmap::SubBlocks_ = util::params::ParamSubBlocks;

long_t Bitmap::L1Mask_;
long_t Bitmap::L2Mask_;
long_t Bitmap::L3Mask_;
unsigned Bitmap::L1Shift_;
unsigned Bitmap::L2Shift_;
unsigned Bitmap::L3Shift_;

Node::Node(unsigned level, size_t nEntries, std::vector<unsigned> nextEntries) :
    level_(level), nEntries_(nEntries), nUsedEntries_(0),
    usedEntries_(nEntries),
    firstUsedEntry_(-1), lastUsedEntry_(-1),
    nextEntries_(nextEntries)
{
    TRACE(LOCAL, "Node constructor");

    long shift = 0;
    for (size_t i = 0; i < nextEntries_.size(); i++) {
        shift += log2(nextEntries[i]);
    }

    for (size_t i = 0; i < nEntries; i++) {
        usedEntries_[i] = false;
    }

    mask_  = (long_t(nEntries) - 1) << shift;
    shift_ = shift;
    TRACE(LOCAL, "Entries: %zd", nEntries_);
    TRACE(LOCAL, "Shift: %u", shift_);
    TRACE(LOCAL, "Mask : %lx", mask_);
}

template <typename S>
NodeStore<S>::~NodeStore()
{
    TRACE(LOCAL, "NodeStore destructor");

    if (nextEntries_.size() > 0) {
        for (long_t i = getFirstUsedEntry(); i <= getLastUsedEntry(); i++) {
            Node *node = getNode(i);
            if (node != NULL) {
                delete node;
            }
        }
    }
}

void
NodeHost::registerRange(long_t startIndex, long_t endIndex)
{
    long_t localStartIndex = getLocalIndex(startIndex);
    long_t localEndIndex = getLocalIndex(endIndex);

    addEntries(localStartIndex, localEndIndex);

    TRACE(LOCAL, "registerRange 0x%lx 0x%lx", localStartIndex, localEndIndex);

    if (nextEntries_.size() == 0) {
        for (long_t i = localStartIndex; i <= localEndIndex; i++) {
            uint8_t &leaf = getLeafRef(i);
            leaf = uint8_t(BITMAP_UNSET);
        }
        return;
    }

    long_t startWIndex = startIndex;
    long_t endWIndex   = (localStartIndex == localEndIndex)? endIndex:
                          getGlobalIndex(localStartIndex + 1) - 1;

    long_t i = localStartIndex;
    do {
        Node *&node = getNodeRef(i);
        if (node == NULL) {
            node = createChild();
        }
        node->registerRange(getNextIndex(startWIndex), getNextIndex(endWIndex));
        i++;
        startWIndex = getGlobalIndex(i);
        endWIndex = (i < localEndIndex)? getGlobalIndex(i + 1) - 1: endIndex;
    } while (i <= localEndIndex);
}

void
NodeHost::unregisterRange(long_t startIndex, long_t endIndex)
{
    long_t localStartIndex = getLocalIndex(startIndex);
    long_t localEndIndex = getLocalIndex(endIndex);

    TRACE(LOCAL, "unregisterRange 0x%lx 0x%lx", localStartIndex, localEndIndex);

    if (nextEntries_.size() == 0) {
        removeEntries(localStartIndex, localEndIndex);
        return;
    }

    long_t startWIndex = startIndex;
    long_t endWIndex   = (localStartIndex == localEndIndex)? endIndex:
                         getGlobalIndex(localStartIndex + 1) - 1;

    long_t i = localStartIndex;
    do {
        Node *&node = getNodeRef(i);
        node->unregisterRange(getNextIndex(startWIndex), getNextIndex(endWIndex));
        if (node->getNUsedEntries() == 0) {
            delete node;
            node = NULL;
        }
        i++;
        startWIndex = getGlobalIndex(i);
        endWIndex = (i < localEndIndex)? getGlobalIndex(i + 1) - 1: endIndex;
    } while (i <= localEndIndex);
}

void
NodeShared::registerRange(long_t startIndex, long_t endIndex)
{
    long_t localStartIndex = getLocalIndex(startIndex);
    long_t localEndIndex = getLocalIndex(endIndex);

    addEntries(localStartIndex, localEndIndex);
    addDirtyEntries(localStartIndex, localEndIndex);

    if (entriesAcc_ == NULL) allocAcc(getLevel() == 0);

    TRACE(LOCAL, "registerRange 0x%lx 0x%lx", localStartIndex, localEndIndex);

    if (nextEntries_.size() == 0) {
        for (long_t i = localStartIndex; i <= localEndIndex; i++) {
            uint8_t &leaf = getLeafRef(i);
            leaf = uint8_t(BITMAP_UNSET);
        }
        return;
    }

    long_t startWIndex = startIndex;
    long_t endWIndex   = (localStartIndex == localEndIndex)? endIndex:
                                getGlobalIndex(localStartIndex + 1) - 1;

    long_t i = localStartIndex;
    do {
        NodeShared *&node = getNodeRef(i);
        if (node == NULL) {
            node = (NodeShared *) createChild();
            node->allocAcc(false);

            NodeShared *&nodeAcc = getNodeAccHostRef(i);
            nodeAcc = node->getNodeAccAddr(0);
            TRACE(LOCAL, "linking with 0x%p", nodeAcc);
        }

        node->registerRange(getNextIndex(startWIndex), getNextIndex(endWIndex));
        i++;
        startWIndex = getGlobalIndex(i);
        endWIndex = (i < localEndIndex)? getGlobalIndex(i + 1) - 1: endIndex;
    } while (i <= localEndIndex);
}

void
NodeShared::unregisterRange(long_t startIndex, long_t endIndex)
{
    long_t localStartIndex = getLocalIndex(startIndex);
    long_t localEndIndex = getLocalIndex(endIndex);

    TRACE(LOCAL, "unregisterRange 0x%lx 0x%lx", localStartIndex, localEndIndex);

    if (nextEntries_.size() == 0) {
        removeEntries(localStartIndex, localEndIndex);
        return;
    }

    long_t startWIndex = startIndex;
    long_t endWIndex   = (localStartIndex == localEndIndex)? endIndex:
                                getGlobalIndex(localStartIndex + 1) - 1;

    long_t i = localStartIndex;
    do {
        NodeShared *&node = getNodeRef(i);
        node->unregisterRange(getNextIndex(startWIndex), getNextIndex(endWIndex));
        if (node->getNUsedEntries() == 0) {
            NodeShared *&nodeAcc = getNodeAccHostRef(i);
            delete node;
            node = NULL;
            nodeAcc = NULL;
        }
        i++;
        startWIndex = getGlobalIndex(i);
        endWIndex = (i < localEndIndex)? getGlobalIndex(i + 1) - 1: endIndex;
    } while (i <= localEndIndex);
}


Node *
NodeHost::createChild()
{
    if (nextEntries_.size() == 0) return NULL;

    std::vector<unsigned> nextEntries(nextEntries_.size() - 1);
    std::copy(++nextEntries_.begin(), nextEntries_.end(), nextEntries.begin());
    return new NodeHost(getLevel() + 1, root_, nextEntries_[0], nextEntries);
}

Node *
NodeShared::createChild()
{
    if (nextEntries_.size() == 0) return NULL;

    std::vector<unsigned> nextEntries(nextEntries_.size() - 1);
    std::copy(++nextEntries_.begin(), nextEntries_.end(), nextEntries.begin());
    NodeShared *node = new NodeShared(getLevel() + 1, root_, nextEntries_[0], nextEntries);
    return node;
}

void
NodeShared::acquire()
{
    TRACE(LOCAL, "Acquire");

    if (nextEntries_.size() > 0) {
        for (long_t i = getFirstUsedEntry(); i <= getLastUsedEntry(); i++) {
            NodeShared *node = (NodeShared *) getNode(i);
            if (node != NULL) node->acquire();
        }
    } else {
        // Only leaf nodes must be synced
        this->setSynced(false);
    }
}

void
NodeShared::release()
{
    if (nextEntries_.size() > 0) {
        for (long_t i = getFirstUsedEntry(); i <= getLastUsedEntry(); i++) {
            NodeShared *node = (NodeShared *) getNode(i);
            if (node != NULL) node->release();
        }
    }

    if (this->isDirty()) {
        if (nextEntries_.size() > 0) {
            this->syncToAccelerator<Node *>(this->firstDirtyEntry_, this->lastDirtyEntry_);
        } else {
            this->syncToAccelerator<uint8_t>(this->firstDirtyEntry_, this->lastDirtyEntry_);
        }
    }
}

void
Bitmap::Init()
{
    unsigned shift = SubBlockShift_;

    if (BitmapLevels_ == 3) {
        Bitmap::L3Shift_ = shift;
        Bitmap::L3Mask_  = long_t(Bitmap::L3Entries_ - 1) << shift;
        shift += log2(Bitmap::L3Entries_);
        TRACE(GLOBAL, "L3SEntries_ %u", Bitmap::L3Shift_);
        TRACE(GLOBAL, "L3Shift %u", Bitmap::L3Shift_);
        TRACE(GLOBAL, "L3Mask %lu", Bitmap::L3Mask_);
    }

    Bitmap::L2Shift_ = shift;
    Bitmap::L2Mask_  = (long_t(Bitmap::L2Entries_) - 1) << shift;
    shift += log2(Bitmap::L2Entries_);
    TRACE(GLOBAL, "L2Entries %u", Bitmap::L2Entries_);
    TRACE(GLOBAL, "L2Shift %u", Bitmap::L2Shift_);
    TRACE(GLOBAL, "L2Mask %lu", Bitmap::L2Mask_);

    Bitmap::L1Shift_ = shift;
    Bitmap::L1Mask_  = (long_t(Bitmap::L1Entries_) - 1) << shift;
    TRACE(GLOBAL, "L1Shift %u", Bitmap::L1Shift_);
    TRACE(GLOBAL, "L1Mask %lu", Bitmap::L1Mask_);
}

Bitmap::Bitmap(core::Mode &mode, bool shared) :
    mode_(mode)
{
    TRACE(LOCAL, "Bitmap constructor");
}

Bitmap::~Bitmap()
{
    TRACE(LOCAL, "Bitmap destructor");
}

void
Bitmap::cleanUp()
{
    delete root_;
}

BitmapHost::BitmapHost(core::Mode &mode) :
    Bitmap(mode, false)
{
    std::vector<unsigned> nextEntries;

    if (BitmapLevels_ > 1) {
        nextEntries.push_back(L2Entries_);
    }
    if (BitmapLevels_ == 3) {
        nextEntries.push_back(L3Entries_);
    }

    root_ = new NodeHost(0, *this, L1Entries_, nextEntries);
}

BitmapShared::BitmapShared(core::Mode &mode) :
    Bitmap(mode, true),
    released_(false)
{
    std::vector<unsigned> nextEntries;

    if (BitmapLevels_ > 1) {
        nextEntries.push_back(L2Entries_);
    }
    if (BitmapLevels_ == 3) {
        nextEntries.push_back(L3Entries_);
    }

    NodeShared *node = new NodeShared(0, *this, L1Entries_, nextEntries);
    root_ = node;
}

void
Bitmap::registerRange(const accptr_t addr, size_t bytes)
{
    TRACE(LOCAL, "registerRange %p %zd", (void *) addr, bytes);

    root_->registerRange(getIndex(addr), getIndex(addr + bytes - 1));
}

void
Bitmap::unregisterRange(const accptr_t addr, size_t bytes)
{
    TRACE(LOCAL, "unregisterRange %p %zd", (void *) addr, bytes);

    root_->unregisterRange(getIndex(addr), getIndex(addr + bytes - 1));
}

#if 0
#ifdef BITMAP_BIT
const unsigned Bitmap::EntriesPerByte_ = 8;
#else // BITMAP_BYTE
const unsigned Bitmap::EntriesPerByte_ = 1;
#endif

Bitmap::Bitmap(core::Mode &mode, unsigned bits) :
    RWLock("Bitmap"), bits_(bits), mode_(mode), bitmap_(NULL), dirty_(true), minPtr_(NULL), maxPtr_(NULL)
{
    unsigned rootEntries = (1 << bits) >> 32;
    if (rootEntries == 0) rootEntries = 1;
    rootEntries_ = rootEntries;

    bitmap_ = new hostptr_t[rootEntries];
    ::memset(bitmap_, 0, rootEntries * sizeof(hostptr_t));

    shiftBlock_ = int(log2(util::params::ParamPageSize));
    shiftPage_  = shiftBlock_ - int(log2(util::params::ParamSubBlocks));

    subBlockSize_ = (util::params::ParamSubBlocks) - 1;
    subBlockMask_ = (util::params::ParamSubBlocks) - 1;
    pageMask_     = subBlockSize_ - 1;

    size_    = (1 << (bits - shiftPage_)) / EntriesPerByte_;
#ifdef BITMAP_BIT
    bitMask_ = (1 << 3) - 1;
#endif

    TRACE(LOCAL, "Pages: %u", 1 << (bits - shiftPage_));
    TRACE(LOCAL,"Size : %u", size_);
}

Bitmap::Bitmap(const Bitmap &base) :
    RWLock("Bitmap"),
    bits_(base.bits_),
    mode_(base.mode_),
    bitmap_(base.bitmap_),
    dirty_(true),
    shiftBlock_(base.shiftBlock_),
    shiftPage_(base.shiftPage_),
    subBlockSize_(base.subBlockSize_),
    subBlockMask_(base.subBlockMask_),
    pageMask_(base.pageMask_),
#ifdef BITMAP_BIT
    bitMask_(base.bitMask_),
#endif
    size_(base.size_),
    minEntry_(-1), maxEntry_(-1)
{
}


Bitmap::~Bitmap()
{
    
}

void
Bitmap::cleanUp()
{
    for (long int i = minRootEntry_; i <= maxRootEntry_; i++) {
        if (bitmap_[i] != NULL) {
            delete [] bitmap_[i];
        }
    }
    delete [] bitmap_;
}

SharedBitmap::SharedBitmap(core::Mode &mode, unsigned bits) :
    Bitmap(mode, bits), linked_(false), synced_(true), accelerator_(NULL)
{
}

SharedBitmap::SharedBitmap(const Bitmap &host) :
    Bitmap(host), linked_(true), synced_(true), accelerator_(NULL)
{
}

SharedBitmap::~SharedBitmap()
{
}


#ifdef DEBUG_BITMAP
void Bitmap:dump()
{
    core::Context * ctx = Mode::current()->context();
    ctx->invalidate();

    static int idx = 0;
    char path[256];
    sprintf(path, "_bitmap__%d", idx++);
    FILE * file = fopen(path, "w");
    fwrite(bitmap_, 1, size_, file);    
    fclose(file);
}
#endif
#endif

}}}

#endif
