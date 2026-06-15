#ifndef OBJETOS_H_INCLUDED
#define OBJETOS_H_INCLUDED

#include "draw3D.h"

#include <iostream>
#include <string>
#include <vector>

// =========================================================
// objetos.h
// ---------------------------------------------------------
// Este archivo organiza los objetos del bloque de ciudad.
// La idea es:
//   1. Cargar cada OBJ base una sola vez.
//   2. Guardar muchas matrices de instancia por cada tipo.
//   3. Dibujar cada modelo varias veces cambiando su transform.
//   4. Separar capa cercana y capa media para luego aplicar parallax.
//
// IMPORTANTE:
// Las transformaciones de Blender las colocaras manualmente aqui.
// Usa la conversion que acordamos:
//   Location OpenGL = (Blender X, Blender Z, -Blender Y)
//   Scale    OpenGL = (Blender Scale X, Blender Scale Z, Blender Scale Y)
//   Blender Rotation Z aprox. OpenGL rotateY
// =========================================================

namespace Objetos {

static constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

// ---------------------------------------------------------
// Helper para escribir transformaciones mas facil.
// Los valores ya deben estar convertidos a OpenGL.
// ---------------------------------------------------------
inline Mat4 makeTransform(
    float x, float y, float z,
    float rotXDeg = 0.0f,
    float rotYDeg = 0.0f,
    float rotZDeg = 0.0f,
    float sx = 1.0f,
    float sy = 1.0f,
    float sz = 1.0f
) {
    return
        Mat4::translate3D(x, y, -z) *
        Mat4::rotateZ(rotZDeg * DEG_TO_RAD) *
        Mat4::rotateY(rotYDeg * DEG_TO_RAD) *
        Mat4::rotateX(rotXDeg * DEG_TO_RAD) *
        Mat4::scale3D(sx, sy, sz);
}

// ---------------------------------------------------------
// Carga de textura.
// flip = true invierte verticalmente la textura.
// repeat = true sirve para hojas o texturas con UV fuera de 0..1.
// ---------------------------------------------------------
inline Texture2D loadTexture(
    const std::string& path,
    bool flip = false,
    bool repeat = false
) {
    TextureOptions options{};
    options.flipVertically = flip;
    options.srgb = true;
    options.generateMipmaps = false;
    options.minFilter = GL_LINEAR;
    options.magFilter = GL_LINEAR;

    if (repeat) {
        options.wrapS = GL_REPEAT;
        options.wrapT = GL_REPEAT;
    } else {
        options.wrapS = GL_CLAMP_TO_EDGE;
        options.wrapT = GL_CLAMP_TO_EDGE;
    }

    Texture2D texture = loadTexture2D(path, options);

    if (!texture.valid()) {
        std::cerr << "[Objetos] Error cargando textura: " << path << "\n";
    } else {
        std::cout << "[Objetos] Textura cargada: " << path
                  << " ID=" << texture.id
                  << " Size=" << texture.width << "x" << texture.height
                  << " Channels=" << texture.channels << "\n";
    }

    return texture;
}

// ---------------------------------------------------------
// Material para objetos opacos con textura.
// Road, bancas, troncos, etc.
// ---------------------------------------------------------
inline Material makeOpaqueMaterial(Texture2D texture) {
    Material material{};
    material.ambient = {0.45f, 0.45f, 0.45f, 1.0f};
    material.diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    material.specular = {0.08f, 0.08f, 0.08f, 1.0f};
    material.emissive = {0.02f, 0.02f, 0.02f, 1.0f};
    material.shininess = 10.0f;
    material.opacity = 1.0f;
    material.alphaCutoff = 0.0f;
    material.diffuseMap = texture.id;
    return material;
}

// ---------------------------------------------------------
// Material para arbol completo: tronco + hojas en el mismo OBJ.
// Ej: low_green_tree.obj y big_green_tree.obj.
// Usa un cutoff bajo para no cortar el tronco.
// ---------------------------------------------------------
inline Material makeAlphaTreeMaterial(Texture2D texture) {
    Material material{};
    material.ambient = {0.65f, 0.65f, 0.65f, 1.0f};
    material.diffuse = {1.05f, 1.05f, 1.05f, 1.0f};
    material.specular = {0.04f, 0.04f, 0.04f, 1.0f};
    material.emissive = {0.04f, 0.04f, 0.04f, 1.0f};
    material.shininess = 6.0f;
    material.opacity = 1.0f;
    material.alphaCutoff = 0.005f;
    material.diffuseMap = texture.id;
    return material;
}

// ---------------------------------------------------------
// Material para hojas separadas.
// Ej: middle_green_tree_leaves.obj, yellow_tree_leafs.obj.
// ---------------------------------------------------------
inline Material makeAlphaLeavesMaterial(Texture2D texture) {
    Material material{};
    material.ambient = {0.70f, 0.70f, 0.70f, 1.0f};
    material.diffuse = {1.05f, 1.05f, 1.05f, 1.0f};
    material.specular = {0.03f, 0.03f, 0.03f, 1.0f};
    material.emissive = {0.05f, 0.05f, 0.05f, 1.0f};
    material.shininess = 4.0f;
    material.opacity = 1.0f;
    material.alphaCutoff = 0.02f;
    material.diffuseMap = texture.id;
    return material;
}

// ---------------------------------------------------------
// Material sin textura para edificios, postes, bus stop, etc.
// ---------------------------------------------------------
inline Material makeSimpleMaterial(Color color) {
    Material material{};
    material.ambient = {color.r * 0.35f, color.g * 0.35f, color.b * 0.35f, 1.0f};
    material.diffuse = color;
    material.specular = {0.10f, 0.10f, 0.10f, 1.0f};
    material.emissive = {color.r * 0.04f, color.g * 0.04f, color.b * 0.04f, 1.0f};
    material.shininess = 8.0f;
    material.opacity = 1.0f;
    material.alphaCutoff = 0.0f;
    return material;
}

// =========================================================
// Modelo repetible: carga un OBJ una sola vez y lo dibuja
// varias veces usando distintas matrices.
// =========================================================
class RepeatedModel {
public:
    explicit RepeatedModel(const std::string& objPath)
        : model(objPath) {
        if (!model.loaded()) {
            std::cerr << "[Objetos] Error cargando OBJ: " << objPath
                      << " -> " << model.error() << "\n";
        } else {
            std::cout << "[Objetos] OBJ cargado: " << objPath << "\n";
        }

        model.color = {1.0f, 1.0f, 1.0f, 1.0f};
    }

    bool loaded() const {
        return model.loaded();
    }

    void setMaterial(const Material& material) {
        model.enableLighting(material);
    }

    void setInstances(const std::vector<Mat4>& matrices) {
        instances = matrices;
    }

    std::vector<Mat4>& transforms() {
        return instances;
    }

    const std::vector<Mat4>& transforms() const {
        return instances;
    }

    void draw(const Mat4& layerTransform = Mat4::identity()) {
        for (const Mat4& instance : instances) {
            model.transform.matrix = layerTransform * instance;
            model.draw();
        }
    }

private:
    GenShape model;
    std::vector<Mat4> instances;
};

// =========================================================
// Clases por tipo de objeto.
// Cada clase carga un modelo base y trae la cantidad de
// transformaciones que dijiste. Reemplaza cada makeTransform
// por tus valores convertidos desde Blender.
// =========================================================

class BackTowerObjects : public RepeatedModel {
public:
    explicit BackTowerObjects(const std::string& dir)
        : RepeatedModel(dir + "back_tower.obj") {
        setMaterial(makeSimpleMaterial({0.22f, 0.22f, 0.25f, 1.0f}));
        setInstances({
            makeTransform(-45.0464, -3.05723, -54.6141, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, -2.27723, -41.4941, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, -0.917226, -26.7941, 0, 0, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, -2.07723 , -24.4441 , 0, 0, 0, 1, 4.64632, 1), 
            makeTransform(-45.0464, -1.52723, -11.8041, 0, 0, 0, 1, 4.64632, 1), 
            makeTransform(-45.0464, -0.257226, -2.87411, 0, -180, 0, 1, 3.92632, 0.81), 
            makeTransform(-45.0464, -3.40723, 29.4959, 0, -180, 0, 1, 3.92632, 0.81), 
            makeTransform(-45.0464, -2.10297, 39.6522, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, -1.32297, 52.7722, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, 0.037034, 67.4722, 0, 0, 0, 1, 6.25632, 0.81), 
            makeTransform(-45.0464, -1.12297, 69.8222, 0, 0, 0, 1, 4.64632, 0.81),
			
            makeTransform(44.0464, -3.05723, -54.6141, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, -2.27723, -41.4941, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, -0.917226, -26.7941, 0, 0, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, -2.07723 , -24.4441 , 0, 0, 0, 1, 4.64632, 1), 
            makeTransform(44.0464, -1.52723, -11.8041, 0, 0, 0, 1, 4.64632, 1), 
            makeTransform(44.0464, -0.257226, -2.87411, 0, -180, 0, 1, 3.92632, 0.81), 
            makeTransform(44.0464, -3.40723, 29.4959, 0, -180, 0, 1, 3.92632, 0.81), 
            makeTransform(44.0464, -2.10297, 39.6522, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, -1.32297, 52.7722, 0, -180, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, 0.037034, 67.4722, 0, 0, 0, 1, 6.25632, 0.81), 
            makeTransform(44.0464, -1.12297, 69.8222, 0, 0, 0, 1, 4.64632, 0.81)
			
        });
    }
};

class BackConstructionObjects : public RepeatedModel {
public:
    explicit BackConstructionObjects(const std::string& dir)
        : RepeatedModel(dir + "back_construct.obj") {
        setMaterial(makeSimpleMaterial({0.28f, 0.26f, 0.24f, 1.0f}));
        setInstances({
            makeTransform(-45.0464, 6.37432, -46.1906, 0, 0, 0, 1, 2.08916, 1), 
            makeTransform(-45.0464, 3.64433, -35.4806, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(-45.0464, 4.55433, -9.6006, 0, 0, 0, 1, 1.42916, 1),
            makeTransform(-45.0464, 4.1543, -0.890599, 0, 0, 0, 1, 1.16916, 1),
            makeTransform(-45.0464, 5.88433, 6.5694, 0, 0, 0, 1, 1.98916, 1),
            makeTransform(-45.0464, 5.15433, 9.7594, 0, 0, 0, 1, 1.98916, 1),
            makeTransform(-45.0464, 3.2343, 22.5094, 0, 0, 0, 1, 1.16916, 1),
            makeTransform(-45.0464, 7.32859, 48.0757, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(-45.0464, 4.59859, 58.7857, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(-45.0464, 4.59859, 75.2957, 0, 0, 0, 1, 2.08916, 1),

            makeTransform(44.0464, 6.37432, -46.1906, 0, 0, 0, 1, 2.08916, 1), 
            makeTransform(44.0464, 3.64433, -35.4806, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(44.0464, 4.55433, -9.6006, 0, 0, 0, 1, 1.42916, 1),
            makeTransform(44.0464, 4.1543, -0.890599, 0, 0, 0, 1, 1.16916, 1),
            makeTransform(44.0464, 5.88433, 6.5694, 0, 0, 0, 1, 1.98916, 1),
            makeTransform(44.0464, 5.15433, 9.7594, 0, 0, 0, 1, 1.98916, 1),
            makeTransform(44.0464, 3.2343, 22.5094, 0, 0, 0, 1, 1.16916, 1),
            makeTransform(44.0464, 7.32859, 48.0757, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(44.0464, 4.59859, 58.7857, 0, 0, 0, 1, 2.08916, 1),
            makeTransform(44.0464, 4.59859, 75.2957, 0, 0, 0, 1, 2.08916, 1)
        });
    }
};

class RoadObjects : public RepeatedModel {
public:
    explicit RoadObjects(const std::string& dir)
        : RepeatedModel(dir + "Road.obj") {
        Texture2D tex = loadTexture(dir + "Texture/Road_Baked.png", true, false); // Road invertida
        if (tex.valid()) setMaterial(makeOpaqueMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.12f, 0.12f, 0.12f, 1.0f}));

        setInstances({
            makeTransform(0, 0, -51), // TODO 1/8
            makeTransform(0, 0, -34), // TODO 2/8
            makeTransform(0, 0, -17), // TODO 3/8
            makeTransform(0, 0, 0), // TODO 4/8
            makeTransform(0, 0, 17), // TODO 5/8
            makeTransform(0, 0, 34), // TODO 6/8
            makeTransform(0, 0, 51), // TODO 7/8
            makeTransform(0, 0, 68)  // TODO 8/8
        });
    }
};


class SidewalkObjects : public RepeatedModel {
public:
    explicit SidewalkObjects(const std::string& dir)
        : RepeatedModel(dir + "sidewalk.obj") {
        // Sidewalk_Baked.png es textura opaca. Si sale invertida, cambia false -> true.
        Texture2D tex = loadTexture(dir + "Texture/Sidewalk_Baked.png", true, false);
        if (tex.valid()) setMaterial(makeOpaqueMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.42f, 0.42f, 0.40f, 1.0f}));

        // 26 instancias. Reemplaza cada makeTransform por tus valores reales.
        // Formato: makeTransform(x, y, z, rotX, rotY, rotZ, sx, sy, sz)
        setInstances({
            makeTransform(10.6, -0.12, -52.19), // TODO 1/26
            makeTransform(10.6, -0.12, -41.84), // TODO 2/26
            makeTransform(10.6, -0.12, -31.31), // TODO 3/26
            makeTransform(10.6, -0.12, -20.85), // TODO 4/26
            makeTransform(10.6, -0.12, -10.36), // TODO 5/26
            makeTransform(10.6, -0.12, 0.129999), // TODO 6/26
            makeTransform(10.6, -0.12, 10.59 ), // TODO 7/26
            makeTransform(10.6, -0.12, 21.05), // TODO 8/26
            makeTransform(10.6, -0.12, 31.51), // TODO 9/26
            makeTransform(10.6, -0.12, 41.97), // TODO 10/26
            makeTransform(10.6, -0.12, 52.43), // TODO 11/26
            makeTransform(10.6, -0.12, 62.89), // TODO 12/26
            makeTransform(10.6, -0.12, 73.35), // TODO 13/26
            makeTransform(-7.1, -0.12,-52.19), // TODO 14/26
            makeTransform(-7.1, -0.12,-41.84), // TODO 15/26
            makeTransform(-7.1, -0.12,-31.31), // TODO 16/26
            makeTransform(-7.1, -0.12,-20.85), // TODO 17/26
            makeTransform(-7.1, -0.12,-10.36), // TODO 18/26
            makeTransform(-7.1, -0.12,0.129999), // TODO 19/26
            makeTransform(-7.1, -0.12, 10.59), // TODO 20/26
            makeTransform(-7.1, -0.12, 21.05), // TODO 21/26
            makeTransform(-7.1, -0.12,31.51), // TODO 22/26
            makeTransform(-7.1, -0.12,41.97), // TODO 23/26
            makeTransform(-7.1, -0.12,52.43), // TODO 24/26
            makeTransform(-7.1, -0.12,62.89), // TODO 25/26
            makeTransform(-7.1, -0.12,73.35)  // TODO 26/26
        });
    }
};

class BenchObjects : public RepeatedModel {
public:
    explicit BenchObjects(const std::string& dir)
        : RepeatedModel(dir + "bench.obj") {
        Texture2D tex = loadTexture(dir + "Texture/bench.png", true, false);
        if (tex.valid()) setMaterial(makeOpaqueMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.45f, 0.25f, 0.12f, 1.0f}));

        setInstances({
            makeTransform(7.5, 0.554209, -45.11, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 1/9
            makeTransform(7.5, 0.554209, -43.11, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 2/9
            makeTransform(7.5, 0.554209, -15.5743, 0, -270, 0, 0.07, 0.03, 0.03), // TODO 3/9
            makeTransform(7.5, 0.554209, 13, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 4/9
            makeTransform(7.5, 0.554209, 15, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 5/9
            makeTransform(7.5, 0.554209, 17, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 6/9
            makeTransform(7.5, 0.554209, 72.81, 0, -270, 0, 0.03, 0.03, 0.03), // TODO 7/9
            makeTransform(-7.5, 0.554209, 47, 0, -90, 0, 0.03, 0.03, 0.03), // TODO 8/9
            makeTransform(-7.5, 0.554209, 65.59, 0, -90, 0, 0.03, 0.03, 0.03)  // TODO 9/9
        });
    }
};

class LightPostObjects : public RepeatedModel {
public:
    explicit LightPostObjects(const std::string& dir)
        : RepeatedModel(dir + "lightpost.obj") {
        setMaterial(makeSimpleMaterial({0.18f, 0.18f, 0.17f, 1.0f}));
        setInstances({
            makeTransform(5.68, 2, -30, 0, 180, 0), // TODO 1/9
            makeTransform(5.68, 2, 0, 0, 180, 0), // TODO 2/9
            makeTransform(5.68, 2, 30, 0, 180, 0), // TODO 3/9
            makeTransform(5.68, 2, 60, 0, 180, 0), // TODO 4/9
            makeTransform(-5.68, 2, -30, 0, 180, 0), // TODO 5/9
            makeTransform(-5.68, 2, 0, 0, 180, 0), // TODO 6/9
            makeTransform(-5.68, 2, 30, 0, 180, 0), // TODO 7/9
            makeTransform(-5.68, 2, 60, 0, 180, 0) // TODO 8/9
        });
    }
};

class BusStopObjects : public RepeatedModel {
public:
    explicit BusStopObjects(const std::string& dir)
        : RepeatedModel(dir + "busstop.obj") {
        setMaterial(makeSimpleMaterial({0.30f, 0.30f, 0.34f, 1.0f}));
        setInstances({
            makeTransform(9.06, 2.8, 45) // TODO 1/1
        });
    }
};

class LowGreenTreeObjects : public RepeatedModel {
public:
    explicit LowGreenTreeObjects(const std::string& dir)
        : RepeatedModel(dir + "low_green_tree.obj") {
        Texture2D tex = loadTexture(dir + "Texture/Tree_Opac_Leaves_Trunk.png", false, false);
        if (tex.valid()) setMaterial(makeAlphaTreeMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.20f, 0.50f, 0.20f, 1.0f}));
        setInstances({
            makeTransform(8.28388, 2.4155, 7.95033, -90), // TODO 1/5
            makeTransform(8.28388, 2.4155, 35.8703, -90), // TODO 2/5
            makeTransform(-8.61612, 2.4155, -49.4397, -90), // TODO 3/5
            makeTransform(-8.98612, 2.4155, 53.6503, -90), // TODO 4/5
            makeTransform(-8.56612, 2.4155, 69.1303, -90)  // TODO 5/5
        });
    }
};

class BigGreenTreeObjects : public RepeatedModel {
public:
    explicit BigGreenTreeObjects(const std::string& dir)
        : RepeatedModel(dir + "big_green_tree.obj") {
        Texture2D tex = loadTexture(dir + "Texture/Tree_Opac_Leaves_Trunk.png", false, false);
        if (tex.valid()) setMaterial(makeAlphaTreeMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.20f, 0.45f, 0.20f, 1.0f}));
        setInstances({
            makeTransform(8.74569, 7.86262, -39.8483, -90), // TODO 1/5
            makeTransform(6.86569, 7.86262, -6.34828, -90), // TODO 2/5
            makeTransform(8.08569, 7.86262, 75.6617, -90), // TODO 3/5
            makeTransform(-9.08431, 7.86262, -12.7483, -90), // TODO 4/5
            makeTransform(-9.08431, 7.86262, 39.1117, -90)  // TODO 5/5
        });
    }
};

class MiddleGreenTreeWoodObjects : public RepeatedModel {
public:
    explicit MiddleGreenTreeWoodObjects(const std::string& dir)
        : RepeatedModel(dir + "middle_green_tree_wood.obj") {
        Texture2D tex = loadTexture(dir + "Texture/Tree_Opac_Leaves_Trunk.png", false, false);
        if (tex.valid()) setMaterial(makeOpaqueMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.35f, 0.20f, 0.10f, 1.0f}));
        setInstances({
            makeTransform(8.0676, 3.03897, -22.9081, -90), // TODO 1/4
            makeTransform(8.06758, 3.03897, 21.6419, -90), // TODO 2/4
            makeTransform(-8.70242, 3.03897 , -35.8981, -90), // TODO 3/4
            makeTransform(-8.70242, 3.03897 , 6.91189, -90)  // TODO 4/4
        });
    }
};

class MiddleGreenTreeLeavesObjects : public RepeatedModel {
public:
    explicit MiddleGreenTreeLeavesObjects(const std::string& dir)
        : RepeatedModel(dir + "middle_green_tree_leaves.obj") {
        Texture2D tex = loadTexture(dir + "Texture/green_leaves.png", false, true);
        if (tex.valid()) setMaterial(makeAlphaLeavesMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.20f, 0.55f, 0.20f, 1.0f}));
        setInstances({
            makeTransform(8.0676, 3.03897, -22.9081, -90), // TODO 1/4
            makeTransform(8.06758, 3.03897, 21.6419, -90), // TODO 2/4
            makeTransform(-8.70242, 3.03897 , -35.8981, -90), // TODO 3/4
            makeTransform(-8.70242, 3.03897 , 6.91189, -90)  // TODO 4/4
        });
    }
};

class YellowTreeWoodObjects : public RepeatedModel {
public:
    explicit YellowTreeWoodObjects(const std::string& dir)
        : RepeatedModel(dir + "yellow_tree_wood.obj") {
        Texture2D tex = loadTexture(dir + "Texture/Tree_Opac_Leaves_Trunk.png", false, false);
        if (tex.valid()) setMaterial(makeOpaqueMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.35f, 0.20f, 0.10f, 1.0f}));
        setInstances({
            makeTransform(8.02436, 6.50854, -49.1732, -90), // TODO 1/4
            makeTransform(8.25436, 6.50854, 51.8168, -90), // TODO 2/4
            makeTransform(-8.81564, 6.50854, 21.0868, -90) // TODO 3/4

        });
    }
};

class YellowTreeLeafObjects : public RepeatedModel {
public:
    explicit YellowTreeLeafObjects(const std::string& dir)
        : RepeatedModel(dir + "yellow_tree_leafs.obj") {
        Texture2D tex = loadTexture(dir + "Texture/yellow_leaves.png", false, true);
        if (tex.valid()) setMaterial(makeAlphaLeavesMaterial(tex));
        else setMaterial(makeSimpleMaterial({0.75f, 0.62f, 0.16f, 1.0f}));
        setInstances({
            makeTransform(8.02436, 6.50854, -49.1732, -90), // TODO 1/4
            makeTransform(8.25436, 6.50854, 51.8168, -90), // TODO 2/4
            makeTransform(-8.81564, 6.50854, 21.0868, -90) // TODO 3/4
			
        });
    }
};

class LowTowerObjects : public RepeatedModel {
public:
    explicit LowTowerObjects(const std::string& dir)
        : RepeatedModel(dir + "low_tower.obj") {
        setMaterial(makeSimpleMaterial({0.24f, 0.24f, 0.27f, 1.0f}));
        setInstances({
            makeTransform(-13.9658, -0.118336, -44.2826, 0, 0, 0, 4.0, 7.3348, 2.23), // TODO 1/5
            makeTransform(-13.9658, -0.118336, -13.8826, 0, 0, 0, 4.0, 7.3348, 4),
            makeTransform(-13.9658, -0.118336, 26.2574, 0, 0, 0, 4.0, 7.3348, 3.2),
            makeTransform(-13.9658, -0.118336, 32.5074, 0, 0, 0, 4.0, 7.3348, 2.23),
            makeTransform(-13.9658, -0.118336, 62.0974, 0, 0, 0, 4.0, 7.3348, 1.35),
            makeTransform(12.9658, -0.118336, -44.2826, 0, 0, 0, 4.0, 7.3348, 2.23), // TODO 1/5
            makeTransform(12.9658, -0.118336, -13.8826, 0, 0, 0, 4.0, 7.3348, 4),
            makeTransform(12.9658, -0.118336, 26.2574, 0, 0, 0, 4.0, 7.3348, 3.2),
            makeTransform(12.9658, -0.118336, 32.5074, 0, 0, 0, 4.0, 7.3348, 2.23),
            makeTransform(12.9658, -0.118336, 62.0974, 0, 0, 0, 4.0, 7.3348, 1.35)
			
        });
    }
};

class LowConstructionObjects : public RepeatedModel {
public:
    explicit LowConstructionObjects(const std::string& dir)
        : RepeatedModel(dir + "low_construction.obj") {
        setMaterial(makeSimpleMaterial({0.30f, 0.28f, 0.25f, 1.0f}));
        setInstances({
            makeTransform(-14.0268, 2.21, -50.843, 0, 0, 0, 4, 0.567212, 4), // TODO 1/10
            makeTransform(-14.0268, 5.71, -36.933, 0, 0, 0, 4, 1.36721, 4.38001),
            makeTransform(-14.0268, 4.34, -25.773, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(-14.0268, 5.71, -4.99304, 0, 0, 0, 4, 1.36721, 4),
            makeTransform(-14.0268, 2.21, 4.55696, 0, 0, 0, 4, 0.567212, 4),
            makeTransform(-14.0268, 4.34, 15.717, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(-14.0268, 4.34, 41.657, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(-14.0268, 2.21, 54.297, 0, 0, 0, 4, 0.567212, 5.24),
            makeTransform(-14.0268, 4.8, 66.587, 0, 0, 0, 4, 1.15721, 2.86),
            makeTransform(-14.0268, 2.63, 75.117, 0, 0, 0, 4, 0.657212, 4.46),
            makeTransform(13.0268, 2.21, -50.843, 0, 0, 0, 4, 0.567212, 4), // TODO 1/10
            makeTransform(13.0268, 5.71, -36.933, 0, 0, 0, 4, 1.36721, 4.38001),
            makeTransform(13.0268, 4.34, -25.773, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(13.0268, 5.71, -4.99304, 0, 0, 0, 4, 1.36721, 4),
            makeTransform(13.0268, 2.21, 4.55696, 0, 0, 0, 4, 0.567212, 4),
            makeTransform(13.0268, 4.34, 15.717, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(13.0268, 4.34, 41.657, 0, 0, 0, 4, 1.05721, 5.37),
            makeTransform(13.0268, 2.21, 54.297, 0, 0, 0, 4, 0.567212, 5.24),
            makeTransform(13.0268, 4.8, 66.587, 0, 0, 0, 4, 1.15721, 2.86),
            makeTransform(13.0268, 2.63, 75.117, 0, 0, 0, 4, 0.657212, 4.46)
        });
    }
};

// =========================================================
// Bloque de ciudad completo.
// capa_media: edificios del fondo, se pueden mover mas lento.
// capa_cerca: carretera y objetos cercanos, se mueven mas rapido.
//
// Nota sobre ShapeGroup:
// ShapeGroup no permite dibujar el MISMO GenShape en muchas matrices
// distintas sin cambiar su transform entre draws. Por eso cada clase
// dibuja con loops. De todas formas usamos capa_cerca y capa_media como
// matrices de capa para poder aplicar offset/parallax despues.
// =========================================================
class BloqueCiudad {
public:
    explicit BloqueCiudad(const std::string& roadDir)
        : backTowers(roadDir),
          backConstructions(roadDir),
          roads(roadDir),
          sidewalks(roadDir),
          benches(roadDir),
          lightPosts(roadDir),
          busStops(roadDir),
          lowGreenTrees(roadDir),
          bigGreenTrees(roadDir),
          middleGreenWood(roadDir),
          middleGreenLeaves(roadDir),
          yellowWood(roadDir),
          yellowLeaves(roadDir),
          lowTowers(roadDir),
          lowConstructions(roadDir) {
    }

    void setLayerOffsets(float nearZ, float mediumZ) {
        capa_cerca.transform.matrix = Mat4::translate3D(0.0f, 0.0f, nearZ);
        capa_media.transform.matrix = Mat4::translate3D(0.0f, 0.0f, mediumZ);
    }

    void draw() {
        const Mat4 nearLayer = capa_cerca.transform.matrix;
        const Mat4 mediumLayer = capa_media.transform.matrix;

        // Capa media lenta / fondo
        backTowers.draw(mediumLayer);
        backConstructions.draw(mediumLayer);

        // Capa cercana rapida
        roads.draw(nearLayer);
        sidewalks.draw(nearLayer);
        benches.draw(nearLayer);
        lightPosts.draw(nearLayer);
        busStops.draw(nearLayer);
        lowGreenTrees.draw(nearLayer);
        bigGreenTrees.draw(nearLayer);
        middleGreenWood.draw(nearLayer);
        middleGreenLeaves.draw(nearLayer);
        yellowWood.draw(nearLayer);
        yellowLeaves.draw(nearLayer);
        lowTowers.draw(nearLayer);
        lowConstructions.draw(nearLayer);
    }

    // Matrices de capa. Luego aqui aplicaremos parallax/offset infinito.
    ShapeGroup capa_cerca;
    ShapeGroup capa_media;

    // Objetos de capa media
    BackTowerObjects backTowers;
    BackConstructionObjects backConstructions;

    // Objetos de capa cercana
    RoadObjects roads;
    SidewalkObjects sidewalks;
    BenchObjects benches;
    LightPostObjects lightPosts;
    BusStopObjects busStops;
    LowGreenTreeObjects lowGreenTrees;
    BigGreenTreeObjects bigGreenTrees;
    MiddleGreenTreeWoodObjects middleGreenWood;
    MiddleGreenTreeLeavesObjects middleGreenLeaves;
    YellowTreeWoodObjects yellowWood;
    YellowTreeLeafObjects yellowLeaves;
    LowTowerObjects lowTowers;
    LowConstructionObjects lowConstructions;
};

} // namespace Objetos

#endif // OBJETOS_H_INCLUDED
