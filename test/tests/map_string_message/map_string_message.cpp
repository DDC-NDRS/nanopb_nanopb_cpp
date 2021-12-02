#include <map>

#include "tests_common.h"
#include "inner_message.hpp"
#include "map_string_message.pb.h"

using namespace NanoPb::Converter;

struct LOCAL_TestMessage {
    using MapType = std::map<std::string, LOCAL_InnerMessage>;
    MapType items;

    LOCAL_TestMessage() = default;
    LOCAL_TestMessage(const LOCAL_TestMessage&) = delete;
    LOCAL_TestMessage(LOCAL_TestMessage&&) = default;
    LOCAL_TestMessage(MapType &&items) : items(std::move(items)) {}

    bool operator==(const LOCAL_TestMessage &rhs) const {
        return items == rhs.items;
    }

    bool operator!=(const LOCAL_TestMessage &rhs) const {
        return !(rhs == *this);
    }

    static std::vector<LOCAL_TestMessage> createTestMessages() {
        std::vector<LOCAL_TestMessage> ret;
        auto innerMessages = LOCAL_InnerMessage::createTestMessages();

        LOCAL_TestMessage::MapType items;

        uint32_t key = 0;
        for (auto& m: innerMessages){
            items.emplace("key_" + std::to_string(key), std::move(m));
            key++;
        }

        ret.push_back(LOCAL_TestMessage(std::move(items)));

        return ret;
    }
};

class TestMessageConverter : public AbstractMessageConverter<
        TestMessageConverter,
        LOCAL_TestMessage,
        PROTO_TestMessage,
        &PROTO_TestMessage_msg>
{
private:
    class ItemsConverter : public MapConverter<
            ItemsConverter,
            LOCAL_TestMessage::MapType,
            PROTO_TestMessage_ItemsEntry,
            &PROTO_TestMessage_ItemsEntry_msg>
    {
    public:
        static ProtoPairType itemEncoderInit(const ItemEncoderContext& localPair) {
            return ProtoPairType{
                    .key = StringConverter::encoderInit(localPair.key),
                    .has_value = true,
                    .value = InnerMessageConverter::encoderInit(localPair.value),
            };
        }
        static ProtoPairType itemDecoderInit(ItemDecoderContext& localPair){
            return ProtoPairType{
                    .key = StringConverter::decoderInit(localPair.key),
                    .value = InnerMessageConverter::decoderInit(localPair.value)
            };
        }
        static bool itemDecoderApply(const ProtoPairType& proto, ItemDecoderContext& localPair){
            InnerMessageConverter::decoderApply(proto.value, localPair.value);
            return true;
        }
    };

public:
    static ProtoType encoderInit(const LocalType& local) {
        return ProtoType{
                .items = ItemsConverter::encoderInit(local.items)
        };
    }

    static ProtoType decoderInit(LocalType& local){
        return ProtoType{
                .items = ItemsConverter::decoderInit(local.items)
        };
    }

    static bool decoderApply(const ProtoType& proto, LocalType& local){
        return true;
    }
};


int main() {
    int status = 0;

    const auto messages = LOCAL_TestMessage::createTestMessages();

    for (auto& original : messages){

        NanoPb::StringOutputStream outputStream(STRING_BUFFER_STREAM_MAX_SIZE);

        TEST(NanoPb::encode<TestMessageConverter>(outputStream, original));

        auto inputStream = NanoPb::StringInputStream(outputStream.release());

        LOCAL_TestMessage decoded;

        TEST(NanoPb::decode<TestMessageConverter>(inputStream, decoded));

        TEST(original == decoded);
    }

    return status;
}
