#include "Common.h"

namespace Thrift
{
    namespace Client
    {
        ContextQueue::ContextQueue()
        {
            _handle = GCHandle::Alloc(this);
            _queue = gcnew ConcurrentQueue<ThriftContext^>();
        }


        ContextQueue::~ContextQueue()
        {
            if (_handle.IsAllocated)
            {
                _handle.Free();
            }
        }


        void ContextQueue::Enqueue(ThriftContext^ context)
        {
            _queue->Enqueue(context);
        }


        bool ContextQueue::TryDequeue(ThriftContext^ %context)
        {
            return _queue->TryDequeue(context);
        }


        void* ContextQueue::ToPointer()
        {
            return GCHandle::ToIntPtr(_handle).ToPointer();
        }


        ContextQueue^ ContextQueue::FromPointer(void* ptr)
        {
            GCHandle handle = GCHandle::FromIntPtr(IntPtr(ptr));
            return (ContextQueue^)handle.Target;
        }
    }
}