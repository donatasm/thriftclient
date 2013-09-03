#include "uv.h"

using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::Runtime::InteropServices;
using namespace Thrift::Transport;

namespace Thrift
{
    namespace Client
    {
        public delegate void TransportHandler(TTransport^);


        public interface class ITransportFactory
        {
            TTransport^ Create();
        };


        public ref class ThriftClient sealed
        {
        public:
            ThriftClient(ITransportFactory^ factory);
            void Send(TransportHandler^ request);
            void Run();
            ~ThriftClient();
        internal:
            ITransportFactory^ _factory;
            ConcurrentQueue<TransportHandler^>^ _requests;
            uv_loop_t* _loop;
            uv_async_t* _notifier;
            void* _thisHandle;
        };


        static void NotifyCallback(uv_async_t* notifier, int status);


        static GCHandle GCHandleFromVoidPtr(void* ptr);
        static void* GCHandleAllocNormal(Object^ object);
    }
}