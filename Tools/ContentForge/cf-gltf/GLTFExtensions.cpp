#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#include "GLTFExtensions.h"

#include <GLTFSDK/ExtensionsKHR.h>
#include <GLTFSDK/Document.h>
#include <GLTFSDK/RapidJsonUtils.h>

using namespace Microsoft::glTF;

namespace
{
    static void ParseExtensions(const rapidjson::Value& v, glTFProperty& node, const ExtensionDeserializer& extensionDeserializer)
    {
        const auto& extensionsIt = v.FindMember("extensions");
        if (extensionsIt != v.MemberEnd())
        {
            const rapidjson::Value& extensionsObject = extensionsIt->value;
            for (const auto& entry : extensionsObject.GetObject())
            {
                ExtensionPair extensionPair = { entry.name.GetString(), Serialize(entry.value) };

                if (extensionDeserializer.HasHandler(extensionPair.name, node) ||
                    extensionDeserializer.HasHandler(extensionPair.name))
                {
                    node.SetExtension(extensionDeserializer.Deserialize(extensionPair, node));
                }
                else
                {
                    node.extensions.emplace(std::move(extensionPair.name), std::move(extensionPair.value));
                }
            }
        }
    }

	static void ParseExtras(const rapidjson::Value& v, glTFProperty& node)
    {
        rapidjson::Value::ConstMemberIterator it;
        if (TryFindMember("extras", v, it))
        {
            const rapidjson::Value& a = it->value;
            node.extras = Serialize(a);
        }
    }

	static void ParseProperty(const rapidjson::Value& v, glTFProperty& node, const ExtensionDeserializer& extensionDeserializer)
    {
        ParseExtensions(v, node, extensionDeserializer);
        ParseExtras(v, node);
    }

	static void SerializePropertyExtensions(const Document& gltfDocument, const glTFProperty& property, rapidjson::Value& propertyValue, rapidjson::Document::AllocatorType& a, const ExtensionSerializer& extensionSerializer)
    {
        auto registeredExtensions = property.GetExtensions();

        if (!property.extensions.empty() || !registeredExtensions.empty())
        {
            rapidjson::Value& extensions = RapidJsonUtils::FindOrAddMember(propertyValue, "extensions", a);

            // Add registered extensions
            for (const auto& extension : registeredExtensions)
            {
                const auto extensionPair = extensionSerializer.Serialize(extension, property, gltfDocument);

                if (property.HasUnregisteredExtension(extensionPair.name))
                {
                    throw GLTFException("Registered extension '" + extensionPair.name + "' is also present as an unregistered extension.");
                }

                if (gltfDocument.extensionsUsed.find(extensionPair.name) == gltfDocument.extensionsUsed.end())
                {
                    throw GLTFException("Registered extension '" + extensionPair.name + "' is not present in extensionsUsed");
                }

                const auto d = RapidJsonUtils::CreateDocumentFromString(extensionPair.value);//TODO: validate the returned document against the extension schema!
                rapidjson::Value v(rapidjson::kObjectType);
                v.CopyFrom(d, a);
                extensions.AddMember(RapidJsonUtils::ToStringValue(extensionPair.name, a), v, a);
            }

            // Add unregistered extensions
            for (const auto& extension : property.extensions)
            {
                const auto d = RapidJsonUtils::CreateDocumentFromString(extension.second);
                rapidjson::Value v(rapidjson::kObjectType);
                v.CopyFrom(d, a);
                extensions.AddMember(RapidJsonUtils::ToStringValue(extension.first, a), v, a);
            }
        }
    }

	static void SerializePropertyExtras(const glTFProperty& property, rapidjson::Value& propertyValue, rapidjson::Document::AllocatorType& a)
    {
        if (!property.extras.empty())
        {
            auto d = RapidJsonUtils::CreateDocumentFromString(property.extras);
            rapidjson::Value v(rapidjson::kObjectType);
            v.CopyFrom(d, a);
            propertyValue.AddMember("extras", v, a);
        }
    }

	static void SerializeProperty(const Document& gltfDocument, const glTFProperty& property, rapidjson::Value& propertyValue, rapidjson::Document::AllocatorType& a, const ExtensionSerializer& extensionSerializer)
    {
        SerializePropertyExtensions(gltfDocument, property, propertyValue, a, extensionSerializer);
        SerializePropertyExtras(property, propertyValue, a);
    }
}

ExtensionSerializer KHR::GetKHRExtensionSerializer_DEM()
{
    ExtensionSerializer extensionSerializer = GetKHRExtensionSerializer();
    extensionSerializer.AddHandler<Lights::LightPunctual, Node>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::SerializeLightPunctual);
    return extensionSerializer;
}

ExtensionDeserializer KHR::GetKHRExtensionDeserializer_DEM()
{
    ExtensionDeserializer extensionDeserializer = GetKHRExtensionDeserializer();
    extensionDeserializer.AddHandler<Lights::LightPunctual, Node>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::DeserializeLightPunctual);
    return extensionDeserializer;
}

// KHR::Lights::LightPunctual

std::unique_ptr<Extension> KHR::Lights::LightPunctual::Clone() const
{
    return std::make_unique<LightPunctual>(*this);
}

bool KHR::Lights::LightPunctual::IsEqual(const Extension& rhs) const
{
    const auto other = dynamic_cast<const LightPunctual*>(&rhs);

    return other != nullptr
        && glTFProperty::Equals(*this, *other)
		&& this->type == other->type
		&& this->name == other->name
		&& this->color == other->color
		&& this->intensity == other->intensity
		&& this->range == other->range;
}

std::string KHR::Lights::SerializeLightPunctual(const Lights::LightPunctual& light, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer)
{
    rapidjson::Document doc;
    auto& a = doc.GetAllocator();
    rapidjson::Value extensionJson(rapidjson::kObjectType);

	switch (light.type)
	{
		case Type::Directional: extensionJson.AddMember("type", "directional", a); break;
		case Type::Point: extensionJson.AddMember("type", "point", a); break;
		case Type::Spot: extensionJson.AddMember("type", "spot", a); break;
	}

	if (!light.name.empty())
	{
		extensionJson.AddMember("name", rapidjson::Value(light.name.c_str(), light.name.size()), a);
	}

    if (light.color != Color3(1.0f, 1.0f, 1.0f))
    {
		extensionJson.AddMember("color", RapidJsonUtils::ToJsonArray(light.color, a), a);
    }

	if (light.intensity != 1.f)
	{
		extensionJson.AddMember("intensity", light.intensity, a);
	}

	if (light.range.HasValue())
	{
		extensionJson.AddMember("range", light.range.Get(), a);
	}

	if (light.type == Type::Spot && (light.innerConeAngle != 0.f || light.outerConeAngle != PI / 4.f))
	{
		rapidjson::Value spot(rapidjson::kObjectType);

		if (light.innerConeAngle != 0.f)
			spot.AddMember("innerConeAngle", light.innerConeAngle, a);

		if (light.outerConeAngle != PI / 4.f)
			spot.AddMember("outerConeAngle", light.outerConeAngle, a);

		extensionJson.AddMember("spot", spot, a);
	}

    SerializeProperty(gltfDocument, light, extensionJson, a, extensionSerializer);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	extensionJson.Accept(writer);

    return buffer.GetString();
}

std::unique_ptr<Extension> KHR::Lights::DeserializeLightPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer)
{
	Lights::LightPunctual light;

    auto doc = RapidJsonUtils::CreateDocumentFromString(json);
    const rapidjson::Value sit = doc.GetObject();

	// Type

	auto typeIt = sit.FindMember("type");
	if (typeIt == sit.MemberEnd() || !typeIt->value.IsString())
		return nullptr;

	const std::string typeString = typeIt->value.GetString();
	if (typeString == "directional")
		light.type = Type::Directional;
	else if (typeString == "point")
		light.type = Type::Point;
	else if (typeString == "spot")
		light.type = Type::Spot;
	else
		return nullptr;

	// Name
	
	auto nameIt = sit.FindMember("name");
	if (nameIt != sit.MemberEnd())
		light.name = nameIt->value.GetString();

    // Color

	auto colorIt = sit.FindMember("color");
    if (colorIt != sit.MemberEnd())
    {
        std::vector<float> color;
        for (auto ait = colorIt->value.Begin(); ait != colorIt->value.End(); ++ait)
        {
			color.push_back(static_cast<float>(ait->GetDouble()));
        }
		light.color = Color3(color[0], color[1], color[2]);
    }

	// Intensity

	auto intensityIt = sit.FindMember("intensity");
	if (intensityIt != sit.MemberEnd())
		light.intensity = intensityIt->value.GetFloat();

	// Range

	auto rangeIt = sit.FindMember("range");
	if (rangeIt != sit.MemberEnd())
		light.range = rangeIt->value.GetFloat();

	if (light.type == Type::Spot)
	{
		auto spotIt = sit.FindMember("spot");
		if (spotIt != sit.MemberEnd() && spotIt->value.IsObject())
		{
			auto spotObj = spotIt->value.GetObject();

			auto innerIt = spotObj.FindMember("innerConeAngle");
			if (innerIt != spotObj.MemberEnd())
				light.innerConeAngle = innerIt->value.GetFloat();

			auto outerIt = spotObj.FindMember("outerConeAngle");
			if (outerIt != spotObj.MemberEnd())
				light.outerConeAngle = outerIt->value.GetFloat();
		}
	}

    ParseProperty(sit, light, extensionDeserializer);

    return std::make_unique<LightPunctual>(light);
}
