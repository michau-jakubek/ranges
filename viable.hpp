#ifndef __VIABLE_HPP__
#define __VIABLE_HPP__

#include <type_traits>
#include <utility>

/**
 * @author		Michau Jakubek
 * @date		20190407
 *
 * @brief		Introduces helpers family to compile-time checking if given type is viable.
 *				The type might be a type then you can check if it can be constructed from given arg types.
 *				Forementioned type can be a function pointer or method pointer then you can check if that
 *				pointer can be successfuly called with arg types.
 *				Finally this type can be a closure (lambda with its environment) and in that case you can
 *				check if its parenthesis operator can be call with given types as well.
 *
 * @usage:		template<class Type, class... Args>
 *				viable::is_viable<CLASS>::from_args<Args...>::value
 *
 *				template<class Lambda> struct lambda_info;
 *
 *				template<class Lambda> lambda_info<Lambda> make_lambda_info(Lambda&&);
 *
 * @example:	struct Foo {
 *					Foo(int,int);
 *					int bar(int);
 *				};
 *				static_assert( is_viable<Foo>::from_args<int,int>::value, "Foo ctor params mismatch" );
 *
 *				typedef int(Foo:*FooBarPtr)(int);
 *				static_assert( is_viable<FooBarPtr>::from_args<int>::value, "Unable to call int(Foo::*)(int)" );
 *
 *				auto lambda = [](Foo& foo, int x) { return foo.bar(x); };
 *				typedef lambda_info<decltype(lambda)> closure;
 *				static_assert( std::is_same< closure::result_type, int >::value, "" );
 *				static_assert( is_viable< closure::pointer_type>::from_args<Foo&,int>::value, "" );
 *				static_assert( closure::from_args<Foo&,int>::value, "" );
 *
 *				Foo foo(1,2);
 *				auto info = make_lambda_info(lamda);
 *				foo.invoke(foo,3);
 */

namespace viable
{
	template<class T, class... Args>
	struct is_viable_from
	{
		static std::false_type	test(...);
		template<class X>
		static std::true_type	test(X*, typename std::add_pointer<
			decltype(X(std::declval<Args>()...)) >::type = nullptr);
		using type = decltype( test(static_cast<T*>(nullptr)) );
	};

	template<class R, class... Pars, class... Args>
	struct is_viable_from<R (*)(Pars...), Args...>
	{
		static std::false_type	test(...);
		template<class X>
		static std::true_type	test(X, typename std::add_pointer<
			decltype((*(std::declval<X>()))(std::declval<Args>()...)) >::type = nullptr);
		using type = decltype( test(static_cast<R (*)(Pars...)>(nullptr)) );
	};

	template<class C, class R, class... Pars, class... Args>
	struct is_viable_from<R (C::*)(Pars...), Args...>
	{
		static std::false_type	test(...);
		template<class X>
		static std::true_type	test(X, typename std::add_pointer<
			decltype( (std::declval<C>().*(std::declval<X>()) )(std::declval<Args>()...))>::type = nullptr);
		using type = decltype( test(static_cast<R (C::*)(Pars...)>(nullptr)) );
	};

	template<class R, class... Pars, class... Args>
	struct is_viable_from<R (&)(Pars...), Args...> : is_viable_from<R (*)(Pars...), Args...> {};

	template<class C, class R, class... Pars, class... Args>
	struct is_viable_from<R (C::*)(Pars...) const, Args...> : is_viable_from<R (C::*)(Pars...), Args...> {};

	template<class T>
	struct is_viable
	{
		template<class... Args>
		struct from_args : is_viable_from<T, Args...>::type { };
	
		template<class Tp> struct from_tuple_args;
		template<template<class...> class Tuple, class... Args>
		struct from_tuple_args<Tuple<Args...>> : is_viable_from<T, Args...>::type {};
	};

	template<class> struct member_info;
	template<class R, class C, class... Args>
	struct member_info<R (C::*)(Args...) const>
	{
		typedef R result_type;
		typedef R (C::*pointer_type)(Args...) const;
		static constexpr std::size_t arg_count = sizeof...(Args);
	};

	template<class F>
	struct lambda_info
	{
		typedef typename std::remove_reference<F>::type				lambda_type;
		typedef member_info<decltype(&lambda_type::operator())>		pointer_info;
		typedef typename pointer_info::result_type					result_type;
		typedef typename pointer_info::pointer_type					pointer_type;
		static constexpr std::size_t								arg_count = pointer_info::arg_count;
	
		template<class... Args> struct from_args : is_viable<pointer_type>::template from_args<Args...> {};
	
		static pointer_type get_pointer() { return &lambda_type::operator(); }
	
		template<class... Args, class=typename std::enable_if<from_args<Args...>::value>::type>
		result_type invoke(F&& lambda, Args&&... args) const
		{
			return lambda.operator()(std::forward<Args>(args)...);
		}
	};

	template<class F>
	constexpr auto make_lambda_info(F&& f) -> lambda_info<F>
	{
		return lambda_info<F>();
	}

} // namespace viable

#endif // __VIABLE_HPP__


