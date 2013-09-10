#include "Common.h"

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


        void OpenCompleted(uv_connect_t* connectRequest, int status)
        {
            FrameTransport^ transport = FrameTransport::FromPointer(connectRequest->data);
            delete connectRequest;

            if (status != 0)
            {
                transport->Close();
                UvException::Throw(status);
            }

            transport->_isOpen = true;
            transport->SendFrame();
        }


        void SendFrameCompleted(uv_write_t* writeRequest, int status)
        {
            FrameTransport^ transport = FrameTransport::FromPointer(writeRequest->data);
            delete writeRequest;

            if (status != 0)
            {
                transport->Close();
                UvException::Throw(status);
            }

            transport->_socketBuffer->socket.data = transport->ToPointer();

            transport->ReceiveFrame();
        }


        void ReceiveFrameCompleted(uv_stream_t* socket, ssize_t nread, const uv_buf_t* buffer)
        {
            FrameTransport^ transport = FrameTransport::FromPointer(socket->data);

            if (nread < 0)
            {
                transport->Close();
                UvException::Throw((int)nread);
            }

            if (transport->_position + nread > MAX_FRAME_SIZE)
            {
                transport->Close();
                throw gcnew TTransportException(String::Format("Maximum frame size {0} exceeded.", MAX_FRAME_SIZE));
            }

            // read frame header
            if (transport->_position < FRAME_HEADER_SIZE)
            {
                int header = 0;

                if (nread > 0)
                {
                    header |= (buffer->base[0] & 0xFF) << 24;
                }
                if (nread > 1)
                {
                    header |= (buffer->base[1] & 0xFF) << 16;
                }
                if (nread > 2)
                {
                    header |= (buffer->base[2] & 0xFF) << 8;
                }
                if (nread > 3)
                {
                    header |= (buffer->base[3] & 0xFF);
                }

                transport->_header = header;
            }

            transport->_position += (int)nread;

            // check if frame already received
            if (transport->_header == transport->_position - FRAME_HEADER_SIZE)
            {
                // stop reading
                int error = uv_read_stop((uv_stream_t*)&transport->_socketBuffer->socket);
                if (error != 0)
                {
                    transport->Close();
                    UvException::Throw(error);
                }

                transport->_position = FRAME_HEADER_SIZE;
                transport->_context->OutputProtocolCallback(transport->Protocol, nullptr);

                // prepare for the next frame to be written
                transport->_position = FRAME_HEADER_SIZE;
                transport->_header = 0;

                // if transport is still opened, return it to the pool
                if (transport->IsOpen)
                {
                    transport->_context->Client->TransportPool->Enqueue(transport);
                }
            }
        }


        void AllocateFrameBuffer(uv_handle_t* socket, size_t size, uv_buf_t* buffer)
        {
            FrameTransport^ transport = FrameTransport::FromPointer(socket->data);

            buffer->base = transport->_socketBuffer->buffer;
            buffer->len = MAX_FRAME_SIZE;
        }
    }
}