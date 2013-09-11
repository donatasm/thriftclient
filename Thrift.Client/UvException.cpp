#include "ThriftClient.h"

namespace Thrift
{
    namespace Client
    {
        UvException::UvException(String^ message) : Exception(message)
        {
        }


        UvException^ UvException::CreateFromLastError(int error)
        {
            return gcnew UvException(gcnew String(uv_strerror(error)));
        }


        void UvException::Throw(int error)
        {
            throw CreateFromLastError(error);
        }
    }
}