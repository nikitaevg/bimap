#include <gtest/gtest.h>
#include <random>
#include <unordered_map>
#include "bimap.hpp"

using namespace bimaps;

namespace
{

    class random
    {
    protected:
        std::random_device r;
        std::default_random_engine eng;

        random() : r(), eng(r())
        {

        }
    };

    template<typename T>
    class random_generator;

    template<>
    class random_generator<double> : random
    {
        std::uniform_real_distribution<double> double_dist;
    public:
        double operator()()
        {
            return double_dist(eng);
        }
    };

    template<>
    class random_generator<std::string> : random
    {
        std::uniform_int_distribution<size_t> int_dist;
        static constexpr size_t string_size = 10;
    public:
        std::string operator()()
        {
            std::string s;
            for (size_t i = 0; i < string_size; i++)
                s.push_back(static_cast<char>(int_dist(eng) % 26 + 97));
            return s;
        }
    };

    template<>
    class random_generator<int32_t> : random
    {
        std::uniform_int_distribution<int32_t> int_dist;
    public:
        int32_t operator()()
        {
            return int_dist(eng);
        }
    };

    random_generator<double> d_gen;
    random_generator<std::string> s_gen;
    random_generator<int32_t> i_gen;

    template<typename Side, typename A, typename B,
            template<typename, typename> typename Map1,
            template<typename, typename> typename Map2,
            typename Map>
    void test_maps_equality(bimap<A, B, Map1, Map2> &bm, Map &map)
    {
        for (auto &iter : map)
        {
            ASSERT_EQ(bm.template at<Side>(iter.first), iter.second);
            ASSERT_FALSE(bm.template find<Side>(iter.first) == bm.template end<Side>());
        }
    }

    template<typename A, typename B,
            template<typename, typename> typename Map1,
            template<typename, typename> typename Map2>
    void test_insert(bimap<A, B, Map1, Map2> &bm, random_generator<A> &generator_a,
                     random_generator<B> &generator_b, size_t iters)
    {
        std::map<A, B> map1;
        std::map<B, A> map2;
        for (size_t i = 0; i < iters; i++)
        {
            A first = generator_a();
            B second = generator_b();
            bool inserted1 = map1.insert({first, second}).second;
            bool inserted2 = map2.insert({second, first}).second;
            if (inserted1 && inserted2)
            {
                // Not to find not existing objects
                ASSERT_EQ(bm.template find<Left>(first), bm.template end<Left>());
                ASSERT_EQ(bm.template find<Right>(second), bm.template end<Right>());
                bm.insert(first, second);
            }
        }

        test_maps_equality<Left>(bm, map1);
        test_maps_equality<Right>(bm, map2);

        for (auto iter = bm.template begin<Left>(); iter != bm.template end<Left>(); iter++)
        {
            ASSERT_EQ(iter->second, map1.at(iter->first));
            ASSERT_EQ(iter->first, map2.at(iter->second));
        }
        ASSERT_EQ(bm.size(), map1.size());

        if (iters)
            ASSERT_FALSE(bm.empty());
    }

    template<typename Side, typename A, typename B,
            template<typename, typename> typename Map1,
            template<typename, typename> typename Map2>
    void erase_elements(bimap<A, B, Map1, Map2> &bm,
                        typename bimap<A, B, Map1, Map2>::template iterator<Side> iter, size_t iters)
    {
        for (size_t i = 0; i < iters; i++)
        {
            auto &&a = iter->first;
            auto &&b = iter->second;
            if (i % 2)
                iter = bm.erase(iter);
            else
            {
                iter++;
                bm.template erase<Side>(a);
            }
            ASSERT_EQ(bm.template find<Side>(a), bm.template end<Side>());
            ASSERT_EQ(bm.template find<reverse_t<Side>>(b), bm.template end<reverse_t<Side>>());
        }
    }

    template<typename A, typename B,
            template<typename, typename> typename Map1,
            template<typename, typename> typename Map2>
    void test_erase(bimap<A, B, Map1, Map2> &bm, random_generator<A> &generator_a,
                    random_generator<B> &generator_b, size_t iters)
    {
        for (size_t i = 0; i < iters; i++)
            bm.insert(generator_a(), generator_b());

        size_t size = bm.size();
        erase_elements<Left>(bm, bm.template begin<Left>(), size / 2);
        size /= 2;
        erase_elements<Right>(bm, bm.template begin<Right>(), size);
        ASSERT_EQ(bm.size(), 0);
    }

}

constexpr size_t ITERATIONS = 10000;

TEST(simple_map, copy_ctor)
{
    const std::string check_string("abacaba");
    bimap<int32_t, std::string> bm;
    bm.insert(1, check_string);
    bimap<int32_t, std::string> bimap2(bm);
    ASSERT_EQ(bimap2.find<Left>(1)->second, check_string);
    bm.erase<Right>(check_string);
    ASSERT_EQ(bm.find<Left>(1), bm.end<Left>());
    ASSERT_EQ(bimap2.find<Left>(1)->second, check_string);
}

TEST(simple_map, insert)
{
    bimap<double, std::string> bm;
    ASSERT_TRUE(bm.empty());
    test_insert(bm, d_gen, s_gen, ITERATIONS);
}

TEST(simple_map, clear)
{
    bimap<std::string, double> bm;
    test_insert(bm, s_gen, d_gen, ITERATIONS);
    bm.clear();
    ASSERT_TRUE(bm.empty());
    ASSERT_EQ(bm.size(), 0);
}

TEST(simple_map, erase)
{
    bimap<int32_t, std::string> bm;
    test_erase(bm, i_gen, s_gen, ITERATIONS);
}

TEST(hash_map, insert_and_erase)
{
    bimap<std::string, std::string, std::unordered_map> bm;
    test_insert(bm, s_gen, s_gen, ITERATIONS);
    bm.clear();
    ASSERT_TRUE(bm.empty());
    test_erase(bm, s_gen, s_gen, ITERATIONS);
}