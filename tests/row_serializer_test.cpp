#include <gtest/gtest.h>
#include <stdexcept>
#include "storage/row_serializer.h"

static Schema MakeSchema() {
    return Schema{{{"id", ColumnType::INT, 4}, {"name", ColumnType::VARCHAR, 10}}};
}

TEST(RowSerializerTest, RoundTrip) {
    Schema s = MakeSchema();
    Row row = {int32_t(-42), std::string("alice")};
    std::string bytes = RowSerializer::Serialize(s, row);
    EXPECT_EQ(bytes.size(), s.RecordSize());
    Row back = RowSerializer::Deserialize(s, bytes.data());
    EXPECT_EQ(std::get<int32_t>(back[0]), -42);
    EXPECT_EQ(std::get<std::string>(back[1]), "alice");
}

TEST(RowSerializerTest, EmptyAndMaxLengthStrings) {
    Schema s = MakeSchema();
    for (std::string name : {std::string(""), std::string("abcdefghij")}) {
        Row row = {int32_t(7), name};
        Row back = RowSerializer::Deserialize(s, RowSerializer::Serialize(s, row).data());
        EXPECT_EQ(std::get<std::string>(back[1]), name);
    }
}

TEST(RowSerializerTest, TooLongStringThrows) {
    Schema s = MakeSchema();
    Row row = {int32_t(1), std::string("abcdefghijk")};   // 11 chars, limit is 10
    EXPECT_THROW(RowSerializer::Serialize(s, row), std::runtime_error);
}