#include <cmath>
#include "src/include/ValueDeserializer.h"

#define NUM_BITS_PER_BYTE 8.0

namespace PeterDB {
    void ValueDeserializer::deserialize(const void *data, const std::vector<Attribute> &attributes,
                                        std::vector<Value> &values) {
        std::vector<bool> isAttrValueNull;
        deserializeNullFlag(data, values.size(), isAttrValueNull);
        const unsigned int nullFlagSizeBytes = computeNullFlagSizeBytes(values.size());
        deserializeValues((byte *) data + nullFlagSizeBytes, attributes, isAttrValueNull, values);
    }

    void ValueDeserializer::deserializeNullFlag(const void *data, unsigned int numAttributes,
                                                std::vector<bool> &isAttrValueNull) {
        for (int fieldNum = 0; fieldNum < numAttributes; ++fieldNum) {
            //fixme
            isAttrValueNull.push_back(false);
        }
    }

    void ValueDeserializer::deserializeValues(byte *data,
                                              const std::vector<Attribute> &attributes,
                                              const std::vector<bool> &isAttrNull,
                                              std::vector<Value> &values) {
        const unsigned int numAttributes = isAttrNull.size();
        uint32_t attrValSize = 0;
        for (int fieldNum = 0; fieldNum < numAttributes; ++fieldNum) {
            const AttrType &attributeType = attributes.at(fieldNum).type;
            Value value;
            value.type = attributeType;

            attrValSize = 0;
            if (isAttrNull.at(fieldNum)) {
                value.data = nullptr;
            } else {
                attrValSize = 4;
                if (TypeVarChar == attributeType) {
                    attrValSize += *((uint32_t*)data);
                }

                value.data = malloc(attrValSize);
                assert(value.data != nullptr);
                memcpy(&(value.data), data, attrValSize);
            }
            values.push_back(value);
            data += attrValSize;
        }
    }

    unsigned int ValueDeserializer::computeNullFlagSizeBytes(unsigned int numAttributes) {
        return ceil(numAttributes / NUM_BITS_PER_BYTE);
    }
}
