#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftContext::ThriftContext(InputProtocol^ input, OutputProtocol^ output, ThriftClient^ client)
        {
            _input = input;
            _output = output;
            _address = "127.0.0.1";
            _port = 1337;
            _client = client;
        }
    }
}