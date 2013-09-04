using System;
using Thrift.Protocol;
using Thrift.Transport;

namespace Thrift.Server
{
    internal static class Run
    {
        private static void Main()
        {
            new EchoServer().Start();
        }
    }

    internal sealed class EchoServer : TProcessor
    {
        private readonly TThreadPoolServer _server;

        public EchoServer()
        {
            _server = new TThreadPoolServer(this,
                new TServerSocket(1337),
                new TFramedTransport.Factory(),
                new TFramedTransport.Factory(),
                new TBinaryProtocol.Factory(),
                new TBinaryProtocol.Factory(),
                16, 128,
                Log);
        }

        public void Start()
        {
            _server.Serve();
        }

        public bool Process(TProtocol iprot, TProtocol oprot)
        {
            var received = iprot.ReadI32();
            LogFormat("Received '{0}'", received);

            for (var i = 0; i < 65536; i++)
            {
                oprot.WriteI32(i + 1);
            }

            oprot.Transport.Flush();

            LogFormat("Request processed!");

            return true;
        }

        private static void Log(string message)
        {
            LogFormat(message);
        }

        private static void LogFormat(string message, params object[] args)
        {
            Console.WriteLine("[Server Log] {0}", String.Format(message, args));
        }
    }
}
