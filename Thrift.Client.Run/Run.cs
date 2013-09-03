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
                
            }
        }

        private class TransportFactory : ITransportFactory
        {
            public TTransport Create()
            {
                throw new NotImplementedException();
            }
        }
    }
}
