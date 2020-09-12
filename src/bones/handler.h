//
// Created by Colin MacKenzie on 2019-01-16.
//

#ifndef RESTFULLY_FUNCTION_TRAITS_H
#define RESTFULLY_FUNCTION_TRAITS_H

#include <functional>
#include <memory>
#include <tuple>


namespace Rest {

    template<class... TArgs>
    class Handler {
    public:
        typedef std::function< int(TArgs... args) > F0;
        typedef std::tuple< TArgs... > arguments;

#if 0
        Handler() {}
        //Handler(const F0& _f) : handler(_f) {}
        Handler(F0&& _f) : handler(std::move(_f)) {}
        //Handler(std::function<TArgs...> _f) : handler(_f) {}
        Handler(int (_handler)(TArgs... args)) { handler = _handler; }
#endif
        template<class ... MArgs>
        Handler(MArgs... args) : handler(args...) {}

        int operator()(TArgs... args) {
            return handler(args...);
        }

        inline bool operator==(std::nullptr_t v) const { return handler==v; }
        inline bool operator!=(std::nullptr_t v) const { return handler!=v; }
        inline operator bool() const { return handler!=nullptr; }

        F0 handler;
    };

#if 0
    /*** Function Traits
     *  adapted from boost::function_traits
     */
    template<class F>
    struct function_traits;

    // function pointer
    template<class R, class... Args>
    struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {
    };

    // Remove the first item in a tuple
    template<typename T>
    struct tuple_tail;

    template<typename Head, typename ... Tail>
    struct tuple_tail<std::tuple<Head, Tail ...>>
    {
        using type = std::tuple<Tail ...>;
    };

    // Free functions
    template<class R, class... Args>
    struct function_traits<R(Args...)> {
        using return_type = R;

        using arguments = std::tuple<Args ...>;

        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        struct argument {
            static_assert(N < arity, "error: invalid parameter index.");
            using type = typename std::tuple_element<N, std::tuple < Args...>>::type;
        };

        typedef Rest::Handler<Args...> HandlerType;
        typedef std::function<return_type(Args...)> FunctionType;
    };


    // std::function
    template<typename FunctionT>
    struct function_traits
    {
        using arguments = typename tuple_tail<
                typename function_traits<decltype( & FunctionT::operator())>::arguments>::type;

        static constexpr std::size_t arity = std::tuple_size<arguments>::value;

        template<std::size_t N>
        using argument_type = typename std::tuple_element<N, arguments>::type;

        using return_type = typename function_traits<decltype( & FunctionT::operator())>::return_type;

        using HandlerType = typename function_traits<decltype( & FunctionT::operator())>::HandlerType ;
        using FunctionType = typename function_traits<decltype( & FunctionT::operator())>::FunctionType ;
    };

    // member function pointer
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...)> : public function_traits<R(C *, Args...)> {
    };

    // const member function pointer
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C *, Args...)> {
    };

    // member object pointer
    //template<class C, class R>
    //struct function_traits<R(C::*)> : public function_traits<R(C&)>  {};

    template<typename FunctionT>
    struct function_traits<FunctionT&>: public function_traits<FunctionT> {};

    template<typename FunctionT>
    struct function_traits<FunctionT&&>: public function_traits<FunctionT> {};

#if 0
    // std::bind for free functions
    template<typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__1::__bind<ReturnTypeT( &)(Args ...), FArgs ...>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
    struct function_traits<std::_Bind<ReturnTypeT(*(FArgs ...))(Args ...)>>
#elif defined _MSC_VER  // MS Visual Studio
        struct function_traits<std::_Binder<std::_Unforced, ReturnTypeT(__cdecl &)(Args ...), FArgs ...>>
 #else
 #error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};
#endif
#else

    template <typename, typename, typename T>
    struct has_method {
        static_assert(std::integral_constant<T, false>::value,
                      "Third template parameter needs to be of function type.");
    };

    template <typename C, class caller, typename Ret, typename... Args>
    struct has_method<C, caller, Ret(Args...)> {
    private:
        template <typename T>
        static constexpr auto check(T *) ->
        typename std::is_same<decltype(std::declval<caller>().template call<T>(
                std::declval<Args>()...)),
                Ret>::type
        {
            return typename std::is_same<
                    decltype(std::declval<caller>().template call<T>(
                            std::declval<Args>()...)),
                    Ret>::type();
            //return to surpresswarnings
        }

        template <typename>
        static constexpr std::false_type check(...)
        {
            return std::false_type();
        }

        typedef decltype(check<C>(0)) type;

    public:
        static constexpr bool value = type::value;
    };

    struct attach_caller {
        template <class T, typename... Args>
        constexpr auto call(Args... args) const
        -> decltype(std::declval<T>().attach(args...)) {
            return decltype(std::declval<T>().attach(args...))();  //return to surpresswarnings
        }
    };

    // Remove the first item in a tuple
    template<typename T>
    struct tuple_tail;

    template<typename Head, typename ... Tail>
    struct tuple_tail<std::tuple<Head, Tail ...>>
    {
        using type = std::tuple<Tail ...>;
    };

    // std::function
    template<typename FunctionT>
    struct function_traits: function_traits<decltype( & FunctionT::operator() )>{
    };

    template<typename FunctionT>
    struct function_traits< Handler<FunctionT> >: function_traits<decltype( & Handler<FunctionT>::F0::operator() )> {
    };

    // Free functions
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT(Args ...)>
    {
        using arguments = std::tuple<Args ...>;

        static constexpr std::size_t arity = std::tuple_size<arguments>::value;

        template<std::size_t N>
        using argument_type = typename std::tuple_element<N, arguments>::type;

        using return_type = ReturnTypeT;

        typedef Rest::Handler<Args...> HandlerType;
        typedef std::function<return_type(Args...)> FunctionType;

        template<class X> using CVFunctionType = return_type(X::*)(Args...);
        template<class X> using CVConstFunctionType = return_type(X::*)(Args...) const;
        template<class RT> using MakeFunction = RT(Args...);
        template<class RT, class Klass> using MakeMemberFunction = RT(Klass::*)(Args...);

    };

    // Function pointers
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT (*)(Args ...)>: function_traits<ReturnTypeT(Args ...)>
    {};

    // std::bind for object methods
    template<typename ClassT, typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__bind<ReturnTypeT (ClassT::*)(Args ...), FArgs ...>>
#elif defined _GLIBCXX_RELEASE  // glibc++ (GNU C++ >= 7.1)
        struct function_traits<std::_Bind<ReturnTypeT(ClassT::*(FArgs ...))(Args ...)>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
struct function_traits<std::_Bind<std::_Mem_fn<ReturnTypeT (ClassT::*)(Args ...)>(FArgs ...)>>
#elif defined _MSC_VER  // MS Visual Studio
struct function_traits<
  std::_Binder<std::_Unforced, ReturnTypeT(__cdecl ClassT::*)(Args ...), FArgs ...>
>
#else
#error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};

    // std::bind for free functions
    template<typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__1::__bind<ReturnTypeT( &)(Args ...), FArgs ...>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
        struct function_traits<std::_Bind<ReturnTypeT(*(FArgs ...))(Args ...)>>
#elif defined _MSC_VER  // MS Visual Studio
struct function_traits<std::_Binder<std::_Unforced, ReturnTypeT(__cdecl &)(Args ...), FArgs ...>>
#else
#error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};

    // Lambdas
    // we must _hide_ the Class type when we recursively pass to function_traits<>
    // warning: this will also catch 'const member function pointers', but will not forward the class type
    template<typename C, typename R, typename ... Args>
    struct function_traits<R (C::*)(Args ...) const>
            : public function_traits<R(Args ...)>
    {};

    // member function pointer
    // we must _include_ the Class type when we recursively pass to function_traits<>
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...)> : public function_traits<R(C *, Args...)> {
    };

    // const member function pointer
    //template<class C, class R, class... Args>
    //struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C *, Args...)> {
    //};

    template<typename FunctionT>
    struct function_traits<FunctionT &>: public function_traits<FunctionT>
    {};

    template<typename FunctionT>
    struct function_traits<FunctionT &&>: public function_traits<FunctionT>
    {};

    /* NOTE(esteve):
     * VS2015 does not support expression SFINAE, so we're using this template to evaluate
     * the arity of a function.
     */
    template<std::size_t Arity, typename FunctorT>
    struct arity_comparator : std::integral_constant<
            bool, (Arity == function_traits<FunctorT>::arity)>{};

    template<typename FunctorT, typename ... Args>
    struct check_arguments : std::is_same<
            typename function_traits<FunctorT>::arguments,
            std::tuple<Args ...>
    >
    {};

    template<typename FunctorAT, typename FunctorBT>
    struct same_arguments : std::is_same<
            typename function_traits<FunctorAT>::arguments,
            typename function_traits<FunctorBT>::arguments
    >
    {};

#endif


    /*
     * StaticHandler<>
     * Converts a given handler, specifically CV-type (class) method handlers into thier static non-class function types
     */
    template<typename R, typename ... Args>
    struct StaticHandler {
        using Type = R(Args ...);
        using Function = std::function< Type >;
    };

    template<typename R, typename ... Args>
    struct StaticHandler<R(Args...)> : StaticHandler<R, Args...> {};

    template<typename ... Args>
    struct StaticHandler<Rest::Handler< int(Args...) > > {
        using Type = Rest::Handler<int(Args ...)>;
        using Function = std::function< Type >;
    };

    template<typename R, typename C, typename ... Args>
    struct StaticHandler<R(C::*)(Args...)> : StaticHandler<R, Args...> {
        using Type = int(C::*)(Args...);

        template<class I>
        static std::function< int(Args...) > bind(typename std::function<int(C::*)(Args...)>& handler, I& inst) {
            return std::bind(handler, inst, std::placeholders::_1);
        }

        template<class I>
        static std::function< int(Args...) > bind(Type& handler, I& inst) {
            return std::bind(handler, inst, std::placeholders::_1);
        }
    };

    template<typename C, typename ... Args>
    struct StaticHandler<Rest::Handler< int(C::*)(Args...) > > : StaticHandler< Rest::Handler<int, Args...> > {
        template<class I>
        static Rest::Handler< int(Args...)> bind(typename Rest::Handler< int(C::*)(Args...) >& handler, I& inst) {
            return std::bind(handler.handler, inst, std::placeholders::_1);
        }
    };

    template<typename C, typename ... Args>
    struct StaticHandler<Rest::Handler< int(C::*)(Args...)const > > : StaticHandler< Rest::Handler<int, Args...> > {
        template<class I>
        static Rest::Handler< int(Args...)const > bind(typename Rest::Handler< int(C::*)(Args...)const >& handler, I& inst) {
            return std::bind(handler.handler, inst, std::placeholders::_1);
        }
    };



    /*
       * StaticHandler<>
       * Converts a given handler, specifically CV-type (class) method handlers into thier static non-class function types
       */
    template<class K, class H>
    struct KlassHandler {
        using HandlerTraits = Rest::function_traits<H>;
        using Type = typename HandlerTraits::template CVFunctionType<K>;
        using ConstType = typename HandlerTraits::template CVConstFunctionType<K>;
    };

    template<class K, class H>
    struct KlassHandler< K, Rest::Handler<H> > {
        using HandlerTraits = Rest::function_traits< Rest::Handler<H> >;
        using Type = Rest::Handler< typename HandlerTraits::template CVFunctionType<K> >;
        using ConstType = Rest::Handler< typename HandlerTraits::template CVConstFunctionType<K> >;
    };


} // ns: Rest

#endif //RESTFULLY_FUNCTION_TRAITS_H