#ifndef JULES_DATAFRAME_DATAFRAME_DECL_H
#define JULES_DATAFRAME_DATAFRAME_DECL_H

#include "array/array.hpp"
#include "dataframe/column.hpp"
#include "dataframe/io_decl.hpp"
#include "formula/expression_decl.hpp"

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace jules
{
template <typename Coercion> class base_dataframe
{
  private:
    using expr_t = base_expr<Coercion, void>;
    using expr_list_t = base_expr_list<Coercion>;

  public:
    using column_t = base_column<Coercion>;

    base_dataframe() = default;
    base_dataframe(std::initializer_list<column_t> columns);

    base_dataframe(const base_dataframe& source) = default;
    base_dataframe(base_dataframe&& source) = default;

    base_dataframe& operator=(const base_dataframe& source) = default;
    base_dataframe& operator=(base_dataframe&& source) = default;

    [[deprecated("use colbind")]] base_dataframe& cbind(const column_t& column);
    [[deprecated("use colbind")]] base_dataframe& cbind(column_t&& column);

    vector<std::string> colnames() const;

    const column_t& select(std::size_t i) const { return columns_.at(i); }
    const column_t& select(const std::string& name) const;

    column_t select(const expr_t& expression) const;
    base_dataframe select(const expr_list_t& expression_list) const;

    bool empty() const { return ncol() == 0; }

    std::size_t nrow() const { return nrow_; }
    std::size_t ncol() const { return columns_.size(); }

    static base_dataframe read(std::istream& is,
                               const dataframe_storage_options& opt = {R"(\t)", R"(\n)", true});
    static void write(const base_dataframe& df, std::ostream& os,
                      const dataframe_storage_options& opt = {"\t", "\n", true});

  private:
    std::size_t nrow_;

    std::vector<column_t> columns_;
    std::unordered_map<std::string, std::size_t> colindexes_;
};

using dataframe = base_dataframe<default_coercion_rules>;

} // namespace jules

#endif // JULES_DATAFRAME_DATAFRAME_DECL_H
