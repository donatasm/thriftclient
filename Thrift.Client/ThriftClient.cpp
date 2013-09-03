#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        ThriftClient::ThriftClient()
        {
            _loop = uv_loop_new();
        }


        ThriftClient::~ThriftClient()
        {
            uv_loop_delete(_loop);
        }
    }
}
