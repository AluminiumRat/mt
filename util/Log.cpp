#include <util/Log.h>

using namespace mt;

std::mutex Log::_writeMutex;
std::ostream* Log::_stream = &std::cout;

void Log::setStream(std::ostream& stream)
{
  _stream = &stream;
}
