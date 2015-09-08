#ifndef JULES_ARRAY_ARRAY_H
#define JULES_ARRAY_ARRAY_H

#include "array/array_decl.hpp"

namespace jules
{
template <typename T>
ndarray<T, 1>::ndarray(std::size_t size)
    : detail::base_ndarray<T, 1>(std::array<std::size_t, 1>{{size}})
{
}

template <typename T>
template <typename Range, typename R>
ndarray<T, 1>::ndarray(Range&& rng)
    : detail::base_ndarray<T, 1>(std::array<std::size_t, 1>{{static_cast<std::size_t>(range::size(rng))}})
{
    range::copy(rng, std::begin(this->data_));
}

template <typename T>
ndarray<T, 1>::ndarray(const T* data, std::size_t size)
    : detail::base_ndarray<T, 1>({{size}}, data, size)
{
}

template <typename T>
ndarray<T, 1>::ndarray(const T& value, std::size_t size)
    : detail::base_ndarray<T, 1>({{size}}, value, size)
{
}

} // namespace jules

#endif // JULES_ARRAY_ARRAY_H
