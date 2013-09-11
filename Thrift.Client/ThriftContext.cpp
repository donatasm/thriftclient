#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftContext::ThriftContext(InputProtocol^ input, OutputProtocol^ output, ThriftClient^ client)
        {
            InputProtocolCallback = input;
            OutputProtocolCallback = output;
            Address = "127.0.0.1";
            Port = 1337;
            Client = client;
        }
    }
}