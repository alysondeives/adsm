#include "gtest/gtest.h"
#include "core/IOBuffer.h"
#include "core/hpe/Mode.h"
#include "core/hpe/Process.h"
#include "memory/Manager.h"

using namespace gmac::core::hpe;
using namespace gmac::memory;

class ManagerTest : public testing::Test {
public:
    static Process *Process_;
    static const size_t Size_;

    static void SetUpTestCase();
    static void TearDownTestCase();
};

Process *ManagerTest::Process_ = NULL;
const size_t ManagerTest::Size_ = 4 * 1024 * 1024;

extern void OpenCL(Process &);
extern void CUDA(Process &);

void ManagerTest::SetUpTestCase()
{
    Process_ = new Process();
    ASSERT_TRUE(Process_ != NULL);
#if defined(USE_CUDA)
    CUDA(*Process_);
#endif
#if defined(USE_OPENCL)
    OpenCL(*Process_);
#endif
}

void ManagerTest::TearDownTestCase()
{
    ASSERT_TRUE(Process_ != NULL);
    Process_->destroy(); Process_ = NULL;
}

TEST_F(ManagerTest, Creation)
{
    ASSERT_TRUE(Process_ != NULL);
    for(int i = 0; i < 16; i++) {
        Manager *manager = new Manager(*Process_);
        ASSERT_TRUE(manager != NULL);
        manager->destroy();
    }
}

TEST_F(ManagerTest, Alloc)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);
    
    for(size_t size = 4096; size < Size_; size *= 2) {
        hostptr_t ptr = NULL;
        ASSERT_EQ(gmacSuccess, manager->alloc(Process_->getCurrentMode(), &ptr, size));
        ASSERT_TRUE(ptr != NULL);

        ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    }
    manager->destroy();
}

TEST_F(ManagerTest, GlobalAllocReplicated)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);
    
    for(size_t size = 4096; size < Size_; size *= 2) {
        hostptr_t ptr = NULL;
        ASSERT_EQ(gmacSuccess, manager->globalAlloc(Process_->getCurrentMode(), &ptr, size,
                                                    GMAC_GLOBAL_MALLOC_REPLICATED));
        ASSERT_TRUE(ptr != NULL);

        ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    }
    manager->destroy();
}

TEST_F(ManagerTest, GlobalAllocCentralized)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);
    
    for(size_t size = 4096; size < Size_; size *= 2) {
        hostptr_t ptr = NULL;
        ASSERT_EQ(gmacSuccess, manager->globalAlloc(Process_->getCurrentMode(), &ptr, size,
                                                    GMAC_GLOBAL_MALLOC_CENTRALIZED));
        ASSERT_TRUE(ptr != NULL);

        ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    }
    manager->destroy();
}

TEST_F(ManagerTest, Coherence)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);

    hostptr_t ptr = NULL;
    ASSERT_EQ(gmacSuccess, manager->alloc(Process_->getCurrentMode(), &ptr, Size_));
    ASSERT_TRUE(ptr != NULL);
    ASSERT_TRUE(manager->translate(Process_->getCurrentMode(), ptr).get() != NULL);

	ASSERT_TRUE(Process_->getCurrentMode().validObjects());

    for(int n = 0; n < 16; n++) {
	    for(size_t s = 0; s < Size_; s++) {
	        ptr[s] = (s & 0xff);
	    }
	
        if(Process_->getCurrentMode().validObjects()) {
    	    ASSERT_EQ(gmacSuccess, manager->releaseObjects(Process_->getCurrentMode()));
    	    ASSERT_EQ(gmacSuccess, manager->acquireObjects(Process_->getCurrentMode()));
        }
	    ASSERT_FALSE(Process_->getCurrentMode().validObjects());
	    for(size_t s = 0; s < Size_; s++) {
	        EXPECT_EQ(ptr[s], (s & 0xff));
	    }
    }

    ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    manager->destroy();
}

TEST_F(ManagerTest, IOBufferWrite)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);

    hostptr_t ptr = NULL;
    ASSERT_EQ(gmacSuccess, manager->alloc(Process_->getCurrentMode(), &ptr, Size_));
    ASSERT_TRUE(ptr != NULL);
    ASSERT_TRUE(manager->translate(Process_->getCurrentMode(), ptr).get() != NULL);

    __impl::core::IOBuffer &buffer = Process_->getCurrentMode().createIOBuffer(Size_);

    for(size_t n = 0; n < 16; n++) {
	    for(size_t s = 0; s < Size_; s++) {
	        ptr[s] = (s & 0xff);
	    }

        memset(buffer.addr(), 0x5a, Size_);
        ASSERT_EQ(gmacSuccess, manager->fromIOBuffer(Process_->getCurrentMode(), ptr + n * 128,
                                                     buffer, n * 128, Size_ - n * 128));

        for(size_t s = 0; s < n * 128; s++) EXPECT_EQ(ptr[s], (s & 0xff));
	    for(size_t s = n * 128; s < Size_; s++) EXPECT_EQ(ptr[s], 0x5a);
    }
    Process_->getCurrentMode().destroyIOBuffer(buffer);
    ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    manager->destroy();
}


TEST_F(ManagerTest, IOBufferRead)
{
    ASSERT_TRUE(Process_ != NULL);
    Manager *manager = new Manager(*Process_);
    ASSERT_TRUE(manager != NULL);

    hostptr_t ptr = NULL;
    ASSERT_EQ(gmacSuccess, manager->alloc(Process_->getCurrentMode(), &ptr, Size_));
    ASSERT_TRUE(ptr != NULL);
    ASSERT_TRUE(manager->translate(Process_->getCurrentMode(), ptr).get() != NULL);

    __impl::core::IOBuffer &buffer = Process_->getCurrentMode().createIOBuffer(Size_);

    for(size_t n = 0; n < 16; n++) {
	    for(size_t s = n * 128; s < Size_; s++) {
	        ptr[s] = (s & 0xff);
	    }

        memset(buffer.addr(), 0x5a, Size_);
        ASSERT_EQ(gmacSuccess, manager->toIOBuffer(Process_->getCurrentMode(), buffer, n * 128,
                                                     ptr + n * 128, Size_ - n * 128));

        for(size_t s = 0; s < n * 128; s++) EXPECT_EQ(buffer.addr()[s], 0x5a);
	    for(size_t s = n * 128; s < Size_; s++) EXPECT_EQ(buffer.addr()[s], (s & 0xff));
    }
    Process_->getCurrentMode().destroyIOBuffer(buffer);
    ASSERT_EQ(gmacSuccess, manager->free(Process_->getCurrentMode(), ptr));
    manager->destroy();
}

