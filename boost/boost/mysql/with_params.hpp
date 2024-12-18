//
// Copyright (c) 2019-2024 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_WITH_PARAMS_HPP
#define BOOST_MYSQL_WITH_PARAMS_HPP

#include <boost/mysql/constant_string_view.hpp>

#include <boost/mysql/detail/format_sql.hpp>

#include <tuple>
#include <utility>

namespace boost {
namespace mysql {

/**
 * \brief Type trait that applies the transformation performed by `std::make_tuple` to a single element.
 * \details
 * For example: \n
 *   - `make_tuple_element_t<int>` yields `int` \n
 *   - `make_tuple_element_t<const int&>` yields `int` \n
 *   - `make_tuple_element_t<std::reference_wrapper<int>>` yields `int&` \n
 * \n
 * Consult the <a href="https://en.cppreference.com/w/cpp/utility/tuple/make_tuple">`std::make_tuple`</a> docs
 * for more info.
 */
template <class T>
using make_tuple_element_t = typename std::tuple_element<0, decltype(std::make_tuple(std::declval<T&&>()))>::
    type;

/**
 * \brief A query format string and format arguments that can be executed.
 * \details
 * Contains a query with placeholders (i.e. `{}`) and a set of parameters to
 * expand such placeholders. Satisfies `ExecutionRequest` and can thus be passed
 * to \ref any_connection::execute, \ref any_connection::start_execution and its
 * async counterparts.
 * \n
 * When executed, client-side SQL formatting is invoked
 * to expand the provided query with the supplied parameters. The resulting query is then sent to
 * the server for execution. Formally, given a `conn` variable of \ref any_connection type,
 * the query is generated as if the following was called:
 * ```
 *   format_sql(
 *       this->query,
 *       conn.format_opts().value(),
 *       std::get<i>(this->args)... // for i in [0, sizeof...(Formattable))
 *   );
 * ```
 * \n
 * Objects of this type are usually created using \ref with_params, which
 * creates `args` by calling `std::make_tuple`.
 *
 * \par Object lifetimes
 * The format string `query` is stored as a view, as a compile-time string should be used in most cases.
 * When using \ref with_params, `args` will usually contain copies of the passed parameters
 * (as per <a href="https://en.cppreference.com/w/cpp/utility/tuple/make_tuple">`std::make_tuple`</a>),
 * which is safe even when using async functions with deferred completion tokens.
 * You may disable such copies using `std::ref`, as you would when using `std::make_tuple`.
 *
 * \par Errors
 * When passed to \ref any_connection::execute, \ref any_connection::start_execution or
 * its async counterparts, in addition to the usual network and server-generated errors,
 * `with_params_t` may generate the following errors: \n
 *   - Any errors generated by \ref format_sql. This includes errors due to invalid format
 *     strings and unformattable arguments (e.g. invalid UTF-8).
 *   - \ref client_errc::unknown_character_set if the connection does not know the
 *     character set it's using when the query is executed (i.e. \ref any_connection::current_character_set
 *     would return an error.
 */
template <BOOST_MYSQL_FORMATTABLE... Formattable>
struct with_params_t
{
    /// The query to be expanded and executed, which may contain `{}` placeholders.
    constant_string_view query;

    /// The arguments to use to expand the query.
    std::tuple<Formattable...> args;
};

/**
 * \brief Creates a query with parameters (client-side SQL formatting) that can be executed.
 * \details
 * Creates a \ref with_params_t object by packing the supplied arguments into a tuple,
 * calling <a href="https://en.cppreference.com/w/cpp/utility/tuple/make_tuple">`std::make_tuple`</a>.
 * As per `std::make_tuple`, parameters will be decay-copied into the resulting object.
 * This behavior can be disabled by passing `std::reference_wrapper` objects, which are
 * transformed into references.
 * \n
 * This function does not inspect the supplied query string and arguments.
 * Errors like missing format arguments are detected when the resulting object is executed.
 * This function does not involve communication with the server.
 * \n
 * The passed `args` must either satisfy `Formattable`, or be `std::reference_wrapper<T>`
 * with `T` satisfying `Formattable`.
 * \n
 * See \ref with_params_t for details on how the execution request works.
 * \n
 * \par Exception safety
 * Strong guarantee. Any exception thrown when copying `args` will be propagated.
 * \n
 */
template <class... FormattableOrRefWrapper>
auto with_params(constant_string_view query, FormattableOrRefWrapper&&... args)
    -> with_params_t<make_tuple_element_t<FormattableOrRefWrapper>...>
{
    return {query, std::make_tuple(std::forward<FormattableOrRefWrapper>(args)...)};
}

}  // namespace mysql
}  // namespace boost

#include <boost/mysql/impl/with_params.hpp>

#endif
