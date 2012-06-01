#ifndef GMAC_HAL_CUDA_VIRT_ASPACE_H_
#define GMAC_HAL_CUDA_VIRT_ASPACE_H_

#include <cuda.h>
#include <driver_types.h>
#include <vector_types.h>

#include <queue>

#include "util/unique.h"
#include "util/lock.h"

#include "hal/detail/types.h"

namespace __impl { namespace hal { namespace cuda {

namespace phys {
typedef hal::detail::phys::processing_unit hal_processing_unit;
}

namespace virt {

class context;

typedef hal::detail::code::repository hal_code_repository;
typedef hal::detail::code::repository_view hal_code_repository_view;
typedef hal::detail::virt::aspace hal_aspace;
typedef hal::detail::virt::context hal_context;
typedef hal::detail::virt::object hal_object;
typedef hal::detail::event hal_event;
typedef hal::detail::event_ptr hal_event_ptr;
typedef hal::detail::stream hal_stream;

template <typename T>
class GMAC_LOCAL map_pool :
    std::map<size_t, std::list<T *> >,
    gmac::util::mutex<map_pool<T> > {

    typedef std::list<T *> queue_subset;
    typedef std::map<size_t, queue_subset> Parent;

public:
    map_pool() :
        gmac::util::mutex<map_pool>("map_pool")
    {
    }

    T *pop(size_t size)
    {
        T *ret = NULL;

        this->lock();
        typename Parent::iterator it;
        it = Parent::lower_bound(size);
        if (it != Parent::end()) {
            queue_subset &queue = it->second;
            if (queue.size() > 0) {
                ret = queue.front();
                queue.pop_front();
            }
        }
        this->unlock();

        return ret;
    }

    bool remove(T *val, size_t size)
    {
        bool ret = false;

        this->lock();
        typename Parent::iterator it;
        it = Parent::lower_bound(size);
        if (it != Parent::end()) {
            queue_subset &queue = it->second;

            typename queue_subset::iterator it2;
            it2 = std::find(queue.begin(), queue.end(), val);
            if (it2 != queue.end()) {
                queue.erase(it2);
                ret = true;
            }
        }
        this->unlock();

        return ret;
    }

    void push(T *v, size_t size)
    {
    	this->lock();
        typename Parent::iterator it;
        it = Parent::find(size);
        if (it != Parent::end()) {
            queue_subset &queue = it->second;
            queue.push_back(v);
        } else {
            queue_subset queue;
            queue.push_back(v);
            Parent::insert(typename Parent::value_type(size, queue));
        }
        this->unlock();
    }
};

class GMAC_LOCAL buffer {
    friend class util::factory<buffer>;
public:
    enum type {
        ToHost,
        ToDevice,
        DeviceToDevice
    };
private:
    aspace &aspace_;
    host_ptr addr_;
    size_t size_;

    event_ptr event_;

protected:
    buffer(host_ptr addr, size_t size, aspace &as);

    type get_type() const;

public:
    host_ptr get_addr();
    //virtual ptr get_device_addr() = 0;
    aspace &get_aspace();
    const aspace &get_aspace() const;
    size_t get_size() const;

    void set_event(event_ptr event);

    hal::error wait();
};


class GMAC_LOCAL queue_op :
    std::queue<operation *>,
    gmac::util::spinlock<queue_op> {

    typedef std::queue<operation *> Parent;
    typedef gmac::util::spinlock<queue_op> Lock;

public:
    queue_op();
    operation *pop();
    void push(operation &op);
};

class GMAC_LOCAL aspace :
    public hal_aspace,
    public util::factory<buffer>,
    public util::factory<operation> {

    typedef hal_aspace parent;
    typedef util::factory<buffer>    factory_buffer;
    typedef util::factory<operation> factory_operation;

    friend operation *cuda::create_op(operation::type t, bool async, virt::aspace &as, stream &s);

    friend class buffer_t;
    friend class event_deleter;
    friend class stream;

    CUcontext context_;

    stream *streamToHost_;
    stream *streamToDevice_;
    stream *streamDevice_;
    stream *streamCompute_;

    typedef map_pool<void> map_memory;
    typedef map_pool<buffer> map_buffer;

    typedef util::locked_counter<unsigned, gmac::util::spinlock<aspace> > buffer_counter;

    static const unsigned &MaxBuffersIn_;
    static const unsigned &MaxBuffersOut_;

    buffer_counter nBuffersIn_;
    buffer_counter nBuffersOut_;

    queue_op queueOps_;

    map_memory mapMemory_;

    map_buffer mapFreeBuffersIn_;
    map_buffer mapFreeBuffersOut_;

    map_buffer mapUsedBuffersIn_;
    map_buffer mapUsedBuffersOut_;

    buffer *get_input_buffer(size_t size, stream &stream, event_ptr event);
    void put_input_buffer(buffer &buffer);
 
    buffer *get_output_buffer(size_t size, stream &stream, event_ptr event);
    void put_output_buffer(buffer &buffer);

    buffer *alloc_buffer(size_t size, GmacProtection hint, stream &stream, hal::error &err);
    hal::error free_buffer(buffer &buffer);

    host_ptr get_memory(size_t size);
    void put_memory(void *ptr, size_t size);

public:
    hal_event_ptr copy(hal::ptr dst, hal::const_ptr src, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr copy_async(hal::ptr dst, hal::const_ptr src, size_t count, list_event_detail *dependencies, hal::error &err);

    hal_event_ptr copy(hal::ptr dst, device_input &input, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr copy(device_output &output, hal::const_ptr src, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr memset(hal::ptr dst, int c, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr copy_async(hal::ptr dst, device_input &input, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr copy_async(device_output &output, hal::const_ptr src, size_t count, list_event_detail *dependencies, hal::error &err);
    hal_event_ptr memset_async(hal::ptr dst, int c, size_t count, list_event_detail *dependencies, hal::error &err);

    aspace(hal_aspace::set_processing_unit &compatibleUnits, phys::aspace &pas, hal::error &err);
    virtual ~aspace();

    bool has_direct_copy(hal::const_ptr ptr1, hal::const_ptr ptr2)
    {
        // TODO: refine the logic
        if (&ptr1.get_view().get_vaspace() ==
            &ptr2.get_view().get_vaspace()) {
            // Copy within the same virtual address space
            return true;
        } else {
            // Copy across different virtual address spaces
            return false;
        }
    }

    // hal::ptr alloc(size_t size, hal::error &err);

    hal::ptr map(hal_object &obj, GmacProtection prot, hal::error &err);
    hal::ptr map(hal_object &obj, GmacProtection prot, ptrdiff_t offset, hal::error &err);

    // Specialization for code mappings
    hal_code_repository_view *map(const hal_code_repository &repo, hal::error &err);
#if 0
    hal::ptr alloc_host_pinned(size_t size, GmacProtection hint, hal::error &err);
#endif
    hal::error unmap(hal::ptr p);
    hal::error unmap(hal_code_repository_view &view);

#if 0
    hal::error free(hal::ptr acc);
    hal::error free_host_pinned(hal::ptr ptr);
#endif

    //hal::ptr get_device_addr_from_pinned(host_ptr addr);

    //hal_code_repository &get_code_repository();

    void set();

    CUcontext &operator()();
    const CUcontext &operator()() const;

    operation *get_new_op(operation::type t, bool async, stream &s);
    void dispose_op(operation &op);

    hal_context *create_context(hal::error &err);
    void destroy_context(hal_context &ctx);
};

#if 0
class GMAC_LOCAL buffer_t {
    typedef buffer parent;

    host_ptr addr_;

    aspace &get_aspace();
    const aspace &get_aspace() const;

public:
    buffer_t(host_ptr addr, size_t size, aspace &as);

    host_ptr get_addr();
    //hal::ptr get_device_addr();
};
#endif

}

}}}

#endif /* GMAC_HAL_CUDA_CONTEXT_H_ */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
