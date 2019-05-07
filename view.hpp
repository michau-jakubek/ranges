#ifndef __VIEW_HPP__
#define __VIEW_HPP__

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <viable.hpp>

/**
 * 2019-03-05
 * constexpt where + operator()(lambda)
 * constexpr selecting + operator()(selector<T>, CtorParams...)
 * helper selector<T>
 * helper splaceholder
 */

namespace view
{
	struct where_t
	{
		template<class I, class F>
		auto operator()(I&& begin, I&& end, F&& unary_op)
		{
			typedef typename
				std::iterator_traits<std::remove_reference_t<I>>::value_type value_type;

			std::vector<value_type> result;

			std::copy_if(std::forward<I>(begin), std::forward<I>(end),
				std::back_inserter(result), unary_op);

			return result; 
		}
	};

	template<class T>
	struct splaceholder_t
	{
		T& value;
		splaceholder_t(T&& x) : value(x) {}
		T& get() const { return value; }
	};

	struct splaceholder_default {};

	struct splaceholder_factory
	{
		auto operator()() const
			{ return splaceholder_t<splaceholder_default>(splaceholder_default()); }
		template<class T>
		splaceholder_t<T> operator()(T&& x) const
			{ return splaceholder_t<T>(std::forward<T>(x)); }
	};

	template<class T> struct selector { };

	template<class X>
	struct fwd
	{
		X& x;

		template<class Y, class=std::enable_if_t<std::is_convertible<Y,X>::value>>
		fwd(Y&& y) noexcept : x(y) { }

		template<class Y>
		auto get(Y&&) const && -> X&& { return std::move(x); }
	};

	template<>
	struct fwd<splaceholder_t<splaceholder_default>>
	{
		template<class Y>
		fwd(Y&&) noexcept { }

		template<class Y>
		auto get(Y& y) const -> Y& { return y; }
	};

	template<class T>
	struct fwd<splaceholder_t<T>>
	{
		typedef viable::lambda_info<T> LI;

		splaceholder_t<T>& phdr;

		fwd(splaceholder_t<T>&& p) : phdr(p) {}

		template<class Y>
		auto get(Y&& y) const
		{
			static_assert(LI::template from_args<Y>::value, "Unable call lamda with that argument");
			return phdr.get()(y);
		}
	};

	struct select_t
	{
		template<class I, class S, class... Ts>
		auto operator()(I&& begin, I&& end, selector<S>&&, Ts&&... xs)
		{
			typedef typename
				std::iterator_traits<std::remove_reference_t<I>>::value_type value_type;

			static_assert(viable::is_viable<S>::template from_args<
				decltype(fwd<std::remove_reference_t<Ts>>(std::forward<Ts>(xs)).get(*begin))... >::value, "No matching constructor to call");

			std::vector<S> result;

			result.reserve(std::distance(begin, end));

			for (I i = begin; i != end; ++i)
			{
				result.emplace_back(fwd<std::remove_reference_t<Ts>>(std::forward<Ts>(xs)).get(*i)...);
			}

			return result;
		}
	};

	template<class T> struct functor_wrapper;
	template<class T> auto make_functor_wrapper(T&& f)
	{
		return functor_wrapper<T>(std::forward<T>(f));
	}

	template<class T>
	struct functor_wrapper
	{
		T functor;
		T get() const { return std::move(functor); }
		constexpr functor_wrapper() = default;
		functor_wrapper(T&& x) : functor(x) {}
		template<class F>
		auto operator()(F&& f) const
		{
			return make_functor_wrapper(
			[=](auto&& begin, auto&& end)
			{
				return T()(begin, end, f);
			});
		}
		template<class S, class... Ts>
		auto operator()(selector<S>&&, Ts&&... xs) const
		{
			return make_functor_wrapper(
			[&](auto&& begin, auto&& end)
			{
				return T()(begin, end, selector<S>(), std::move(xs)...);
			});
		}
	};

	template<class T, class F>
	auto operator|(T&& x, const view::functor_wrapper<F>& f)
	{
		return f.get()(x.begin(), x.end());
	}

	constexpr functor_wrapper<view::where_t> where{};
	constexpr functor_wrapper<view::select_t> selecting{};
	constexpr splaceholder_factory	splaceholder{};

} // namespace view

#endif // __VIEW_HPP__

