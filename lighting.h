#ifndef PROJECT10_LIGHTING_H_INCLUDED
#define PROJECT10_LIGHTING_H_INCLUDED

// Included by draw2D.h after the shared Color, Shader, matrix, and uniform APIs.

constexpr int MAX_LIGHTS = 8;

enum class LightType {
    Directional,
    Point,
    Spot
};

class LightVec3 {
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class Material {
public:
    Color ambient  {0.15f, 0.15f, 0.15f, 1.0f};
    Color diffuse  {1.00f, 1.00f, 1.00f, 1.0f};
    Color specular {0.55f, 0.55f, 0.55f, 1.0f};
    Color emissive {0.00f, 0.00f, 0.00f, 1.0f};
    float shininess = 32.0f;
    float opacity = 1.0f;

    GLuint diffuseMap = 0;
};

class Light {
public:
    LightType type = LightType::Directional;
    bool enabled = false;
    LightVec3 position {0.0f, 2.0f, 2.0f};
    LightVec3 direction {-0.2f, -1.0f, -0.3f};
    Color color {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;

    // Maximum distance reached by point and spot lights.
    float range = 10.0f;

    // Spot cutoffs are stored as cos(angle).
    float innerCutoff = 0.976296f; // cos(12.5 deg)
    float outerCutoff = 0.953717f; // cos(17.5 deg)
};

class LightingEffects {
public:
    Color globalAmbient {0.18f, 0.18f, 0.18f, 1.0f};

    bool fogEnabled = false;
    Color fogColor {0.55f, 0.58f, 0.62f, 1.0f};
    float fogStart = 6.0f;
    float fogEnd = 18.0f;

    bool gammaCorrection = false;
    float gamma = 2.2f;
};

inline float degToRad(float deg) {
    return deg * (PI / 180.0f);
}

inline array<Light, MAX_LIGHTS>& globalLights() {
    static array<Light, MAX_LIGHTS> lights{};
    return lights;
}

inline int& globalLightCount() {
    static int count = 0;
    return count;
}

inline Material& globalDefaultMaterial() {
    static Material material{};
    return material;
}

inline LightingEffects& globalLightingEffects() {
    static LightingEffects effects{};
    return effects;
}

inline void setDefaultMaterial(const Material& material) {
    globalDefaultMaterial() = material;
}

inline void setLightingEffects(const LightingEffects& effects) {
    globalLightingEffects() = effects;
}

inline void clearLights() {
    for (Light& light : globalLights()) {
        light = Light{};
    }
    globalLightCount() = 0;
}

inline int addLight(const Light& light) {
    int& count = globalLightCount();
    if (count >= MAX_LIGHTS) {
        fprintf(stderr, "gl2d: maximum light count reached (%d)\n", MAX_LIGHTS);
        return -1;
    }
    Light copy = light;
    copy.enabled = true;
    globalLights()[static_cast<size_t>(count)] = copy;
    return count++;
}

inline int addDirectionalLight(LightVec3 direction,
                               Color color = colors::White,
                               float intensity = 1.0f) {
    Light light{};
    light.type = LightType::Directional;
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    return addLight(light);
}

inline int addPointLight(LightVec3 position,
                         Color color = colors::White,
                         float intensity = 1.0f,
                         float range = 10.0f) {
    Light light{};
    light.type = LightType::Point;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.range = range;
    return addLight(light);
}

inline int addSpotLight(LightVec3 position,
                        LightVec3 direction,
                        Color color = colors::White,
                        float intensity = 1.0f,
                        float innerDegrees = 12.5f,
                        float outerDegrees = 17.5f,
                        float range = 10.0f) {
    Light light{};
    light.type = LightType::Spot;
    light.position = position;
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.innerCutoff = cos(degToRad(innerDegrees));
    light.outerCutoff = cos(degToRad(outerDegrees));
    light.range = range;
    return addLight(light);
}

inline bool setLightEnabled(int index, bool enabled) {
    if (index < 0 || index >= globalLightCount()) return false;
    globalLights()[static_cast<size_t>(index)].enabled = enabled;
    return true;
}

namespace Materials {
    inline Material Plastic() {
        Material m{};
        m.ambient = {0.05f, 0.05f, 0.05f, 1.0f};
        m.diffuse = {0.65f, 0.65f, 0.65f, 1.0f};
        m.specular = {0.45f, 0.45f, 0.45f, 1.0f};
        m.shininess = 48.0f;
        return m;
    }

    inline Material Rubber() {
        Material m{};
        m.ambient = {0.02f, 0.02f, 0.02f, 1.0f};
        m.diffuse = {0.12f, 0.12f, 0.12f, 1.0f};
        m.specular = {0.10f, 0.10f, 0.10f, 1.0f};
        m.shininess = 8.0f;
        return m;
    }

    inline Material Glow(Color glow = {1.0f, 0.1f, 0.8f, 1.0f},
                         float strength = 1.0f) {
        Material m{};
        m.ambient = {0.0f, 0.0f, 0.0f, 1.0f};
        m.diffuse = {glow.r * 0.35f, glow.g * 0.35f, glow.b * 0.35f, glow.a};
        m.specular = {0.25f, 0.25f, 0.25f, 1.0f};
        m.emissive = {glow.r * strength, glow.g * strength, glow.b * strength, glow.a};
        m.shininess = 24.0f;
        return m;
    }

    inline Material Stone() {
        Material m{};
        m.ambient = {0.18f, 0.18f, 0.20f, 1.0f};
        m.diffuse = {0.46f, 0.47f, 0.50f, 1.0f};
        m.specular = {0.08f, 0.08f, 0.08f, 1.0f};
        m.shininess = 8.0f;
        return m;
    }

    inline Material Metal() {
        Material m{};
        m.ambient = {0.12f, 0.12f, 0.12f, 1.0f};
        m.diffuse = {0.42f, 0.43f, 0.45f, 1.0f};
        m.specular = {0.90f, 0.88f, 0.82f, 1.0f};
        m.shininess = 64.0f;
        return m;
    }
}

inline void setUniformVec3(const Shader& shader, const char* name, const LightVec3& value) {
    const GLint loc = glGetUniformLocation(shader.program(), name);
    if (loc >= 0) glUniform3f(loc, value.x, value.y, value.z);
}

inline int glLightTypeValue(LightType type) {
    switch (type) {
        case LightType::Directional: return 0;
        case LightType::Point: return 1;
        case LightType::Spot: return 2;
    }
    return 0;
}

inline Shader& defaultShaderLitBlinnPhong() {
    static const char* kVert =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec4 aColor;\n"
        "layout(location = 2) in vec3 aNormal;\n"
        "layout(location = 3) in vec2 aTexCoord;\n"
        "uniform mat4 uProjection;\n"
        "uniform mat4 uView;\n"
        "uniform mat4 uModel;\n"
        "out vec4 vColor;\n"
        "out vec3 vFragPos;\n"
        "out vec3 vNormal;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "    vec4 worldPos = uModel * vec4(aPos, 1.0);\n"
        "    mat3 normalMatrix = mat3(transpose(inverse(uModel)));\n"
        "    vFragPos = worldPos.xyz;\n"
        "    vNormal = normalize(normalMatrix * aNormal);\n"
        "    vTexCoord = aTexCoord;\n"
        "    vColor = aColor;\n"
        "    gl_Position = uProjection * uView * worldPos;\n"
        "}\n";

    static const char* kFrag =
        "#version 330 core\n"
        "const int MAX_LIGHTS = 8;\n"
        "struct Light {\n"
        "    int type;\n"
        "    int enabled;\n"
        "    vec3 position;\n"
        "    vec3 direction;\n"
        "    vec4 color;\n"
        "    float intensity;\n"
        "    float range;\n"
        "    float innerCutoff;\n"
        "    float outerCutoff;\n"
        "};\n"
        "in vec4 vColor;\n"
        "in vec3 vFragPos;\n"
        "in vec3 vNormal;\n"
        "in vec2 vTexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform vec4 uColor;\n"
        "uniform mat4 uView;\n"
        "uniform vec4 uMaterialAmbient;\n"
        "uniform vec4 uMaterialDiffuse;\n"
        "uniform vec4 uMaterialSpecular;\n"
        "uniform vec4 uMaterialEmissive;\n"
        "uniform float uMaterialShininess;\n"
        "uniform float uMaterialOpacity;\n"
        "uniform int uLightCount;\n"
        "uniform Light uLights[MAX_LIGHTS];\n"
        "uniform vec4 uGlobalAmbient;\n"
        "uniform int uFogEnabled;\n"
        "uniform vec4 uFogColor;\n"
        "uniform float uFogStart;\n"
        "uniform float uFogEnd;\n"
        "uniform int uGammaCorrection;\n"
        "uniform float uGamma;\n"
        "uniform int uHasTexCoords;\n"
        "uniform int uHasDiffuseMap;\n"
        "uniform sampler2D uDiffuseMap;\n"
        "float attenuationFor(Light light, vec3 L) {\n"
        "    if (light.type == 0) return 1.0;\n"
        "    float d = length(light.position - vFragPos);\n"
        "    float x = clamp(d / max(light.range, 0.0001), 0.0, 1.0);\n"
        "    float atten = (1.0 - x * x) * (1.0 - x * x);\n"
        "    if (light.type == 2) {\n"
        "        float theta = dot(L, normalize(-light.direction));\n"
        "        float e = max(light.innerCutoff - light.outerCutoff, 0.0001);\n"
        "        atten *= clamp((theta - light.outerCutoff) / e, 0.0, 1.0);\n"
        "    }\n"
        "    return atten;\n"
        "}\n"
        "void main() {\n"
        "    vec4 diffuseTex = (uHasTexCoords == 1 && uHasDiffuseMap == 1) ? texture(uDiffuseMap, vTexCoord) : vec4(1.0);\n"
        "    vec4 baseDiffuse = uColor * vColor * uMaterialDiffuse * diffuseTex;\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 cameraPos = vec3(inverse(uView)[3]);\n"
        "    vec3 V = normalize(cameraPos - vFragPos);\n"
        "    vec3 result = uGlobalAmbient.rgb * uMaterialAmbient.rgb * baseDiffuse.rgb;\n"
        "    for (int i = 0; i < MAX_LIGHTS; ++i) {\n"
        "        if (i >= uLightCount) break;\n"
        "        Light light = uLights[i];\n"
        "        if (light.enabled == 0) continue;\n"
        "        vec3 L = light.type == 0 ? normalize(-light.direction) : normalize(light.position - vFragPos);\n"
        "        vec3 H = normalize(L + V);\n"
        "        float diff = max(dot(N, L), 0.0);\n"
        "        float spec = diff > 0.0 ? pow(max(dot(N, H), 0.0), max(uMaterialShininess, 1.0)) : 0.0;\n"
        "        float atten = attenuationFor(light, L);\n"
        "        vec3 lightRgb = light.color.rgb * light.intensity * atten;\n"
        "        result += baseDiffuse.rgb * diff * lightRgb;\n"
        "        result += uMaterialSpecular.rgb * spec * lightRgb;\n"
        "    }\n"
        "    result += uMaterialEmissive.rgb;\n"
        "    float alpha = baseDiffuse.a * uMaterialOpacity;\n"
        "    if (uFogEnabled == 1) {\n"
        "        float dist = length((uView * vec4(vFragPos, 1.0)).xyz);\n"
        "        float fogT = clamp((uFogEnd - dist) / max(uFogEnd - uFogStart, 0.0001), 0.0, 1.0);\n"
        "        result = mix(uFogColor.rgb, result, fogT);\n"
        "    }\n"
        "    if (uGammaCorrection == 1) {\n"
        "        result = pow(max(result, vec3(0.0)), vec3(1.0 / max(uGamma, 0.0001)));\n"
        "    }\n"
        "    FragColor = vec4(result, alpha);\n"
        "}\n";

    static Shader s(kVert, kFrag);
    return s;
}

inline void uploadLightingUniforms(const Shader& shader,
                                   const Color& drawColor,
                                   const Material& material,
                                   const Mat4& model,
                                   bool hasTexCoords) {
    setUniformMat4(shader, "uProjection", globalProjectionMatrix());
    setUniformMat4(shader, "uView", globalViewMatrix());
    setUniformMat4(shader, "uModel", model);
    setUniformColor(shader, "uColor", drawColor);

    setUniformColor(shader, "uMaterialAmbient", material.ambient);
    setUniformColor(shader, "uMaterialDiffuse", material.diffuse);
    setUniformColor(shader, "uMaterialSpecular", material.specular);
    setUniformColor(shader, "uMaterialEmissive", material.emissive);
    setUniform1f(shader, "uMaterialShininess", material.shininess);
    setUniform1f(shader, "uMaterialOpacity", material.opacity);

    const LightingEffects& fx = globalLightingEffects();
    setUniformColor(shader, "uGlobalAmbient", fx.globalAmbient);
    setUniform1i(shader, "uFogEnabled", fx.fogEnabled ? 1 : 0);
    setUniformColor(shader, "uFogColor", fx.fogColor);
    setUniform1f(shader, "uFogStart", fx.fogStart);
    setUniform1f(shader, "uFogEnd", fx.fogEnd);
    setUniform1i(shader, "uGammaCorrection", fx.gammaCorrection ? 1 : 0);
    setUniform1f(shader, "uGamma", fx.gamma);

    setUniform1i(shader, "uHasTexCoords", hasTexCoords ? 1 : 0);
    setUniform1i(shader, "uHasDiffuseMap", material.diffuseMap != 0 ? 1 : 0);

    setUniform1i(shader, "uLightCount", globalLightCount());
    const array<Light, MAX_LIGHTS>& lights = globalLights();
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        const Light& light = lights[static_cast<size_t>(i)];
        const string prefix = "uLights[" + to_string(i) + "].";
        setUniform1i(shader, (prefix + "type").c_str(), glLightTypeValue(light.type));
        setUniform1i(shader, (prefix + "enabled").c_str(), light.enabled ? 1 : 0);
        setUniformVec3(shader, (prefix + "position").c_str(), light.position);
        setUniformVec3(shader, (prefix + "direction").c_str(), light.direction);
        setUniformColor(shader, (prefix + "color").c_str(), light.color);
        setUniform1f(shader, (prefix + "intensity").c_str(), light.intensity);
        setUniform1f(shader, (prefix + "range").c_str(), light.range);
        setUniform1f(shader, (prefix + "innerCutoff").c_str(), light.innerCutoff);
        setUniform1f(shader, (prefix + "outerCutoff").c_str(), light.outerCutoff);
    }

    if (material.diffuseMap != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuseMap);
        setUniform1i(shader, "uDiffuseMap", 0);
    }
}

#endif // PROJECT10_LIGHTING_H_INCLUDED
