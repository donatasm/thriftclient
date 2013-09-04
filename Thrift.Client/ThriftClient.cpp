#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftClient::ThriftClient()
        {
            _thisHandle = GCHandleAllocNormal(this);

            _requests = gcnew ConcurrentQueue<RequestHandler^>();

            _loop = uv_loop_new();

            // loop notifier when there are requests pending
            _notifier = new uv_async_t();
            _notifier->data = _thisHandle;
            uv_async_init(_loop, _notifier, NotifyCallback);
        }


        void ThriftClient::Send(RequestHandler^ request)
        {
            if (request == nullptr)
                throw gcnew ArgumentNullException("request");

            _requests->Enqueue(request);

            // notify that there are requests pending
            uv_async_send(_notifier);
        }


        void ThriftClient::Run()
        {
            uv_run(_loop, UV_RUN_DEFAULT);
        }


        ThriftClient::~ThriftClient()
        {
            delete _notifier;

            uv_loop_delete(_loop);

            // free handle
            GCHandleFromVoidPtr(_thisHandle).Free();
            delete _thisHandle;
        }


        static void NotifyCallback(uv_async_t* notifier, int status)
        {
            Object^ target = GCHandleFromVoidPtr(notifier->data).Target;
            ThriftClient^ client = (ThriftClient^)target;
            RequestHandler^ request;

            while (client->_requests->TryDequeue(request))
            {               
				TProtocol^ protocol = gcnew TBinaryProtocol(gcnew TMemoryBuffer());
                request(protocol);
            }
        }


        static GCHandle GCHandleFromVoidPtr(void* ptr)
        {
            return GCHandle::FromIntPtr(IntPtr(ptr));
        }


        static void* GCHandleAllocNormal(Object^ object)
        {
            return GCHandle::ToIntPtr(GCHandle::Alloc(object)).ToPointer();
        }
    }
}
