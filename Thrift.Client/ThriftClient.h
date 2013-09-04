#include "uv.h"

using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::Runtime::InteropServices;
using namespace Thrift::Protocol;
using namespace Thrift::Transport;

namespace Thrift
{
    namespace Client
    {
        public delegate void RequestHandler(TProtocol^);
		

        public ref class ThriftClient sealed
        {
        public:
            ThriftClient();
            void Send(RequestHandler^ request);
            void Run();
            ~ThriftClient();
        internal:
            ConcurrentQueue<RequestHandler^>^ _requests;
		private:
            uv_loop_t* _loop;
            uv_async_t* _notifier;
            void* _thisHandle;
        };


        static void NotifyCallback(uv_async_t* notifier, int status);


        static GCHandle GCHandleFromVoidPtr(void* ptr);
        static void* GCHandleAllocNormal(Object^ object);
    }
}