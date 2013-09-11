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
                const char* address = context->_address;
                int port = context->_port;

                FrameTransport^ transport;
                Queue<FrameTransport^>^ transportPool = context->_client->_transportPool;

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
                context->_input(transport->_protocol);
            }
        }


        void StopCompleted(uv_async_t* stop, int status)
        {
            uv_stop(stop->loop);
        }


        ThriftClient::ThriftClient()
        {
            _contextQueue = gcnew ContextQueue();
            _transportPool = gcnew Queue<FrameTransport^>();

            _loop = uv_loop_new();

            _notifier = new uv_async_t();
            _notifier->data = _contextQueue->ToPointer();
            uv_async_init(_loop, _notifier, NotifyCompleted);

            _stop = new uv_async_t();
            uv_async_init(_loop, _stop, StopCompleted);
        }


        ThriftClient::~ThriftClient()
        {
            uv_loop_delete(_loop);

            delete _notifier;
            delete _stop;
            delete _contextQueue;

            while (_transportPool->Count > 0)
            {
                FrameTransport^ transport = _transportPool->Dequeue();
                transport->Close();
            }
        }


        void ThriftClient::Stop()
        {
            uv_async_send(_stop);
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


        void ThriftClient::RunAsync()
        {
            Thread^ loopThread = gcnew Thread(gcnew ThreadStart(this, &ThriftClient::RunInternal));
            loopThread->IsBackground = true;
            loopThread->Start();
        }


        void ThriftClient::RunInternal()
        {
            this->Run();
            delete this;
        }
    }
}