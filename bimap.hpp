#pragma once

#include <map>
#include <memory>
#include "util.hpp"

namespace bimaps
{

    class Right;
    class Left;

    template <typename Side>
    using reverse_t = std::conditional_t<std::is_same_v<Side, Left>, Right, Left>;

    template<typename A, typename B,
            template<typename, typename> typename C1 = std::map,
            template<typename, typename> typename C2 = C1>
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
        using map_type_by_side_t = std::conditional_t<std::is_same_v<Side, Left>,
                left_container_t, right_container_t>;

        template<typename Side>
        using iter_by_side_t = std::conditional_t<std::is_same_v<Side, Left>,
                const_left_iterator_t, const_right_iterator_t>;

        template <typename Side>
        using curr_key_t = std::conditional_t<std::is_same_v<Side, Left>, A, B>;

        template <typename Side>
        using curr_value_t = curr_key_t<reverse_t<Side>>;

        template<typename Side>
        const map_type_by_side_t<Side> &
        map_by_side() const
        {
            if constexpr (std::is_same_v<Side, Left>)
            {
                return left_map;
            } else
            {
                return right_map;
            }
        }

        template<typename Side>
        map_type_by_side_t<Side> &
        map_by_side()
        {
            auto &x = (static_cast<const bimap *>(this))->map_by_side<Side>();
            return const_cast<map_type_by_side_t<Side> &>(x);
        }

        size_t erase_impl(const A &a, const B &b)
        {
            left_map.erase(a);
            return right_map.erase(b);
        };

    public:

        template<typename Side>
        class iterator
        {
            static constexpr bool is_left = std::is_same_v<Side, Left>;
            using old_iterator_t = std::conditional_t<is_left,
                    typename left_container_t::const_iterator,
                    typename right_container_t::const_iterator>;
            using value_t = std::conditional_t<is_left, std::pair<const A &, const B &>,
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

            const value_t &operator*() const
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

            iterator operator--(int)
            {
                iterator old = *this;
                --*this;
                return old;
            }

            iterator &operator--()
            {
                iterator_impl--;
                return *this;
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
                ~bimap();
                throw std::logic_error();
            }
        };

        bimap(const bimap &map) = default;

        bimap(bimap &&other) = default;

        bimap &operator=(const bimap &) = default;

        bimap &operator=(bimap &&) = default;

        bimap(std::initializer_list<left_value_t> init)
        {
            try
            {
                for (auto &x : init)
                {
                    insert(x.first, x.second);
                }
            }
            catch (...)
            {
                ~bimap();
                throw;
            }
        }

        ~bimap() = default;

        template<typename LeftType, typename RightType>
        std::pair<const_left_iterator_t, const_right_iterator_t>
        insert(LeftType &&left, RightType &&right)
        {
            if (auto l_iter = left_map.find(left); l_iter != left_map.end())
            {
                auto r_iter = right_map.find(right);
                return std::pair{iterator<Left>(l_iter), iterator<Right>(r_iter)};
            }
            auto[l_iter, unused] = left_map.insert({std::forward<LeftType>(left), nullptr});
            try
            {
                auto[r_iter, unused] = right_map.insert({std::forward<RightType>(right), nullptr});
                try
                {
                    left_map[l_iter->first] = &r_iter->first;
                    right_map[r_iter->first] = &l_iter->first;
                    return std::pair{iterator<Left>(l_iter), iterator<Right>(r_iter)};
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
            if constexpr (std::is_same_v<Side, Left>)
            {
                return erase_impl(key, *second_key);
            }
            else
            {
                return erase_impl(*second_key, key);
            }
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