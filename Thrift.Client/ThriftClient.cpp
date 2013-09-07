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

            uv_async_init(_loop, _notifier, NotifyCompleted);
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


        FrameTransport::FrameTransport(const char* address, int port, uv_loop_t* loop)
        {
            _handle = GCHandle::Alloc(this);

            _address = address;
            _port = port;
            _loop = loop;
            _position = FRAME_HEADER_SIZE; // reserve first bytes for frame header
            _header = 0;
            _isOpen = false;
            _socketBuffer = new SocketBuffer();
        }


        FrameTransport::~FrameTransport()
        {
            Close();
        }


        void FrameTransport::Open()
        {
            struct sockaddr_in address;

            int error;

            error = uv_ip4_addr(_address, _port, &address);
            if (error != 0)
            {
                UvException::Throw(error);
            }

            error = uv_tcp_init(_loop, &_socketBuffer->socket);
            if (error != 0)
            {
                UvException::Throw(error);
            }

            uv_connect_t* connectRequest = new uv_connect_t();
            connectRequest->data = this->ToPointer();

            error = uv_tcp_connect(connectRequest, &_socketBuffer->socket, (const sockaddr*)&address, OpenCompleted);
            if (error != 0)
            {
                delete connectRequest;
                UvException::Throw(error);
            }
        }


        void FrameTransport::Close()
        {
            if (_handle.IsAllocated)
            {
                _handle.Free();
            }

            if (_socketBuffer != NULL)
            {
                delete _socketBuffer;
                _socketBuffer = NULL;
            }

            _isOpen = false;
        }


        bool FrameTransport::IsOpen::get()
        {
            return _isOpen;
        }


        int FrameTransport::Read(array<byte>^ buf, int off, int len)
        {
            int left = _header - _position + FRAME_HEADER_SIZE;
            int read;

            if (left < len)
            {
                read = left;
            }
            else
            {
                read = len;
            }

            if (read > 0)
            {
                IntPtr source = IntPtr::IntPtr(_socketBuffer->buffer) + _position;

                Marshal::Copy(source, buf, off, read);

                _position += read;

                return read;
            }

            return 0;
        }


        void FrameTransport::Write(array<byte>^ buf, int off, int len)
        {
            if (_position + len > MAX_FRAME_SIZE)
            {
                throw gcnew TTransportException(String::Format("Maximum frame size {0} exceeded.", MAX_FRAME_SIZE));
            }

            IntPtr destination = IntPtr::IntPtr(_socketBuffer->buffer) + _position;

            Marshal::Copy(buf, off, destination, len);
            
            _position += len;
        }


        void FrameTransport::Flush()
        {
            if (!_isOpen)
            {
                Open();
            }
            else
            {
                SendFrame();
            }
        }


        void* FrameTransport::ToPointer()
        {
            return GCHandle::ToIntPtr(_handle).ToPointer();
        }


        FrameTransport^ FrameTransport::FromPointer(void* ptr)
        {
            GCHandle handle = GCHandle::FromIntPtr(IntPtr(ptr));
            return (FrameTransport^)handle.Target;
        }


        void FrameTransport::SendFrame()
        {
            _header = _position - FRAME_HEADER_SIZE;

            _socketBuffer->buffer[0] = (0xFF & (_header >> 24));
            _socketBuffer->buffer[1] = (0xFF & (_header >> 16));
            _socketBuffer->buffer[2] = (0xFF & (_header >> 8));
            _socketBuffer->buffer[3] = (0xFF & (_header));

            uv_write_t* writeRequest = new uv_write_t();
            writeRequest->data = this->ToPointer();

            uv_buf_t buffer;
            buffer.base = _socketBuffer->buffer;
            buffer.len = _position;

            int error = uv_write(writeRequest, (uv_stream_t*)&_socketBuffer->socket, &buffer, 1, SendFrameCompleted);
            if (error != 0)
            {
                delete writeRequest;
                UvException::Throw(error);
            }
        }

        
        void FrameTransport::ReceiveFrame()
        {
            _position = 0;
            _header = 0;

            int error = uv_read_start((uv_stream_t*)&_socketBuffer->socket, AllocateFrameBuffer, ReceiveFrameCompleted);
            if (error != 0)
            {
                UvException::Throw(error);
            }
        }


        void NotifyCompleted(uv_async_t* notifier, int status)
        {
            ContextQueue^ contextQueue = ContextQueue::FromPointer(notifier->data);

            ThriftContext^ context;

            while (contextQueue->TryDequeue(context))
            {
                const char* address = context->Address;
                int port = context->Port;

                FrameTransport^ transport = gcnew FrameTransport(address, port, notifier->loop);

                transport->_context = context;

                // execute request callback
                context->RequestTransportCallback(transport);
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

                // TODO: check header content if it fits in frame
            }

            transport->_position += (int)nread;

            if (transport->_header == transport->_position - FRAME_HEADER_SIZE)
            {
                transport->_position = FRAME_HEADER_SIZE;
                transport->_context->ResponseTransportCallback(transport, nullptr);
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