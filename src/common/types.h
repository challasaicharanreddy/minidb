#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum class ColumnType {
    INT,
    VARCHAR
};

struct Column {
    std::string name;
    ColumnType type;
    uint16_t length;
};

using Value = std::variant<int32_t, std::string>;

using Row = std::vector<Value>;

struct Schema {
    std::vector<Column> columns;

    uint16_t RecordSize() const {
        uint16_t size = 0;

        for (const Column& c : columns) {
            size += (c.type == ColumnType::INT)
                        ? 4
                        : c.length;
        }

        return size;
    }
};