#ifndef __VIEW_HPP__
#define __VIEW_HPP__

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <viable.hpp>

namespace views
{
	// Main view definition.
	template<class T> using view = std::vector<std::reference_wrapper<T>>;

	// Extracts a value_type from given STL-like collection iterator.
	template<class I> using value_type_t = typename std::iterator_traits<std::remove_reference_t<I>>::value_type;

	// Generic template reference_unwrapper class.
	template<class T>
	struct reference_unwrapper
	{
		typedef T type;
		static T& get(T& x) { return x; }
	};

	// Specialised template reference_unwrapper class that unwraps
	// a reference previously wrapped with std::reference_wrapper.
	template<class T>
	struct reference_unwrapper<std::reference_wrapper<T>>
	{
		typedef T type;
		static T& get(std::reference_wrapper<T>& x) { return x.get(); }
	};

	// Makes a view from the iterator range.
	template<class I, class value_type = value_type_t<I>>
	auto make_view(I first, I last) -> view<typename reference_unwrapper<value_type>::type>
	{
		view<typename reference_unwrapper<value_type>::type> res;
		std::for_each(first, last, [&res](value_type& x)
				{
					res.emplace_back(reference_unwrapper<value_type>::get(x));
				});
		return res;
	}

	// Makes a view from the iterator range but only for which the predicate pred returns true.
	template<class I, class Pred, class value_type = value_type_t<I>>
	auto make_view(I first, I last, Pred pred) -> view<typename reference_unwrapper<value_type>::type>
	{
		view<typename reference_unwrapper<value_type>::type> res;
		std::for_each(first, last, [&res, &pred](value_type& x)
				{
					if (pred(reference_unwrapper<value_type>::get(x)))
						res.emplace_back(reference_unwrapper<value_type>::get(x));
				});
		return res;
	}

	// Where clause class definition.
	struct where_t
	{
		template<class I, class F>
		auto operator()(I&& begin, I&& end, F&& unary_op)
		{
			return make_view(begin, end, unary_op);
		}
	};

	// Generic splaceholder class definition.
	// When used as an argument to selecting expression each placeholder is replaced by
	// currently processed collection element. Additionally placeholder can be parameterized
	// with callable then underlying callable operator will be called before placeholder is
	// passed to selecting expression.
	template<class T>
	struct splaceholder_t
	{
		T& value;
		splaceholder_t(T&& x) : value(x) {}
		T& get() const { return value; }
	};

	struct splaceholder_default {};

	// Placeholders factory.
	struct splaceholder_factory
	{
		auto operator()() const
			{ return splaceholder_t<splaceholder_default>(splaceholder_default()); }

		template<class T>
		splaceholder_t<T> operator()(T&& x) const
			{ return splaceholder_t<T>(std::forward<T>(x)); }
	};

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
			static_assert(LI::template from_args<Y>::value,
					"Unable call lamda with that argument");
			return phdr.get()(y);
		}
	};

	// Disambiguation tag for a type used in selecting expression.
	template<class T> struct selector { };

	// Selecting expression class definition.
	struct select_t
	{
		template<class I, class S, class... Ts>
		auto operator()(I&& begin, I&& end, selector<S>&&, Ts&&... xs)
		{
			typedef typename
				std::iterator_traits<std::remove_reference_t<I>>::value_type value_type;

			static_assert(viable::is_viable<S>::template from_args<
				decltype(fwd<std::remove_reference_t<Ts>>(std::forward<Ts>(xs)).get(*begin))... >::value,
					"No matching constructor to call");

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

		functor_wrapper(T&& x) : functor(std::move(x)) {}

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
	auto operator|(T&& x, const functor_wrapper<F>& f)
	{
		return f.get()(x.begin(), x.end());
	}

	constexpr functor_wrapper<where_t>	where{};
	constexpr functor_wrapper<select_t>	selecting{};
	constexpr splaceholder_factory		splaceholder{};

} // namespace view;

#endif // __VIEW_HPP__

