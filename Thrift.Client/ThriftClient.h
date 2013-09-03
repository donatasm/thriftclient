#include "uv.h"


namespace Thrift
{
    namespace Client
    {
        public ref class ThriftClient sealed
        {
        public:
            ThriftClient();
            ~ThriftClient();
        private:
            uv_loop_t* _loop;
        };
    }
}