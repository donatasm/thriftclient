#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftContext::ThriftContext(RequestTransport^ requestTransport, ResponseTransport^ responseTransport)
        {
            RequestTransportCallback = requestTransport;
            ResponseTransportCallback = responseTransport;
            Address = "127.0.0.1";
            Port = 1337;
        }


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


        ThriftClient::ThriftClient()
        {
            _contextQueue = gcnew ContextQueue();

            _loop = uv_loop_new();

            _notifier = new uv_async_t();
            _notifier->data = _contextQueue->ToPointer();

            uv_async_init(_loop, _notifier, NotifyCallback);
        }


        ThriftClient::~ThriftClient()
        {
            delete _notifier;
            uv_loop_delete(_loop);
            delete _contextQueue;
        }


        void ThriftClient::Send(RequestTransport^ requestTransport, ResponseTransport^ responseTransport)
        {
            if (requestTransport == nullptr)
                throw gcnew ArgumentNullException("requestTransport");

            if (responseTransport == nullptr)
                throw gcnew ArgumentNullException("responseTransport");

            ThriftContext^ context = gcnew ThriftContext(requestTransport, responseTransport);

            _contextQueue->Enqueue(context);

            // notify that there are requests pending
            uv_async_send(_notifier);
        }


        void ThriftClient::Run()
        {
            uv_run(_loop, UV_RUN_DEFAULT);
        }


        UvException::UvException(String^ message) : Exception(message)
        {
        }


        UvException^ UvException::CreateFromLastError(int error)
        {
            return gcnew UvException(gcnew String(uv_strerror(error)));
        }


        void UvException::Throw(int error)
        {
            throw CreateFromLastError(error);
        }


        void NotifyCallback(uv_async_t* notifier, int status)
        {
            ContextQueue^ contextQueue = ContextQueue::FromPointer(notifier->data);

            ThriftContext^ context;

            while (contextQueue->TryDequeue(context))
            {
                // execute request callback
                context->RequestTransportCallback(gcnew TMemoryBuffer());

                Open(context, notifier->loop);
            }
        }


        void Open(ThriftContext^ context, uv_loop_t* loop)
        {
            struct sockaddr_in address;

            int error;

            error = uv_ip4_addr(context->Address, context->Port, &address);
            if (error != 0)
            {
                UvException::Throw(error);
            }

            int contentLength = 4; // frame content length
            TFrame* frame = new TFrame();
            ResetFrame(frame);
            // inject frame header
            frame->buffer[0] = (0xFF & (contentLength >> 24));
            frame->buffer[1] = (0xFF & (contentLength >> 16));
            frame->buffer[2] = (0xFF & (contentLength >> 8));
            frame->buffer[3] = (0xFF & (contentLength));
            // set frame body
            frame->buffer[4] = 1;
            frame->buffer[5] = 2;
            frame->buffer[6] = 3;
            frame->buffer[7] = 4;
            // set position
            frame->position = 8;

            error = uv_tcp_init(loop, &frame->socket);
            if (error != 0)
            {
                delete frame;
                UvException::Throw(error);
            }

            uv_connect_t* connectRequest = new uv_connect_t();
            connectRequest->data = frame;

            error = uv_tcp_connect(connectRequest, &frame->socket, (const sockaddr*)&address, OpenCallback);
            if (error != 0)
            {
                delete connectRequest;
                delete frame;
                UvException::Throw(error);
            }
        }


        void OpenCallback(uv_connect_t* connectRequest, int status)
        {
            TFrame* frame = (TFrame*)connectRequest->data;
            delete connectRequest;

            if (status != 0)
            {
                delete frame;
                UvException::Throw(status);
            }

            SendFrame(frame);
        }


        void SendFrame(TFrame* frame)
        {
            uv_write_t* writeRequest = new uv_write_t();
            writeRequest->data = frame;

            uv_buf_t buffer = InitFrameBuffer(frame);

            int error = uv_write(writeRequest, (uv_stream_t*)&frame->socket, &buffer, 1, SendFrameCallback);
            if (error != 0)
            {
                delete writeRequest;
                delete frame;
                UvException::Throw(error);
            }
        }


        void SendFrameCallback(uv_write_t* writeRequest, int status)
        {
            TFrame* frame = (TFrame*)writeRequest->data;
            delete writeRequest;

            if (status != 0)
            {
                delete frame;
                UvException::Throw(status);
            }

            frame->socket.data = frame;

            ReceiveFrame(frame);
        }


        void ReceiveFrame(TFrame* frame)
        {
            ResetFrame(frame);

            int error = uv_read_start((uv_stream_t*)&frame->socket, AllocateFrameBuffer, ReceiveFrameCallback);
            if (error != 0)
            {
                delete frame;
                UvException::Throw(error);
            }
        }


        void ReceiveFrameCallback(uv_stream_t* socket, ssize_t nread, const uv_buf_t* buffer)
        {
            TFrame* frame = (TFrame*)socket->data;

            if (nread < 0)
            {
                delete frame;
                UvException::Throw(nread);
            }

            // read frame header
            if (frame->position < FRAME_HEADER_SIZE)
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

                frame->header = header;
            }

            frame->position += nread;

            if (frame->header == frame->position - FRAME_HEADER_SIZE)
            {
                // TODO: handle frame completed
                Console::WriteLine("Header: {0} Position {1}", frame->header, frame->position);
            }
        }


        void AllocateFrameBuffer(uv_handle_t* socket, size_t size, uv_buf_t* buffer)
        {
            TFrame* frame = (TFrame*)socket->data;

            buffer->base = frame->buffer;
            buffer->len = MAX_FRAME_SIZE;
        }


        uv_buf_t InitFrameBuffer(TFrame* frame)
        {
            uv_buf_t buffer;

            buffer.base = frame->buffer;
            buffer.len = frame->position;

            return buffer;
        }


        void ResetFrame(TFrame* frame)
        {
            frame->header = -1;
            frame->position = 0;
        }
    }
}