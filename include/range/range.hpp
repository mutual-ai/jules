#ifndef JULES_RANGE_RANGE_H
#define JULES_RANGE_RANGE_H

#include <boost/range/adaptor/strided.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/istream_range.hpp>

#include <range/tokenized.hpp>

namespace jules
{
namespace range
{
using boost::make_iterator_range;
using boost::copy;
using boost::range_value;
using boost::size;
using boost::begin;
using boost::end;
} // namespace range

namespace adaptors
{
using boost::adaptors::strided;
using boost::adaptors::transformed;
} // namespace adaptors
} // namespace jules

#endif // JULES_RANGE_RANGE_H
