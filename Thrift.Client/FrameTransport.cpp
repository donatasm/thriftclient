#include "Common.h"

namespace Thrift
{
    namespace Client
    {
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

            Protocol = gcnew TBinaryProtocol(this);
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

            // TODO: close socket too

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
    }
}