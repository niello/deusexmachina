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
	extensionSerializer.AddHandler<Lights::LightsPunctual, Document>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::SerializeLightsPunctual);
	extensionSerializer.AddHandler<Lights::NodeLightPunctual, Node>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::SerializeNodeLightPunctual);
	return extensionSerializer;
}

ExtensionDeserializer KHR::GetKHRExtensionDeserializer_DEM()
{
    ExtensionDeserializer extensionDeserializer = GetKHRExtensionDeserializer();
	extensionDeserializer.AddHandler<Lights::LightsPunctual, Document>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::DeserializeLightsPunctual);
	extensionDeserializer.AddHandler<Lights::NodeLightPunctual, Node>(Lights::LIGHTS_PUNCTUAL_NAME, Lights::DeserializeNodeLightPunctual);
	return extensionDeserializer;
}

// KHR_lights_punctual

bool KHR::Lights::LightsPunctual::IsEqual(const Extension& rhs) const
{
	return &rhs == this;

	//const auto other = dynamic_cast<const LightsPunctual*>(&rhs);

	//return other != nullptr
	//	&& glTFProperty::Equals(*this, *other)
	//	&& this->type == other->type
	//	&& this->name == other->name
	//	&& this->color == other->color
	//	&& this->intensity == other->intensity
	//	&& this->range == other->range;
}

std::string KHR::Lights::SerializeLightsPunctual(const Lights::LightsPunctual& lights, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer)
{
    rapidjson::Document doc;
    auto& a = doc.GetAllocator();
	rapidjson::Value lightArrayJson(rapidjson::kArrayType);

	for (const auto& light : lights.lights)
	{
		rapidjson::Value lightJson(rapidjson::kObjectType);

		switch (light.type)
		{
			case Type::Directional: lightJson.AddMember("type", "directional", a); break;
			case Type::Point: lightJson.AddMember("type", "point", a); break;
			case Type::Spot: lightJson.AddMember("type", "spot", a); break;
		}

		if (!light.name.empty())
		{
			lightJson.AddMember("name", rapidjson::Value(light.name.c_str(), light.name.size()), a);
		}

		if (light.color != Color3(1.0f, 1.0f, 1.0f))
		{
			lightJson.AddMember("color", RapidJsonUtils::ToJsonArray(light.color, a), a);
		}

		if (light.intensity != 1.f)
		{
			lightJson.AddMember("intensity", light.intensity, a);
		}

		if (light.range.HasValue())
		{
			lightJson.AddMember("range", light.range.Get(), a);
		}

		if (light.type == Type::Spot && (light.innerConeAngle != 0.f || light.outerConeAngle != PI / 4.f))
		{
			rapidjson::Value spot(rapidjson::kObjectType);

			if (light.innerConeAngle != 0.f)
				spot.AddMember("innerConeAngle", light.innerConeAngle, a);

			if (light.outerConeAngle != PI / 4.f)
				spot.AddMember("outerConeAngle", light.outerConeAngle, a);

			lightJson.AddMember("spot", spot, a);
		}

		lightArrayJson.PushBack(lightJson, a);
	}

	rapidjson::Value extensionJson(rapidjson::kObjectType);
	extensionJson.AddMember("lights", lightArrayJson, a);

	SerializeProperty(gltfDocument, lights, extensionJson, a, extensionSerializer);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	extensionJson.Accept(writer);

    return buffer.GetString();
}

std::unique_ptr<Extension> KHR::Lights::DeserializeLightsPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer)
{
	Lights::LightsPunctual lights;

    auto doc = RapidJsonUtils::CreateDocumentFromString(json);
    const rapidjson::Value lightsObj = doc.GetObject();

	auto lightsIt = lightsObj.FindMember("lights");
	if (lightsIt != lightsObj.MemberEnd() && lightsIt->value.IsArray())
	{
		for (auto lightIt = lightsIt->value.Begin(); lightIt != lightsIt->value.End(); ++lightIt)
		{
			Light light;

			// Type

			auto typeIt = lightIt->FindMember("type");
			if (typeIt == lightIt->MemberEnd() || !typeIt->value.IsString())
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

			auto nameIt = lightIt->FindMember("name");
			if (nameIt != lightIt->MemberEnd())
				light.name = nameIt->value.GetString();

			// Color

			auto colorIt = lightIt->FindMember("color");
			if (colorIt != lightIt->MemberEnd())
			{
				std::vector<float> color;
				for (auto ait = colorIt->value.Begin(); ait != colorIt->value.End(); ++ait)
				{
					color.push_back(static_cast<float>(ait->GetDouble()));
				}
				light.color = Color3(color[0], color[1], color[2]);
			}

			// Intensity

			auto intensityIt = lightIt->FindMember("intensity");
			if (intensityIt != lightIt->MemberEnd())
				light.intensity = intensityIt->value.GetFloat();

			// Range

			auto rangeIt = lightIt->FindMember("range");
			if (rangeIt != lightIt->MemberEnd())
				light.range = rangeIt->value.GetFloat();

			// Stop

			if (light.type == Type::Spot)
			{
				auto spotIt = lightIt->FindMember("spot");
				if (spotIt != lightIt->MemberEnd() && spotIt->value.IsObject())
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

			lights.lights.push_back(std::move(light));
		}
	}

    ParseProperty(lightsObj, lights, extensionDeserializer);

    return std::make_unique<LightsPunctual>(lights);
}

bool KHR::Lights::NodeLightPunctual::IsEqual(const Extension& rhs) const
{
	const auto other = dynamic_cast<const NodeLightPunctual*>(&rhs);

	return other != nullptr
		&& glTFProperty::Equals(*this, *other)
		&& this->lightIndex == other->lightIndex;
}

std::string KHR::Lights::SerializeNodeLightPunctual(const NodeLightPunctual& light, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer)
{
	rapidjson::Document doc;
	auto& a = doc.GetAllocator();

	rapidjson::Value extensionJson(rapidjson::kObjectType);
	extensionJson.AddMember("light", light.lightIndex, a);

	SerializeProperty(gltfDocument, light, extensionJson, a, extensionSerializer);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	extensionJson.Accept(writer);

	return buffer.GetString();
}

std::unique_ptr<Extension> KHR::Lights::DeserializeNodeLightPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer)
{
	NodeLightPunctual light;

	auto doc = RapidJsonUtils::CreateDocumentFromString(json);
	const rapidjson::Value lightObj = doc.GetObject();

	auto it = lightObj.FindMember("light");
	if (it != lightObj.MemberEnd())
		light.lightIndex = it->value.GetInt();

	ParseProperty(lightObj, light, extensionDeserializer);

	return std::make_unique<NodeLightPunctual>(light);
}
