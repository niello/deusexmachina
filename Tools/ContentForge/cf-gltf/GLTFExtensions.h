#pragma once
#include <GLTFSDK/ExtensionHandlers.h>
#include <GLTFSDK/Optional.h>

namespace Microsoft::glTF::KHR
{
ExtensionSerializer   GetKHRExtensionSerializer_DEM();
ExtensionDeserializer GetKHRExtensionDeserializer_DEM();

namespace Lights
{
    constexpr const char* LIGHTS_PUNCTUAL_NAME = "KHR_lights_punctual";
	constexpr float PI = 3.14159265358979323846f;

	enum class Type
	{
		Directional,
		Point,
		Spot
	};

    struct Light
    {
		Type type;
		std::string name;
		Color3 color = { 1.f, 1.f, 1.f };
		float intensity = 1.f;
		Optional<float> range;
		float innerConeAngle = 0.f;
		float outerConeAngle = PI / 4.f;
    };

	struct LightsPunctual : Extension, glTFProperty
	{
		std::vector<Light> lights;

		std::unique_ptr<Extension> Clone() const override { return std::make_unique<LightsPunctual>(*this); }
		bool IsEqual(const Extension& rhs) const override;
	};

	std::string SerializeLightsPunctual(const LightsPunctual& lights, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer);
	std::unique_ptr<Extension> DeserializeLightsPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer);

	struct NodeLightPunctual : Extension, glTFProperty
	{
		int lightIndex = -1;

		std::unique_ptr<Extension> Clone() const override { return std::make_unique<NodeLightPunctual>(*this); }
		bool IsEqual(const Extension& rhs) const override;
	};

	std::string SerializeNodeLightPunctual(const NodeLightPunctual& light, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer);
	std::unique_ptr<Extension> DeserializeNodeLightPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer);
}

}
