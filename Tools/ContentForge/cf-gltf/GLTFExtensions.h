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

    // KHR_lights_punctual
    struct LightPunctual : Extension, glTFProperty
    {
		Type type;
		std::string name;
		Color3 color = { 1.f, 1.f, 1.f };
		float intensity = 1.f;
		Optional<float> range;
		float innerConeAngle = 0.f;
		float outerConeAngle = PI / 4.f;

        std::unique_ptr<Extension> Clone() const override;
        bool IsEqual(const Extension& rhs) const override;
    };

    std::string SerializeLightPunctual(const LightPunctual& specGloss, const Document& gltfDocument, const ExtensionSerializer& extensionSerializer);
    std::unique_ptr<Extension> DeserializeLightPunctual(const std::string& json, const ExtensionDeserializer& extensionDeserializer);
}

}
