#pragma once

#include <map>
#include <memory>
#include "util.hpp"

namespace bimaps
{

    class Left;
    class Right;

    template <typename Key, typename T>
    using map_adapter = std::map<Key, T, std::less<Key>, std::allocator<std::pair<const Key, T>>>;

    template <typename Key, typename T>
    using umap_adapter = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>,
            std::allocator<std::pair<const Key, T>>>;

    template <typename Side>
    class is_left;

    template <>
    class is_left<Left>
    {
    public:
        static constexpr bool value = true;
    };

    template <>
    class is_left<Right>
    {
    public:
        static constexpr bool value = false;
    };

    template <typename Side>
    using reverse_t = std::conditional_t<is_left<Side>::value, Right, Left>;

    template<typename A, typename B,
            template <typename, typename> class C1 = map_adapter,
            template <typename, typename> class C2 = C1>
    class bimap
    {
        using left_container_t       = C1<A, const B *>;
        using right_container_t      = C2<B, const A *>;
    public:
        template<typename Side>
        class iterator;

        using left_key_t             = A;
        using right_key_t            = B;
        using left_value_t           = std::pair<A, B>;
        using right_value_t          = std::pair<B, A>;
        using const_left_iterator_t  = iterator<Left>;
        using const_right_iterator_t = iterator<Right>;

    private:
        left_container_t left_map;
        right_container_t right_map;

        template<typename Side>
        using map_type_by_side_t = std::conditional_t<is_left<Side>::value,
                left_container_t, right_container_t>;

        template<typename Side>
        using iter_by_side_t = std::conditional_t<is_left<Side>::value,
                const_left_iterator_t, const_right_iterator_t>;

        template <typename Side>
        using curr_key_t = std::conditional_t<is_left<Side>::value, A, B>;

        template <typename Side>
        using curr_value_t = curr_key_t<reverse_t<Side>>;

        const map_type_by_side_t<Left> &
        map_by_side(const detail::side_identity<Left> &) const
        {
            return left_map;
        }

        const map_type_by_side_t<Right> &
        map_by_side(const detail::side_identity<Right> &) const
        {
            return right_map;
        }

        template<typename Side>
        const map_type_by_side_t<Side> &
        map_by_side() const
        {
            return map_by_side(detail::side_identity<Side>());
        }

        template<typename Side>
        map_type_by_side_t<Side> &
        map_by_side()
        {
            auto &x = (static_cast<const bimap *>(this))->map_by_side<Side>();
            return const_cast<map_type_by_side_t<Side> &>(x);
        }

        size_t erase_impl(const curr_key_t<Left> &a, const curr_value_t<Left> &b, const detail::side_identity<Left> &)
        {
            left_map.erase(a);
            return right_map.erase(b);
        }

        size_t erase_impl(const curr_key_t<Right> &a, const curr_value_t<Right> &b, const detail::side_identity<Right> &)
        {
            return erase_impl<Left>(b, a);
        }

        template <typename Side>
        size_t erase_impl(const curr_key_t<Side> &a, const curr_value_t<Side> &b)
        {
            return erase_impl(a, b, detail::side_identity<Side>());
        }


    public:

        template<typename Side>
        class iterator
        {
            static constexpr bool is_left_v = is_left<Side>::value;
            using old_iterator_t = std::conditional_t<is_left_v,
                    typename left_container_t::const_iterator,
                    typename right_container_t::const_iterator>;
            using value_t = std::conditional_t<is_left_v, std::pair<const A &, const B &>,
                    std::pair<const B &, const A &>>;
            old_iterator_t iterator_impl;

        public:

            iterator() = default;

            iterator &operator=(const iterator &) = default;

            ~iterator() = default;

            iterator(const old_iterator_t &iter) : iterator_impl(iter)
            {
            }

            const old_iterator_t &get_old_iterator() const
            {
                return iterator_impl;
            }

            value_t operator*() const
            {
                auto &ret = *iterator_impl;
                return {ret.first, *ret.second};
            }

            std::unique_ptr<const value_t> operator->() const
            {
                auto &ret = *iterator_impl;
                return std::make_unique<const value_t>(ret.first, *ret.second);
            }

            iterator &operator++()
            {
                iterator_impl++;
                return *this;
            }

            iterator operator++(int)
            {
                iterator old = *this;
                ++*this;
                return old;
            }

            iterator &operator--()
            {
                iterator_impl--;
                return *this;
            }

            iterator operator--(int)
            {
                iterator old = *this;
                --*this;
                return old;
            }

            friend bool operator==(iterator a, iterator b)
            {
                return a.iterator_impl == b.iterator_impl;
            }

            friend bool operator!=(iterator a, iterator b)
            {
                return !(a == b);
            }
        };

        bimap() = default;

        template<typename... ArgsC1, typename... ArgsC2>
        bimap(std::tuple<ArgsC1...> args1, std::tuple<ArgsC2...> args2)
        {
            using t1 = std::tuple<ArgsC1...>;
            using t2 = std::tuple<ArgsC2...>;
            auto s1 = typename detail::unpacker<std::tuple_size<t1>::value>::type();
            auto s2 = typename detail::unpacker<std::tuple_size<t2>::value>::type();
            detail::call_ctor(left_map, args1, s1);
            detail::call_ctor(right_map, args2, s2);
            if (left_map.size() || right_map.size())
            {
                throw std::logic_error("Attempt to create bimap with non-empty maps");
            }
        };

        bimap(const bimap &map) = delete;

        bimap(bimap &&other) = delete;

        bimap &operator=(const bimap &) = default;

        bimap &operator=(bimap &&) = default;

        bimap(std::initializer_list<left_value_t> init)
        {
            for (auto &x : init)
            {
                insert(x.first, x.second);
            }
        }

        ~bimap() = default;

        template<typename LeftType, typename RightType>
        std::pair<const_left_iterator_t, const_right_iterator_t>
        insert(LeftType &&left, RightType &&right)
        {
            using R = std::pair<const_left_iterator_t, const_right_iterator_t>;
            auto l_find = left_map.find(left);
            auto r_find = right_map.find(right);
            if (l_find != left_map.end() || r_find != right_map.end())
                return R(l_find, r_find);
            auto l_iter = left_map.insert({std::forward<LeftType>(left), nullptr}).first;
            try
            {
                auto r_iter = right_map.insert({std::forward<RightType>(right), &l_iter->first}).first;
                try
                {
                    left_map[l_iter->first] = &r_iter->first;
                    return R(l_iter, r_iter);
                }
                catch (...)
                {
                    right_map.erase(r_iter->first);
                    throw;
                }
            }
            catch (...)
            {
                left_map.erase(l_iter->first);
                throw;
            }
        }

        template<typename Side>
        size_t erase(const curr_key_t<Side> &key)
        {
            auto &map = map_by_side<Side>();
            if (map.find(key) == map.end())
                return 0;
            auto *second_key = map[key];
            return erase_impl<Side>(key, *second_key);
        }

        template<typename Side>
        iterator<Side> erase(iterator<Side> iter)
        {
            iterator<Side> old_iter = iter++;
            auto &side_map = map_by_side<Side>();
            auto &reverse_map = map_by_side<reverse_t<Side>>();
            reverse_map.erase(old_iter->second); // O(log(n))
            side_map.erase(old_iter.get_old_iterator()); // O(1)
            return iter;
        }

        template<typename Side>
        const curr_value_t<Side> &
        at(const curr_key_t<Side> &key) const
        {
            auto &map = map_by_side<Side>();
            return *map.at(key);
        }

        template<typename Side, typename Key>
        iter_by_side_t<Side>
        find(const Key &key) const
        {
            auto &map = map_by_side<Side>();
            return (static_cast<const map_type_by_side_t<Side> &>(map)).find(key);
        }

        template<typename Side>
        iter_by_side_t<Side> begin() const
        {
            auto &map = map_by_side<Side>();
            return map.cbegin();
        }

        template<typename Side>
        iter_by_side_t<Side> end() const
        {
            auto &map = map_by_side<Side>();
            return map.cend();
        }

        bool empty() const noexcept
        {
            return left_map.empty();
        }

        size_t size() const noexcept
        {
            return left_map.size();
        }

        void clear() noexcept
        {
            left_map.clear();
            right_map.clear();
        }

    };
}
