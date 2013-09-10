#include "Common.h"

namespace Thrift
{
    namespace Client
    {
        ThriftClient::ThriftClient()
        {
            _contextQueue = gcnew ContextQueue();
            TransportPool = gcnew Queue<FrameTransport^>();

            _loop = uv_loop_new();

            _notifier = new uv_async_t();
            _notifier->data = _contextQueue->ToPointer();

            uv_async_init(_loop, _notifier, NotifyCompleted);
        }


        ThriftClient::~ThriftClient()
        {
            delete _notifier;
            uv_loop_delete(_loop);
            delete _contextQueue;
        }


        void ThriftClient::Send(InputProtocol^ input, OutputProtocol^ output)
        {
            if (input == nullptr)
                throw gcnew ArgumentNullException("input");

            if (output == nullptr)
                throw gcnew ArgumentNullException("output");

            ThriftContext^ context = gcnew ThriftContext(input, output, this);

            _contextQueue->Enqueue(context);

            // notify that there are requests pending
            uv_async_send(_notifier);
        }


        void ThriftClient::Run()
        {
            uv_run(_loop, UV_RUN_DEFAULT);
        }
    }
}