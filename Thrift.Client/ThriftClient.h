#include "uv.h"

using namespace Thrift::Transport;

namespace Thrift
{
    namespace Client
    {
        public interface class ITransportFactory
        {
            TTransport^ Create();
        };

        public ref class ThriftClient sealed
        {
        public:
            ThriftClient(ITransportFactory^ factory);
            ~ThriftClient();
        private:
            uv_loop_t* _loop;
        };
    }
}