#include <exception>
#include <boost/throw_exception.hpp>
#include <boost/assert/source_location.hpp>

namespace boost {

void throw_exception(std::exception const & e, boost::source_location const &) {
    throw e;
}

void throw_exception(std::exception const & e) {
    throw e;
}

} // namespace boost
