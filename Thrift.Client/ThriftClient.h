#pragma once
#include "uv.h"

#define FRAME_HEADER_SIZE 4 // frame header size
#define MAX_FRAME_SIZE 65536 // maximum size of a frame including headers

using namespace System;
using namespace System::IO;
using namespace System::Collections::Concurrent;
using namespace System::Runtime::InteropServices;
using namespace Thrift::Protocol;
using namespace Thrift::Transport;

namespace Thrift
{
    namespace Client
    {
        public delegate void RequestTransport(TTransport^);
        public delegate void ResponseTransport(TTransport^, Exception^);


        private ref struct ThriftContext sealed
        {
        public:
            ThriftContext(RequestTransport^ requestTransport, ResponseTransport^ responseTransport);
            initonly RequestTransport^ RequestTransportCallback;
            initonly ResponseTransport^ ResponseTransportCallback;
            const char* Address;
            int Port;
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


        public ref class ThriftClient sealed
        {
        public:
            ThriftClient();
            ~ThriftClient();
            void Send(RequestTransport^ requestTransport, ResponseTransport^ responseTransport);
            void Run();
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


        typedef struct
        {
            uv_tcp_t socket;
            int header;
            int position;
            char buffer[MAX_FRAME_SIZE];
        } TFrame;


        void NotifyCallback(uv_async_t* notifier, int status);
        void Open(ThriftContext^ context, uv_loop_t* loop);
        void OpenCallback(uv_connect_t* connectRequest, int status);
        void SendFrame(TFrame* frame);
        void SendFrameCallback(uv_write_t* writeRequest, int status);
        void ReceiveFrame(TFrame* frame);
        void ReceiveFrameCallback(uv_stream_t* socket, ssize_t nread, const uv_buf_t* buffer);
        void AllocateFrameBuffer(uv_handle_t* socket, size_t size, uv_buf_t* buffer);
        uv_buf_t InitFrameBuffer(TFrame* frame);
        void ResetFrame(TFrame* frame);
    }
}