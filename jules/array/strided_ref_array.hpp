// Copyright (c) 2017 Filipe Verri <filipeverri@gmail.com>

#ifndef JULES_ARRAY_STRIDED_REF_ARRAY_H
/// \exclude
#define JULES_ARRAY_STRIDED_REF_ARRAY_H

#include <jules/array/detail/common.hpp>
#include <jules/array/detail/iterator.hpp>
#include <jules/array/mapper.hpp>
#include <jules/array/meta/common.hpp>
#include <jules/array/slicing.hpp>
#include <jules/array/slicing/absolute.hpp>
#include <jules/array/strided_descriptor.hpp>
#include <jules/core/debug.hpp>
#include <jules/core/type.hpp>

namespace jules
{

template <typename T, std::size_t N, typename Mapper, typename... Indexes> class strided_ref_array_proxy
{
  static_assert(sizeof...(Indexes) < N);
  static_assert(Mapper::order == N);

public:
  strided_ref_array_proxy(T* data, Mapper mapper, Indexes... indexes)
    : data_{data}, mapper_{std::move(mapper)}, indexes_(std::forward<Indexes>(indexes)...)
  {
  }

  decltype(auto) operator[](index_t i) && { return at(i); }

  decltype(auto) operator[](absolute_slice slice) && { return at(slice); }

  decltype(auto) operator[](absolute_strided_slice slice) && { return at(slice); }

  template <typename Rng, typename = meta::requires<range::SizedRange<Rng>>> decltype(auto) operator[](const Rng& rng) &&
  {
    return at(rng);
  }

private:
  template <typename Index> decltype(auto) at(Index&& index) &&
  {
    return at(std::forward<Index>(index), std::make_index_sequence<sizeof...(Indexes)>());
  }

  template <typename Index, std::size_t... I> decltype(auto) at(Index&& index) &&
  {
    // clang-format off
    if constexpr (sizeof...(Indexes) == N - 1) {
      return detail::array_slice(
        data_, std::move(mapper_), std::get<I>(indexes_)..., std::forward<Index>(index));
    } else {
      return strided_ref_array_proxy<T, N, Mapper, Indexes..., Index>{
        data_, std::move(mapper_), std::get<I>(indexes_)..., std::forward<Index>(index)};
    }
    // clang-format on
  }

  T* data_;
  Mapper mapper_;
  std::tuple<Indexes...> indexes_;
};

/// Array with non-owned, non-continuous data.
///
/// This class represents a strided_ref view of an concrete array.
///
/// \module Array Types
/// \notes This class is not meant to be used in user code.
template <typename T, std::size_t N, typename Mapper>
class strided_ref_array : public common_array_base<strided_ref_array<T, N, Mapper>>, private Mapper
{
  static_assert(N > 0u, "invalid array dimension");
  static_assert(std::is_same_v<Mapper, detail::identity_mapper<N>> || std::is_same_v<Mapper, detail::vector_mapper<N>>);

  template <typename, std::size_t, typename> friend class strided_ref_array;

public:
  /// \group member_types Class Types and Constants
  ///
  /// (1) The order of dimensionality.
  ///
  /// (2) The type of the elements.
  ///
  /// (3) `ForwardIterator` over the elements.
  ///
  /// (4) Constant `ForwardIterator` over the elements.
  ///
  /// (5) Unsigned integer type that can store the dimensions of the array.
  ///
  /// (6) Signed integer type that can store differences between sizes.
  static constexpr auto order = N;

  /// \group member_types Class Types and Constants
  using value_type = T;

  /// \group member_types Class Types and Constants
  using iterator = detail::iterator_from_indexes<T, typename Mapper::iterator>;

  /// \group member_types Class Types and Constants
  using const_iterator = detail::iterator_from_indexes<const T, typename Mapper::iterator>;

  /// \group member_types Class Types and Constants
  using size_type = index_t;

  /// \group member_types Class Types and Constants
  using difference_type = distance_t;

  strided_ref_array(value_type* data, Mapper mapper) : Mapper{std::move(mapper)}, data_{data} {}

  ~strided_ref_array(){}; // not default to disable default copy, move, assignment, ...

  /// \group Assignment
  template <typename Array> auto operator=(const common_array_base<Array>& source) -> strided_ref_array&
  {
    detail::array_assign(*this, source);
    return *this;
  }

  /// \group Assignment
  template <typename U> auto operator=(const U& source) -> strided_ref_array&
  {
    static_assert(std::is_assignable<value_type&, U>::value, "incompatible assignment");
    range::fill(*this, source);
    return *this;
  }

  /// \group Assignment
  auto operator=(const strided_ref_array& source) -> strided_ref_array&
  {
    detail::array_assign(*this, source);
    return *this;
  }

  /// Implicitly convertable to hold const values.
  operator strided_ref_array<const value_type, order, Mapper>() const { return {data(), mapper()}; }

  /// \group Indexing
  decltype(auto) operator[](size_type i) { return detail::array_at(data(), mapper(), i); }

  /// \group Indexing
  decltype(auto) operator[](size_type i) const { return detail::array_at(data(), mapper(), i); }

  /// \group Indexing
  decltype(auto) operator[](absolute_slice slice) { return detail::array_at(data(), mapper(), slice); }

  /// \group Indexing
  decltype(auto) operator[](absolute_slice slice) const { return detail::array_at(data(), mapper(), slice); }

  /// \group Indexing
  decltype(auto) operator[](absolute_strided_slice slice) { return detail::array_at(data(), mapper(), slice); }

  /// \group Indexing
  decltype(auto) operator[](absolute_strided_slice slice) const { return detail::array_at(data(), mapper(), slice); }

  /// \group Indexing
  template <typename Rng, typename = meta::requires<range::SizedRange<Rng>>> decltype(auto) operator[](const Rng& rng)
  {
    return detail::array_at(data(), mapper(), rng);
  }

  /// \group Indexing
  template <typename Rng, typename = meta::requires<range::SizedRange<Rng>>> decltype(auto) operator[](const Rng& rng) const
  {
    return detail::array_at(data(), mapper(), rng);
  }

  auto begin() noexcept -> iterator { return {data(), this->index_begin()}; }
  auto end() noexcept -> iterator { return {data(), this->index_end()}; }

  auto begin() const noexcept -> const_iterator { return cbegin(); }
  auto end() const noexcept -> const_iterator { return cend(); }

  auto cbegin() const noexcept -> const_iterator { return {data(), this->index_begin()}; }
  auto cend() const noexcept -> const_iterator { return {data(), this->index_end()}; }

  auto size() const noexcept { return this->descriptor().size(); }

  auto length() const noexcept { return this->descriptor().length(); }

  auto row_count() const noexcept { return this->descriptor().row_count(); }

  auto column_count() const noexcept { return this->descriptor().column_count(); }

  auto dimensions() const noexcept -> std::array<size_type, order> { return this->descriptor().extents; }

protected:
  auto data() -> value_type* { return data_; }

  auto data() const -> const value_type* { return data_; }

  auto mapper() const -> const Mapper& { return *this; }

  strided_ref_array(const strided_ref_array& source) = default;
  strided_ref_array(strided_ref_array&& source) noexcept = default;

  /// \exclude
  value_type* const data_;
};

template <typename T, std::size_t N, typename Mapper>
auto eval(const strided_ref_array<T, N, Mapper>& source) -> const strided_ref_array<T, N, Mapper>&
{
  return source;
}

} // namespace jules

#endif // JULES_ARRAY_STRIDED_REF_ARRAY_H
