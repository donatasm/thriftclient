#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftClient::ThriftClient(ITransportFactory^ factory)
        {
            if (factory == nullptr)
                throw gcnew ArgumentNullException("factory");

            _thisHandle = GCHandleAllocNormal(this);

            _factory = factory;
            _requests = gcnew ConcurrentQueue<TransportHandler^>();

            _loop = uv_loop_new();

            // loop notifier when there are requests pending
            _notifier = new uv_async_t();
            _notifier->data = _thisHandle;
            uv_async_init(_loop, _notifier, NotifyCallback);
        }


        void ThriftClient::Send(TransportHandler^ request)
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

            while (true)
            {
                TransportHandler^ request;

                if (!client->_requests->TryDequeue(request))
                {
                    break;
                }

                TTransport^ transport = client->_factory->Create();
                request(transport);
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
