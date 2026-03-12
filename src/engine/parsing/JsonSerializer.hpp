#pragma once

#include "engine/reflection/rayflect.hpp"
#include <string>
#include <sstream>

class JsonSerializer {
public:
    static std::string serialize(const StructDef& def, const void* ptr) {
        std::ostringstream oss;
        oss << "{\n";
        const auto& members = def.get_members();
        for (size_t i = 0; i < members.size(); ++i) {
            const auto& m = members[i];
            oss << "  \"" << m.name << "\": ";
            
            const void* member_ptr = static_cast<const char*>(ptr) + m.offset;
            
            switch (m.type) {
                case PrimitiveType::f32:
                    oss << *static_cast<const float*>(member_ptr);
                    break;
                case PrimitiveType::f64:
                    oss << *static_cast<const double*>(member_ptr);
                    break;
                case PrimitiveType::i32:
                    oss << *static_cast<const int32_t*>(member_ptr);
                    break;
                case PrimitiveType::i64:
                    oss << *static_cast<const int64_t*>(member_ptr);
                    break;
                case PrimitiveType::u32:
                    oss << *static_cast<const uint32_t*>(member_ptr);
                    break;
                case PrimitiveType::u64:
                    oss << *static_cast<const uint64_t*>(member_ptr);
                    break;
                case PrimitiveType::cstr: {
                    const char* str = *static_cast<const char* const*>(member_ptr);
                    if (str) {
                        oss << "\"" << str << "\"";
                    } else {
                        oss << "null";
                    }
                    break;
                }
            }
            
            if (i < members.size() - 1) {
                oss << ",\n";
            } else {
                oss << "\n";
            }
        }
        oss << "}";
        return oss.str();
    }
};
