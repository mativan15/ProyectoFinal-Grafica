#ifndef PROJECT10_SKY_H_INCLUDED
#define PROJECT10_SKY_H_INCLUDED

#include "camera.h"
#include "draw3D.h"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

class Sky {
public:
    Sky(const std::string& assetDir, float radius)
        : skyRadius(radius > 0.0f ? radius : 1.0f),
          skyShader(createSkyShader()),
          skyDome(createSkyDome(skyRadius, 32, 96)),
          sunCore(14.0f, 32, 48),
          sunHalo(
            69.5f,
            96,
            {1.0f, 0.94f, 0.42f, 0.70f}, // center
            {1.0f, 0.62f, 0.22f, 0.0f}  // edge
        )                       
    {
        skyTexture = loadSkyTexture(assetDir);
        loaded_ = skyTexture.valid() && skyDome.loaded();

        skyDome.shader = &skyShader;
        setupSun();
    }

    ~Sky() {
        deleteTexture(skyTexture);
    }

    Sky(const Sky&) = delete;
    Sky& operator=(const Sky&) = delete;

    bool loaded() const {
        return loaded_;
    }

    void draw(const LightingEffects& sceneEffects) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        if (skyTexture.valid()) {
            skyShader.use();
            setUniform1i(skyShader, "uSkyTexture", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, skyTexture.id);
            skyDome.draw();
        }

        LightingEffects backgroundEffects = sceneEffects;
        backgroundEffects.fogEnabled = false;
        setLightingEffects(backgroundEffects);

        sunCore.draw();
        enableAdditiveBlending();
        sunHalo.draw();
        disableBlending();

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        setLightingEffects(sceneEffects);
    }

private:
    float skyRadius = 150.0f;
    Texture2D skyTexture;
    Shader skyShader;
    GenShape skyDome;
    Sphere sunCore;
    GlowHalo sunHalo;
    bool loaded_ = false;

    static Texture2D loadSkyTexture(const std::string& assetDir) {
        TextureOptions skyTexOptions{};
        skyTexOptions.flipVertically = true;
        skyTexOptions.wrapS = GL_REPEAT;
        skyTexOptions.wrapT = GL_CLAMP_TO_EDGE;
        skyTexOptions.generateMipmaps = false;
        skyTexOptions.minFilter = GL_LINEAR;

        GLint previousUnpackAlignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        Texture2D texture = loadTexture2D(
            assetDir + "Sky_Synthwave_Stars.png",
            skyTexOptions
        );
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

        if (!texture.valid()) {
            std::cerr << "Error loading Sky_Synthwave_Stars.png\n";
        }

        return texture;
    }

    static GenShape createSkyDome(float radius, int stacks, int slices) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));
        indices.reserve(static_cast<std::size_t>(stacks * slices * 6));

        for (int stack = 0; stack <= stacks; ++stack) {
            const float stackRatio = static_cast<float>(stack) / static_cast<float>(stacks);
            const float phi = 0.5f * PI * stackRatio;
            const float y = radius * std::cos(phi);
            const float ringRadius = radius * std::sin(phi);

            for (int slice = 0; slice <= slices; ++slice) {
                const float u = static_cast<float>(slice) / static_cast<float>(slices);
                const float theta = 2.0f * PI * u;
                const float x = ringRadius * std::sin(theta);
                const float z = -ringRadius * std::cos(theta);

                MeshVertex vertex{};
                vertex.position = {x, y, z};
                vertex.normal = normalizeVec3({-x, -y, -z});
                vertex.u = u;
                const float domeHeight = 1.0f - stackRatio;
                vertex.v = 0.12f + 0.88f * std::pow(domeHeight, 0.45f);
                vertices.push_back(vertex);
            }
        }

        const int rowLength = slices + 1;
        for (int stack = 0; stack < stacks; ++stack) {
            for (int slice = 0; slice < slices; ++slice) {
                const unsigned int a = static_cast<unsigned int>(stack * rowLength + slice);
                const unsigned int b = static_cast<unsigned int>((stack + 1) * rowLength + slice);
                const unsigned int c = static_cast<unsigned int>((stack + 1) * rowLength + slice + 1);
                const unsigned int d = static_cast<unsigned int>(stack * rowLength + slice + 1);

                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);
            }
        }

        return GenShape(vertices, indices);
    }

    static Shader createSkyShader() {
        static const char* vertexShader =
            "#version 330 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 3) in vec2 aTexCoord;\n"
            "uniform mat4 uProjection;\n"
            "uniform mat4 uView;\n"
            "uniform mat4 uModel;\n"
            "out vec2 vTexCoord;\n"
            "void main() {\n"
            "    vTexCoord = aTexCoord;\n"
            "    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
            "}\n";

        static const char* fragmentShader =
            "#version 330 core\n"
            "in vec2 vTexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform vec4 uColor;\n"
            "uniform sampler2D uSkyTexture;\n"
            "void main() {\n"
            "    float seamDistance = min(vTexCoord.x, 1.0 - vTexCoord.x);\n"
            "    float seamBlend = 1.0 - smoothstep(0.0, 0.14, seamDistance);\n"
            "    vec2 shiftedUv = vec2(fract(vTexCoord.x + 0.5), vTexCoord.y);\n"
            "    vec4 sky = mix(texture(uSkyTexture, vTexCoord),\n"
            "                   texture(uSkyTexture, shiftedUv), seamBlend);\n"
            "    FragColor = sky * uColor;\n"
            "}\n";

        return Shader(vertexShader, fragmentShader);
    }

    void setupSun() {
        sunCore.color = {1.0f, 0.96f, 0.78f, 1.0f};
        sunCore.enableLighting(Materials::Glow({1.0f, 0.88f, 0.42f, 1.0f}, 1.15f));  

        const Vec3 sunDirection = normalize({0.0f, 0.05f, -1.0f});
        const Vec3 sunPosition = sunDirection * skyRadius;
        const Vec3 haloDirection = normalize(sunPosition);
        const float haloPitch = -std::asin(haloDirection.y);
        const float haloYaw = std::atan2(haloDirection.x, haloDirection.z);

        sunCore.transform.matrix =
            Mat4::translate3D(sunPosition.x, sunPosition.y, sunPosition.z);
        sunHalo.transform.matrix =
            Mat4::translate3D(sunPosition.x, sunPosition.y, sunPosition.z) *
            Mat4::rotateY(haloYaw) *
            Mat4::rotateX(haloPitch);
    }
};

#endif
