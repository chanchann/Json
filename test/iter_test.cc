#include "Json.h"
#include <gtest/gtest.h>

using namespace wfrest;

TEST(IterTest, object)
{
    Json data;
    data["key1"] = 1;
    data["key2"] = 2.0;
    data["key3"] = true;
    Json::iterator it = data.begin();
    EXPECT_EQ(it.key(), "key1");
    EXPECT_EQ(it.value().get<int>(), 1);
    EXPECT_EQ((*it).get<int>(), 1);
    it++;
    EXPECT_EQ(it.key(), "key2");
    EXPECT_EQ(it.value().get<double>(), 2.0);
    EXPECT_EQ((*it).get<double>(), 2.0);
    ++it;
    EXPECT_EQ(it.key(), "key3");
    EXPECT_EQ(it.value().get<bool>(), true);
    EXPECT_EQ((*it).get<bool>(), true);
    ++it;
    EXPECT_EQ(it, data.end());
    ++it;
    ++it; // safe to ++ forever, always stay at end() position
    EXPECT_EQ(it, data.end());
    // for (Json::iterator it = data.begin(); it != data.end(); it++)
    // {
    //     std::cout << it.key() << it.value() << std::endl;
    // }
}

TEST(IterTest, Array)
{
    Json data;
    data.push_back(1);
    data.push_back(2.0);
    data.push_back(false);
    Json::iterator it = data.begin();
    EXPECT_EQ(it.value().get<int>(), 1);
    EXPECT_EQ((*it).get<int>(), 1);
    ++it;
    EXPECT_EQ(it.value().get<double>(), 2.0);
    EXPECT_EQ((*it).get<double>(), 2.0);
    it++;
    EXPECT_EQ(it.value().get<bool>(), false);
    EXPECT_EQ((*it).get<bool>(), false);
    ++it;
    it++;
    EXPECT_EQ(it, data.end());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}