#ifndef PORSCHE_CAR_H_INCLUDED
#define PORSCHE_CAR_H_INCLUDED

#include "draw3D.h"

#include <string>
#include <iostream>

class PorscheCar {
public:
    explicit PorscheCar(const std::string& assetDir)
        : dir(assetDir),

          badge(dir + "Badge.obj"),
          base(dir + "Base.obj"),
          carbon(dir + "Carbon.obj"),
          coloured(dir + "Coulored_Geo.obj"),
          cristal(dir + "Cristal.obj"),
          engine(dir + "Engine.obj"),
          grille1(dir + "Grille1.obj"),
          grille2(dir + "Grille2.obj"),
          grille3(dir + "Grille3.obj"),
          interior(dir + "Interior_Geo.obj"),
          interiorTiling(dir + "Interior_Tilling.obj"),
          lights(dir + "Ligth_Geo.obj"),
          manufacture(dir + "Manufacture.obj"),
          paint(dir + "Paint_Geo.obj"),
          windshield(dir + "Windshield.obj"),

          wheelFL(dir + "Wheel.obj"),
          wheelFR(dir + "Wheel.obj"),
          wheelRL(dir + "Wheel.obj"),
          wheelRR(dir + "Wheel.obj")
    {
        loadTextures();
        setupMaterials();
		setupPartTransforms();
        setupGroups();
        checkModels();
    }

    bool loaded() const {
        return ok;
    }

    void setTransform(const Mat4& matrix) {
        globalTransform = matrix;

        opaqueGroup.transform.matrix = globalTransform;
        wheelGroup.transform.matrix = globalTransform;
        glassGroup.transform.matrix = globalTransform;
    }

    void update(float deltaTime, float speed) {
        wheelRotation += speed * deltaTime * wheelRotationFactor;
        updateWheelTransforms();
    }

    void draw() {
        opaqueGroup.draw();
        wheelGroup.draw();

        enableAlphaBlending();
        glDepthMask(GL_FALSE);

        glassGroup.draw();

        glDepthMask(GL_TRUE);
        disableBlending();
    }

private:
    std::string dir;
    bool ok = true;

    Mat4 globalTransform = Mat4::identity();

    ShapeGroup opaqueGroup;
    ShapeGroup wheelGroup;
    ShapeGroup glassGroup;

    GenShape badge;
    GenShape base;
    GenShape carbon;
    GenShape coloured;
    GenShape cristal;
    GenShape engine;
    GenShape grille1;
    GenShape grille2;
    GenShape grille3;
    GenShape interior;
    GenShape interiorTiling;
    GenShape lights;
    GenShape manufacture;
    GenShape paint;
    GenShape windshield;

    GenShape wheelFL;
    GenShape wheelFR;
    GenShape wheelRL;
    GenShape wheelRR;

    Texture2D badgeTex;
    Texture2D carbonTex;
    Texture2D colouredTex;
    Texture2D cristalTex;
    Texture2D engineTex;
    Texture2D grille1Tex;
    Texture2D grille2Tex;
    Texture2D grille3Tex;
    Texture2D interiorTex;
    Texture2D interiorTilingTex;
    Texture2D lightsTex;
    Texture2D manufactureTex;
    Texture2D wheelTex;

    float wheelRotation = 0.0f;
    float wheelRotationFactor = 5.0f;

	float wheelX = 0.0080f;
	float wheelY = 0.0030f;
	float wheelZFront = 0.0128f;
	float wheelZRear = -0.0145f;
	float wheelScale = 0.20f;

private:
    Texture2D loadTexture(const std::string& filename, bool flip = true) {
        TextureOptions options{};
        options.flipVertically = flip;
        options.srgb = true;

        Texture2D texture = loadTexture2D(dir + "Textures/" + filename, options);

        if (!texture.valid()) {
            std::cerr << "Error cargando textura: " << filename << "\n";
        }

        return texture;
    }

    Material makeTexturedMaterial(
        Texture2D texture,
        Color ambient = {0.75f, 0.75f, 0.75f, 1.0f},
        Color diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
        Color specular = {0.25f, 0.25f, 0.25f, 1.0f},
        float shininess = 24.0f
    ) {
        Material material{};
        material.ambient = ambient;
        material.diffuse = diffuse;
        material.specular = specular;
        material.emissive = {0.0f, 0.0f, 0.0f, 1.0f};
        material.shininess = shininess;
        material.opacity = diffuse.a;
        material.diffuseMap = texture.id;
        return material;
    }

    Material makeSimpleMaterial(
        Color ambient,
        Color diffuse,
        Color specular,
        float shininess,
        float opacity = 1.0f
    ) {
        Material material{};
        material.ambient = ambient;
        material.diffuse = diffuse;
        material.specular = specular;
        material.emissive = {0.0f, 0.0f, 0.0f, 1.0f};
        material.shininess = shininess;
        material.opacity = opacity;
        return material;
    }

    void loadTextures() {
        badgeTex = loadTexture("badges.png");
        carbonTex = loadTexture("common_carbon05_black_diff.png");
        colouredTex = loadTexture("Global_Texture_Coloured_Diffuse.png");
        cristalTex = loadTexture("Cristal_Final.png");
        engineTex = loadTexture("Porsche_918Spyder_2015_EngineA_DiffuseAOSO.png");
        grille1Tex = loadTexture("Porsche_918Spyder_2015_Grille1A_DiffuseAOSO.png");
        grille2Tex = loadTexture("Porsche_918Spyder_2015_Grille2A_DiffuseAOSO.png");
        grille3Tex = loadTexture("Porsche_918Spyder_2015_Grille3A_DiffuseAOSO.png");
        interiorTex = loadTexture("Porsche_918Spyder_2015_InteriorA_DiffuseAOSO.png");
        interiorTilingTex = loadTexture("Porsche_918Spyder_2015_InteriorTillingA_DiffuseAOSO.png");
        lightsTex = loadTexture("Porsche_918Spyder_2015_LightA_Diffuse.png");
        manufactureTex = loadTexture("Porsche_918Spyder_2015_ManufacturerPlateA_Diffuse.png");
        wheelTex = loadTexture("Wheel_Final.png");
    }

    void setupMaterials() {
        badge.color = {1.0f, 1.0f, 1.0f, 1.0f};
        badge.enableLighting(makeTexturedMaterial(badgeTex));

        carbon.color = {1.0f, 1.0f, 1.0f, 1.0f};
        carbon.enableLighting(makeTexturedMaterial(
            carbonTex,
            {0.35f, 0.35f, 0.35f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.35f, 0.35f, 0.35f, 1.0f},
            32.0f
        ));

        coloured.color = {1.0f, 1.0f, 1.0f, 1.0f};
        coloured.enableLighting(makeTexturedMaterial(colouredTex));

        engine.color = {1.0f, 1.0f, 1.0f, 1.0f};
        engine.enableLighting(makeTexturedMaterial(engineTex));

        grille1.color = {1.0f, 1.0f, 1.0f, 1.0f};
        grille1.enableLighting(makeTexturedMaterial(grille1Tex));

        grille2.color = {1.0f, 1.0f, 1.0f, 1.0f};
        grille2.enableLighting(makeTexturedMaterial(grille2Tex));

        grille3.color = {1.0f, 1.0f, 1.0f, 1.0f};
        grille3.enableLighting(makeTexturedMaterial(grille3Tex));

        interior.color = {1.0f, 1.0f, 1.0f, 1.0f};
        interior.enableLighting(makeTexturedMaterial(interiorTex));

        interiorTiling.color = {1.0f, 1.0f, 1.0f, 1.0f};
        interiorTiling.enableLighting(makeTexturedMaterial(interiorTilingTex));

        lights.color = {1.0f, 1.0f, 1.0f, 1.0f};
        lights.enableLighting(makeTexturedMaterial(
            lightsTex,
            {0.85f, 0.85f, 0.85f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.60f, 0.60f, 0.60f, 1.0f},
            48.0f
        ));

        manufacture.color = {1.0f, 1.0f, 1.0f, 1.0f};
        manufacture.enableLighting(makeTexturedMaterial(manufactureTex));

        wheelFL.color = {1.0f, 1.0f, 1.0f, 1.0f};
        wheelFR.color = {1.0f, 1.0f, 1.0f, 1.0f};
        wheelRL.color = {1.0f, 1.0f, 1.0f, 1.0f};
        wheelRR.color = {1.0f, 1.0f, 1.0f, 1.0f};

		Material wheelMat = makeTexturedMaterial(
			wheelTex,
			{0.45f, 0.45f, 0.45f, 1.0f},  
			{1.15f, 1.15f, 1.15f, 1.0f},   
			{0.55f, 0.55f, 0.55f, 1.0f},   
			48.0f                          
		);
		
		wheelFL.enableLighting(wheelMat);
		wheelFR.enableLighting(wheelMat);
		wheelRL.enableLighting(wheelMat);
		wheelRR.enableLighting(wheelMat);
		
        Material paintMat{};
		paintMat.ambient = {0.18f, 0.18f, 0.17f, 1.0f};
		paintMat.diffuse = {0.78f, 0.78f, 0.74f, 1.0f};
		paintMat.specular = {0.85f, 0.85f, 0.82f, 1.0f};
		paintMat.emissive = {0.0f, 0.0f, 0.0f, 1.0f};
		paintMat.shininess = 96.0f;
		paintMat.opacity = 1.0f;

		paint.color = {0.78f, 0.78f, 0.74f, 1.0f};
		paint.enableLighting(paintMat);

        base.color = {0.035f, 0.035f, 0.04f, 1.0f};
        base.enableLighting(makeSimpleMaterial(
            {0.04f, 0.04f, 0.045f, 1.0f},
            {0.08f, 0.08f, 0.09f, 1.0f},
            {0.18f, 0.18f, 0.18f, 1.0f},
            16.0f
        ));

		Material glassMat = makeSimpleMaterial(
			{0.12f, 0.12f, 0.12f, 0.55f}, 
			{0.28f, 0.28f, 0.30f, 0.55f}, 
			{0.90f, 0.90f, 0.95f, 1.0f},  
			96.0f,
			4.0f                          
		);

		Material cristalMat{};
		cristalMat.ambient = {0.65f, 0.65f, 0.65f, 1.0f};
		cristalMat.diffuse = {1.20f, 1.20f, 1.20f, 1.0f};
		cristalMat.specular = {0.85f, 0.85f, 0.85f, 1.0f};
		cristalMat.emissive = {0.08f, 0.08f, 0.08f, 1.0f};
		cristalMat.shininess = 72.0f;
		cristalMat.opacity = 1.0f;
		cristalMat.diffuseMap = cristalTex.id;

		cristal.color = {1.0f, 1.0f, 1.0f, 1.0f};
		cristal.enableLighting(cristalMat);


		windshield.color = {0.18f, 0.30f, 0.38f, 0.35f};
		windshield.enableLighting(glassMat);
		
}
	
	void setupPartTransforms() {
		paint.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		base.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		carbon.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		coloured.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		badge.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		engine.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		grille1.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		grille2.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		grille3.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		interior.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		interiorTiling.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		lights.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		manufacture.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		cristal.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);

		windshield.transform.matrix =
			Mat4::translate3D(0.0f, 0.0f, 0.0f) *
			Mat4::scale3D(1.0f, 1.0f, 1.0f);
	}
	

	void setupGroups() {
		
		opaqueGroup.add(&paint);
		opaqueGroup.add(&base);
		opaqueGroup.add(&carbon);
		opaqueGroup.add(&coloured);
		opaqueGroup.add(&badge);
		opaqueGroup.add(&engine);
		opaqueGroup.add(&grille1);
		opaqueGroup.add(&grille2);
		opaqueGroup.add(&grille3);
		opaqueGroup.add(&interior);
		opaqueGroup.add(&interiorTiling);
		opaqueGroup.add(&lights);
		opaqueGroup.add(&manufacture);
		opaqueGroup.add(&cristal);
		
		wheelGroup.add(&wheelFL);
		wheelGroup.add(&wheelFR);
	
		wheelGroup.add(&wheelRL);
		wheelGroup.add(&wheelRR);

		glassGroup.add(&windshield);

		setupPartTransforms();
		updateWheelTransforms();
	}

	void updateWheelTransforms() {
		const float PI = 3.14159265f;

		const float cx = 0.0580f;
		const float cy = 0.0235f;
		const float cz = 0.2725f;

		Mat4 centerWheel =
			Mat4::translate3D(-cx, -cy, -cz);

		wheelFL.transform.matrix =
			Mat4::translate3D(-wheelX, wheelY, wheelZFront) *
			Mat4::rotateY(PI) *
			Mat4::rotateX(-wheelRotation) *
			Mat4::scale3D(wheelScale, wheelScale, wheelScale) *
			centerWheel;

		wheelFR.transform.matrix =
			Mat4::translate3D(wheelX, wheelY, wheelZFront) *
			Mat4::rotateX(wheelRotation) *
			Mat4::scale3D(wheelScale, wheelScale, wheelScale) *
			centerWheel;

		wheelRL.transform.matrix =
			Mat4::translate3D(-wheelX, wheelY, wheelZRear) *
			Mat4::rotateY(PI) *
			Mat4::rotateX(-wheelRotation) *
			Mat4::scale3D(wheelScale, wheelScale, wheelScale) *
			centerWheel;

		wheelRR.transform.matrix =
			Mat4::translate3D(wheelX, wheelY, wheelZRear) *
			Mat4::rotateX(wheelRotation) *
			Mat4::scale3D(wheelScale, wheelScale, wheelScale) *
			centerWheel;
	}

    void checkOne(const GenShape& shape, const std::string& name) {
        if (!shape.loaded()) {
            ok = false;
            std::cerr << "Error cargando " << name << ": " << shape.error() << "\n";
        }
    }

    void checkModels() {
        checkOne(badge, "Badge.obj");
        checkOne(base, "Base.obj");
        checkOne(carbon, "Carbon.obj");
        checkOne(coloured, "Coulored_Geo.obj");
        checkOne(cristal, "Cristal.obj");
        checkOne(engine, "Engine.obj");
        checkOne(grille1, "Grille1.obj");
        checkOne(grille2, "Grille2.obj");
        checkOne(grille3, "Grille3.obj");
        checkOne(interior, "Interior_Geo.obj");
        checkOne(interiorTiling, "Interior_Tilling.obj");
        checkOne(lights, "Ligth_Geo.obj");
        checkOne(manufacture, "Manufacture.obj");
        checkOne(paint, "Paint_Geo.obj");
        checkOne(windshield, "Windshield.obj");
        checkOne(wheelFL, "Wheel.obj");
    }
};

#endif