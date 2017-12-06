#pragma once

namespace bimaps {
    namespace detail {
        template <int... N>
        struct sequence
        {
        };

        template <int S, int... N>
        struct unpacker
        {
            typedef typename unpacker<S - 1, S - 1, N...>::type type;
        };

        template <int... N>
        struct unpacker<0, N...>
        {
            typedef sequence<N...> type;
        };

        template <typename Map, typename... Args, int... S>
        static void call_ctor(Map &map, const std::tuple<Args...> &args, detail::sequence<S...> &)
        {
            map = Map((std::get<S>(args))...);
        };

        template<typename T>
        class side_identity
        {
        };
    }
}