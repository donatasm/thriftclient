#include "uv.h"
#using <system.dll>


#define FRAME_HEADER_SIZE 4 // Frame header size
#define MAX_FRAME_SIZE 65536 // Maximum size of a frame including headers


using namespace System;
using namespace System::IO;
using namespace System::Collections::Concurrent;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace Thrift::Protocol;
using namespace Thrift::Transport;


namespace Thrift
{
    namespace Client
    {
        // Thrift serialization handlers
        public delegate void InputProtocol(TProtocol^ inputProtocol);
        public delegate void OutputProtocol(TProtocol^ outputProtocol, Exception^ exception);


        ref class ContextQueue;
        ref class FrameTransport;
        public ref class ThriftClient sealed
        {
        public:
            ThriftClient();
            ~ThriftClient();
            void Send(InputProtocol^ input, OutputProtocol^ output);
            void Run();
        internal:
            initonly Queue<FrameTransport^>^ TransportPool;
        private:
            uv_loop_t* _loop;
            uv_async_t* _notifier;
            initonly ContextQueue^ _contextQueue;
        };


        public ref class UvException sealed : Exception
        {
        internal:
            UvException(String^ message);
            static UvException^ CreateFromLastError(int status);
            static void Throw(int status);
        };


        // Socket descriptor with it's buffer
        typedef struct
        {
            uv_tcp_t socket;
            char buffer[MAX_FRAME_SIZE];
        } SocketBuffer;


        private ref struct ThriftContext sealed
        {
        public:
            ThriftContext(InputProtocol^ input, OutputProtocol^ output, ThriftClient^ client);
            const char* Address;
            int Port;
            initonly ThriftClient^ Client;
            initonly InputProtocol^ InputProtocolCallback;
            initonly OutputProtocol^ OutputProtocolCallback;
        };


        private ref class ContextQueue sealed
        {
        public:
            ContextQueue();
            ~ContextQueue();
            void Enqueue(ThriftContext^ context);
            bool TryDequeue(ThriftContext^ %context);
            void* ToPointer();
            static ContextQueue^ FromPointer(void* ptr);
        private:
            GCHandle _handle;
            initonly ConcurrentQueue<ThriftContext^>^ _queue;
        };


        private ref class FrameTransport : TTransport
        {
        public:
            FrameTransport(const char* address, int port, uv_loop_t* loop);
            ~FrameTransport();
            virtual void Open() override;
            virtual void Close() override;
            virtual property bool IsOpen { bool get() override; }
            virtual int Read(array<byte>^ buf, int off, int len) override;
            virtual void Write(array<byte>^ buf, int off, int len) override;
            virtual void Flush() override;
            void* ToPointer();
            static FrameTransport^ FromPointer(void* ptr);
            void SendFrame();
            void ReceiveFrame();

            SocketBuffer* _socketBuffer;
            ThriftContext^ _context;
            initonly TBinaryProtocol^ Protocol;
            int _header;
            int _position;
            bool _isOpen;

        private:
            const char* _address;
            int _port;
            uv_loop_t* _loop;
            GCHandle _handle;
        };


        // Libuv callback handlers
        void NotifyCompleted(uv_async_t* notifier, int status);
        void OpenCompleted(uv_connect_t* connectRequest, int status);
        void SendFrameCompleted(uv_write_t* writeRequest, int status);
        void ReceiveFrameCompleted(uv_stream_t* socket, ssize_t nread, const uv_buf_t* buffer);
        void CloseCompleted(uv_handle_t* socket);
        void AllocateFrameBuffer(uv_handle_t* socket, size_t size, uv_buf_t* buffer);
    }
}
