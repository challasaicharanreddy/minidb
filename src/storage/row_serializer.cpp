#include "storage/row_serializer.h"
#include <cstring>
#include <stdexcept>

std::string RowSerializer::Serialize(const Schema& schema, const Row& row) {
    if (row.size() != schema.columns.size()) throw std::runtime_error("wrong number of values");
    std::string out;
    for (size_t i = 0; i < row.size(); i++) {
        const Column& col = schema.columns[i];
        if (col.type == ColumnType::INT) {
            int32_t v = std::get<int32_t>(row[i]);
            out.append(reinterpret_cast<const char*>(&v), sizeof v);
        } else {
            const std::string& s = std::get<std::string>(row[i]);
            if (s.size() > col.length) throw std::runtime_error("string too long for " + col.name);
            out += s;
            out.append(col.length - s.size(), '\0');
        }
    }
    return out;
}

Row RowSerializer::Deserialize(const Schema& schema, const char* data) {
    Row row;
    size_t off = 0;
    for (const Column& col : schema.columns) {
        if (col.type == ColumnType::INT) {
            int32_t v;
            std::memcpy(&v, data + off, sizeof v);
            row.push_back(v);
            off += sizeof v;
        } else {
            std::string s(data + off, col.length);
            size_t end = s.find('\0');   
            if (end != std::string::npos) s.resize(end);
            row.push_back(s);
            off += col.length;
        }
    }
    return row;
}