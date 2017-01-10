// Copyright (c) 2016 Filipe Verri <filipeverri@gmail.com>

#ifndef JULES_DATAFRAME_DATAFRAME_H
#define JULES_DATAFRAME_DATAFRAME_H

#include <jules/core/debug.hpp>
#include <jules/core/range.hpp>
#include <jules/core/type.hpp>
#include <jules/dataframe/column.hpp>

#include <algorithm>
#include <unordered_map>

namespace jules
{

template <typename Coercion> class base_dataframe
{
public:
  using column_type = base_column<Coercion>;
  struct named_column_type {
    string name;
    column_type column;
  };

  base_dataframe() = default;

  base_dataframe(std::initializer_list<named_column_type> elements)
    : base_dataframe(elements.begin(), elements.end(), elements.size())
  {
  }

  template <typename Rng, typename R = range::range_value_t<Rng>,
            typename = std::enable_if_t<std::is_convertible<R, named_column_type>::value>, CONCEPT_REQUIRES_(range::Range<Rng>())>
  base_dataframe(const Rng& rng) : base_dataframe(range::begin(rng), range::end(rng), range::size(rng))
  {
  }

  template <typename Iter, typename Sent, typename R = range::iterator_value_t<Iter>,
            typename = std::enable_if_t<std::is_convertible<R, named_column_type>::value>,
            CONCEPT_REQUIRES_(range::Sentinel<Sent, Iter>() && range::InputIterator<Iter>())>
  base_dataframe(Iter first, Sent last) : base_dataframe(first, last, 0u)
  {
  }

  template <typename Iter, typename Sent, typename R = range::iterator_value_t<Iter>,
            typename = std::enable_if_t<std::is_convertible<R, named_column_type>::value>,
            CONCEPT_REQUIRES_(range::Sentinel<Sent, Iter>() && !range::InputIterator<Iter>())>
  base_dataframe(Iter first, Sent last) : base_dataframe(first, last, range::distance(first, last))
  {
  }

  template <typename Iter, typename Sent, typename R = range::iterator_value_t<Iter>,
            typename = std::enable_if_t<std::is_convertible<R, named_column_type>::value>,
            CONCEPT_REQUIRES_(range::Sentinel<Sent, Iter>())>
  base_dataframe(Iter first, Sent last, index_t size_hint)
  {
    elements_.reserve(size_hint);

    elements_.push_back(*first);
    row_count_ = elements_.back().column.size();

    auto ok = std::all_of(++first, last, [this](auto&& column) {
      elements_.push_back(std::forward<decltype(column)>(column));
      return elements_.back().column.size() == row_count_;
    });

    DEBUG_ASSERT(ok, debug::throwing_module, debug::level::invalid_argument, "columns size mismatch");

    index_t i = 0;
    for (auto& element : elements_) {
      auto& name = element.name;
      if (!name.empty()) {
        DEBUG_ASSERT(indexes_.find(name) == indexes_.end(), debug::throwing_module, debug::level::invalid_argument,
                     "repeated column name");
        indexes_[name] = i++;
      }
    }
  }

  base_dataframe(const base_dataframe& source) = default;
  base_dataframe(base_dataframe&& source) noexcept = default;

  auto operator=(const base_dataframe& source) -> base_dataframe& = default;
  auto operator=(base_dataframe&& source) noexcept -> base_dataframe& = default;

  operator bool() const { return column_count() > 0; }

  auto row_count() const { return row_count_; }
  auto column_count() const { return elements_.size(); }

private:
  index_t row_count_ = 0u;
  std::vector<named_column_type> elements_;
  std::unordered_map<string, index_t> indexes_;
};

using dataframe = base_dataframe<coercion_rules>;

} // namespace jules

#endif // JULES_DATAFRAME_DATAFRAME_H

#ifndef JULES_DATAFRAME_DATAFRAME_H

#include <jules/dataframe/dataframe_decl.hpp>
#include <jules/range/range.hpp>

#include <algorithm>
#include <iterator>
#include <regex>

namespace jules
{

template <typename Coercion> template <typename T> auto base_dataframe<Coercion>::columns() -> vector<base_column_view<T>>
{
  using namespace adaptors;
  return {(columns_ | transformed([](auto& column) { return as_view<T>(column); })).begin(), columns_.size()};
}

template <typename Coercion>
template <typename T>
auto base_dataframe<Coercion>::columns() const -> vector<base_column_view<const T>>
{
  using namespace adaptors;
  return {(columns_ | transformed([](const auto& column) { return as_view<T>(column); })).begin(), columns_.size()};
}

template <typename Coercion> template <typename... T> auto base_dataframe<Coercion>::rows() -> vector<base_row_view<T...>>
{
  if (sizeof...(T) != this->columns_count())
    throw std::runtime_error{"invalid number of columns inferred from column types"};

  auto tuple = this->views<T...>(std::make_index_sequence<sizeof...(T)>());
  return rows_helper(tuple, std::make_index_sequence<sizeof...(T)>(), this->rows_count());
}

template <typename Coercion>
template <typename... T>
auto base_dataframe<Coercion>::rows() const -> vector<base_row_view<const T...>>
{
  if (sizeof...(T) != this->columns_count())
    throw std::runtime_error{"invalid number of columns inferred from column types"};

  auto tuple = this->views<const T...>(std::make_index_sequence<sizeof...(T)>());
  return rows_helper(tuple, std::make_index_sequence<sizeof...(T)>(), this->rows_count());
}

template <typename Coercion>
template <typename... T, std::size_t... I>
auto base_dataframe<Coercion>::views(std::index_sequence<I...>) -> std::tuple<base_column_view<T>...>
{
  return std::make_tuple(as_view<T>(this->columns_[I])...);
}

template <typename Coercion>
template <typename... T, std::size_t... I>
auto base_dataframe<Coercion>::views(std::index_sequence<I...>) const -> std::tuple<base_column_view<const T>...>
{
  return std::make_tuple(as_view<const T>(this->columns_[I])...);
}

template <typename Coercion>
template <typename... T, std::size_t... I>
auto base_dataframe<Coercion>::rows_helper(std::tuple<base_column_view<T>...>& columns, std::index_sequence<I...>,
                                           std::size_t rows_count) -> vector<base_row_view<T...>>
{
  auto i = std::size_t{};
  auto iter = jules::as_iterator([&] {
    auto row_view = base_row_view<T...>{std::get<I>(columns)[i]...};
    ++i;
    return row_view;
  });

  return {iter, rows_count};
}

template <typename Coercion> base_dataframe<Coercion>& base_dataframe<Coercion>::colbind(const column_t& column)
{
  const auto& name = column.name();

  if (!name.empty()) {
    auto it = colindexes_.find(name);
    if (it != colindexes_.end())
      throw std::runtime_error{"column already exists"};
  }

  if (!is_null() && rows_count() != column.size())
    throw std::runtime_error{"invalid column size"};

  if (is_null())
    nrow_ = column.size();

  columns_.push_back(column);
  if (!name.empty())
    colindexes_[name] = columns_.size();

  return *this;
}

template <typename Coercion> base_dataframe<Coercion>& base_dataframe<Coercion>::colbind(column_t&& column)
{
  auto& name = column.name();
  if (!name.empty()) {
    auto it = colindexes_.find(name);
    if (it != colindexes_.end())
      throw std::runtime_error{"column already exists"};
  }

  if (!is_null() && rows_count() != column.size())
    throw std::runtime_error{"invalid column size"};

  if (is_null())
    nrow_ = column.size();

  if (!name.empty())
    colindexes_[name] = columns_.size();
  columns_.push_back(std::move(column));

  return *this;
}

template <typename Coercion> auto base_dataframe<Coercion>::columns_elements_types() const -> vector<std::type_index>
{
  return to_vector<std::type_index>(columns_ | adaptors::transformed([](const column_t& col) { return col.elements_type(); }));
}

template <typename Coercion> auto base_dataframe<Coercion>::columns_names() const -> vector<std::string>
{
  return to_vector<std::string>(columns_ | adaptors::transformed([](const column_t& col) { return col.name(); }));
}

template <typename Coercion> auto base_dataframe<Coercion>::select(const std::string& name) const -> const column_t&
{
  auto it = colindexes_.find(name);
  if (it == colindexes_.end())
    throw std::out_of_range{"column does not exists"};
  return columns_.at(it->second);
}

template <typename Coercion> auto base_dataframe<Coercion>::select(const expr_t& expression) const -> column_t
{
  return expression.extract_from(*this);
}

template <typename Coercion> auto base_dataframe<Coercion>::select(const expr_list_t& expression_list) const -> base_dataframe
{
  return expression_list.extract_from(*this);
}

template <typename Coercion> template <typename T> base_dataframe<Coercion>& base_dataframe<Coercion>::coerce_to()
{
  std::vector<column_t> new_columns;

  new_columns.reserve(columns_.size());
  std::transform(columns_.begin(), columns_.end(), std::back_inserter(new_columns),
                 [](auto&& column) { return jules::as_column<T>(column); });

  columns_ = std::move(new_columns);

  return *this;
}

template <typename T, typename C> auto coerce_to(const base_dataframe<C>& dataframe) -> base_dataframe<C>
{
  base_dataframe<C> coerced;
  for (std::size_t i = 0; dataframe.columns_count(); ++i)
    coerced.colbind(jules::as_column<T>(dataframe.select(i)));
  return coerced;
}

template <typename Coercion> template <typename T> bool base_dataframe<Coercion>::can_coerce_to() const
{
  for (auto&& column : columns_)
    if (!column.template can_coerce_to<T>())
      return false;
  return true;
}

template <typename T, typename Coercion> bool can_coerce_to(const base_dataframe<Coercion>& dataframe)
{
  return dataframe.template can_coerce_to<T>();
}

template <typename Coercion> auto base_dataframe<Coercion>::read(std::istream& is, dataframe_read_options opt) -> base_dataframe
{
  using namespace adaptors;
  using namespace range;

  if (!is)
    return {};

  const auto as_range = [](auto&& match) { return make_iterator_range(match.first, match.second); };

  std::string raw_data;
  raw_data.assign(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());

  std::vector<std::sub_match<std::string::iterator>> data;
  std::size_t ncol = 0;

  auto line_range = raw_data | tokenized(opt.line.regex, opt.line.separator ? -1 : 0, opt.line.flag);

  std::size_t size = 0;
  for (auto&& line : line_range) {
    if (line.first == line.second)
      continue;

    copy(as_range(line) | tokenized(opt.cell.regex, opt.cell.separator ? -1 : 0, opt.cell.flag), std::back_inserter(data));

    if (ncol == 0)
      ncol = data.size() - size;

    if (ncol != 0 && data.size() - size != ncol)
      throw std::runtime_error{"number of columns differ"};

    size = data.size();
  }

  if (ncol == 0)
    return {};

  base_dataframe<Coercion> df;
  if (opt.header && data.size() / ncol == 1) {
    for (std::size_t j = 0; j < ncol; ++j) {
      base_column<Coercion> col(std::string{data[j].first, data[j].second}, std::string{}, 0);
      df.colbind(std::move(col));
    }
    return df;
  }

  for (std::size_t j = 0; j < ncol; ++j) {
    auto column_data =
      make_iterator_range(data.begin() + j + (opt.header ? ncol : 0), data.end()) | strided(ncol) | transformed([](auto&& match) {
        return std::move(std::string{match.first, match.second});
      });
    base_column<Coercion> col(opt.header ? std::string{data[j].first, data[j].second} : std::string{}, column_data);
    df.colbind(std::move(col));
  }
  return df;
}

template <typename C> auto write(const base_dataframe<C>& df, std::ostream& os, dataframe_write_options opt) -> std::ostream&
{
  if (df.rows_count() == 0 || df.columns_count() == 0)
    return os;

  std::vector<base_column<C>> coerced;
  std::vector<column_view<const std::string>> data;

  for (std::size_t j = 0; j < df.columns_count(); ++j) {
    const auto& col = df.select(j);

    if (col.elements_type() == typeid(std::string)) {
      data.push_back(jules::as_view<std::string>(col));
    } else {
      coerced.push_back(std::move(jules::as_column<std::string>(col)));
      data.push_back(jules::as_view<std::string>(coerced.back()));
    }
  }

  if (opt.header) {
    opt.cell.data(os, df.select(0).name());
    for (std::size_t j = 1; j < df.columns_count(); ++j)
      opt.cell.data(opt.cell.separator(os), df.select(j).name());
    opt.line.separator(os);
  }

  for (std::size_t i = 0; i < df.rows_count(); ++i) {
    opt.cell.data(os, data[0][i]);
    for (std::size_t j = 1; j < df.columns_count(); ++j)
      opt.cell.data(opt.cell.separator(os), data[j][i]);
    opt.line.separator(os);
  }

  return os;
}

template <typename C> auto write(const base_dataframe<C>& df, std::ostream& os) -> std::ostream& { return write(df, os, {}); };

template <typename C> auto operator<<(std::ostream& os, const base_dataframe<C>& df) -> std::ostream&
{
  return write(df, os, {});
}

} // namespace jules

#endif // JULES_DATAFRAME_DATAFRAME_H
