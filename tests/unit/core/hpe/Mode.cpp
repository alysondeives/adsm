#include "unit/core/hpe/Mode.h"

#include "core/hpe/Process.h"
#include "core/hpe/Mode.h"

#include "gtest/gtest.h"

using gmac::core::hpe::Process;
using __impl::core::hpe::Mode;

Mode *ModeTest::Mode_ = NULL;
Process *ModeTest::Process_ = NULL;

extern void OpenCL(Process &);
extern void CUDA(Process &);

void ModeTest::SetUpTestCase() {
    Process_ = new Process();
    if(Process_ == NULL) return;
#if defined(USE_CUDA)
    CUDA(*Process_);
#endif
#if defined(USE_OPENCL)
    OpenCL(*Process_);
#endif
    ASSERT_TRUE(Mode_ == NULL);
    Mode_ = Process_->createMode(0);
    ASSERT_TRUE(Mode_ != NULL);
}

void ModeTest::TearDownTestCase() {
    Mode_->detach();
    Process_->destroy();
    Mode_ = NULL;
    Process_ = NULL;
}


TEST_F(ModeTest, ModeCurrent) {
    Mode_->attach();
    Mode &current = Mode::getCurrent();
    ASSERT_TRUE(&current == Mode_);
}

TEST_F(ModeTest, ModeMemory) {
    hostptr_t fakePtr = (uint8_t *) 0xcafebabe;
    accptr_t addr(0);
    ASSERT_EQ(gmacSuccess, Mode_->map(addr, fakePtr, Size_));
    ASSERT_TRUE(addr != 0);

    ASSERT_EQ(gmacSuccess, Mode_->unmap(fakePtr, Size_));
}

TEST_F(ModeTest, MemoryCopy) {
    int *src = new int[Size_];
    ASSERT_TRUE(src != NULL);
    int *dst = new int[Size_];
    ASSERT_TRUE(dst != NULL);
    memset(src, 0x5a, Size_ * sizeof(int));
    accptr_t addr(0);
    ASSERT_EQ(gmacSuccess, Mode_->map(addr, hostptr_t(src), Size_ * sizeof(int)));
    ASSERT_TRUE(addr != 0);
    ASSERT_EQ(gmacSuccess, Mode_->copyToAccelerator(addr, hostptr_t(src), Size_ * sizeof(int)));
    ASSERT_EQ(gmacSuccess, Mode_->copyToHost(hostptr_t(dst), addr, Size_ * sizeof(int)));
    for(size_t i = 0; i < Size_; i++) ASSERT_EQ(0x5a5a5a5a, dst[i]);
    ASSERT_EQ(gmacSuccess, Mode_->unmap(hostptr_t(src), Size_ * sizeof(int)));
    delete[] dst;
    delete[] src;
}

TEST_F(ModeTest, MemorySet) {
    hostptr_t fakePtr = (uint8_t *) 0xcafebabe;
    accptr_t addr(0);
    ASSERT_EQ(gmacSuccess, Mode_->map(addr, fakePtr, Size_ * sizeof(int)));
    ASSERT_TRUE(addr != 0);
    ASSERT_EQ(gmacSuccess, Mode_->memset(addr, 0x5a, Size_ * sizeof(int)));

    int *dst = new int[Size_];
    ASSERT_EQ(gmacSuccess, Mode_->copyToHost(hostptr_t(dst), addr, Size_ * sizeof(int)));
    for(size_t i = 0; i < Size_; i++) ASSERT_EQ(0x5a5a5a5a, dst[i]);

    ASSERT_EQ(gmacSuccess, Mode_->unmap(fakePtr, Size_ * sizeof(int)));
    delete[] dst;
}
