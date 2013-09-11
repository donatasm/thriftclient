#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        void NotifyCompleted(uv_async_t* notifier, int status)
        {
            ContextQueue^ contextQueue = ContextQueue::FromPointer(notifier->data);

            ThriftContext^ context;

            while (contextQueue->TryDequeue(context))
            {
                const char* address = context->Address;
                int port = context->Port;

                FrameTransport^ transport;
                Queue<FrameTransport^>^ transportPool = context->Client->TransportPool;

                if (transportPool->Count > 0)
                {
                    transport = transportPool->Dequeue();
                }
                else
                {
                    transport = gcnew FrameTransport(address, port, notifier->loop);
                }

                transport->_context = context;

                // execute request callback
                context->InputProtocolCallback(transport->Protocol);
            }
        }


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