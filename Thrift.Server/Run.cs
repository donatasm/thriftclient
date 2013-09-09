using System;
using System.Threading;
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
        private const int TimeoutProbability = 15;
        private readonly object _syncRoot = new object();
        private readonly Random _random;
        private readonly TThreadPoolServer _server;

        public EchoServer()
        {
            _random = new Random();

            _server = new TThreadPoolServer(this,
                new TServerSocket(1337),
                new TFramedTransport.Factory(),
                new TFramedTransport.Factory(),
                new TBinaryProtocol.Factory(),
                new TBinaryProtocol.Factory(),
                256, 8192,
                Log);
        }

        public void Start()
        {
            _server.Serve();
        }

        public bool Process(TProtocol iprot, TProtocol oprot)
        {
            var received = iprot.ReadString();

            int random;

            lock (_syncRoot)
            {
                random = _random.Next(100) + 1;
            }

            var shouldTimeout = random <= TimeoutProbability;

            if (shouldTimeout)
            {
                Thread.Sleep(100);
                oprot.WriteString("Timeout");
            }
            else
            {
                oprot.WriteString(received);
            }

            oprot.Transport.Flush();

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
