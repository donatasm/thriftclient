using System;
using Thrift.Transport;

namespace Thrift.Client.Run
{
    internal static class Run
    {
        private static void Main()
        {
            using (var client = new ThriftClient(new TransportFactory()))
            {
                client.Send(transport =>
                    {
                        Console.WriteLine(transport.IsOpen);
                    });

                client.Run();
            }
        }

        private class TransportFactory : ITransportFactory
        {
            public TTransport Create()
            {
                return new TMemoryBuffer();
            }
        }
    }
}
