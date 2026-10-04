#pragma once
#include <string>
#include "common/types.h"

class RowSerializer {
public:
    static std::string Serialize(const Schema& schema, const Row& row);
    static Row Deserialize(const Schema& schema, const char* data);
};