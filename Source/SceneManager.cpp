///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}


/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/PencilMaterial.jpg",
		"pencil");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/PencilMaterial2.jpg",
		"pencil2");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/TipPencil.jpg",
		"tip1");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Painting.jpg",
		"photo");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/Painting2.png",
		"photo2");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/drywall1.jpg",
		"wall");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/NightStand.png",
		"nightstand");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/plasterWall.jpg",
		"wall2");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/bookcover.png",
		"bookcover");
	BindGLTextures();

	bReturn = CreateGLTexture(
		"../../Utilities/textures/bookpaper.png",
		"bookpaper");
	BindGLTextures();


}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for preparing lights for the objects
 *  
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}


/***********************************************************
 *  DefinedObjectMaterials()
 *
 *  This method is used for preparing objects for the objects
 *  
 ***********************************************************/

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4, 0.3, 0.1);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3, 0.2, 0.1);
	woodMaterial.specularColor = glm::vec3(0.1, 0.1, 0.1);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glassMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.shininess = 35.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL paperbackMaterial;
	paperbackMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.723);
	paperbackMaterial.ambientStrength = 0.5;
	paperbackMaterial.diffuseColor = glm::vec3(0.1f, 0.12, 0.02);
	paperbackMaterial.specularColor = glm::vec3(0.02, 0.02, 0.02);
	paperbackMaterial.shininess = 5.0;
	paperbackMaterial.tag = "paperback";
	
	m_objectMaterials.push_back(paperbackMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used for preparing lights for the objects
 *  4 lights max
 ***********************************************************/
void SceneManager::SetupSceneLights() 
{
	m_pShaderManager->setVec3Value("lightSources[0].position", 8.0, 15.0, 10.0); //repositioned to be more centered in the middle of the objects from above
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.09f, 0.1f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.28f, 0.2f, 2.9f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.5f, 1.9f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 2.9); //make the strength a little more prominate
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 40.0f); //more intensity 

	m_pShaderManager->setVec3Value("lightSources[0].position", 15.0, 20.0, 1.0); //repositioned to be more centered in the middle of the objects from above
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.056f, 0.16f, 0.94f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.55f, 0.2f, 0.85f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.5f, 1.9f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.5); //make the strength a little more prominate
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 30.0f); //more intensity 

	m_pShaderManager->setBoolValue("bUseLighting", true);


}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();


	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.65, 0.50, 0.39, 1);
	SetShaderTexture("nightstand");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -7.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.831, 0.871, 0.933, 1.0);
	SetShaderTexture("wall2");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

/***********************************************************
 *
 *
 *          Plane for Photo on wall
 *
 *
 ***********************************************************/

 // set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 5.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 6.8f, -7.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.0);
	SetShaderTexture("photo2");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/




/***********************************************************
 *
 *
 *          PENCIL OBJECT (1)
 *
 *
 ***********************************************************/

 // set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 5.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.2f, .0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	// set the color for the mesh
	SetShaderColor(0.984, 0.769, 0.376, 1);
	SetShaderTexture("pencil");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//Tip of the pencil

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.45f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.92, .78, 0.62, 1);
	SetShaderColor(0.92, .78, 0.62, 1);
	SetShaderTexture("tip1");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/

	//Eraser of the pencil

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.045f, 0.2f, 2.83);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.99, 0.65, 0.59, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//cone mesh for the Lead of the pencil 

	//xyz scale for the cone mesh
	scaleXYZ = glm::vec3(0.11f, 0.45f, 0.11f);

	//set the XYZ rotation of the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = -90.0f;

	positionXYZ = glm::vec3(0.35, 0.2, -0.25f);

	//set the transformation into memory to be used to draw the meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set color of the mesh
	SetShaderColor(0.329412, 0.329411, 0.329412, 1);
	SetShaderMaterial("wood");

	//draw the mesh with the transformation for the cone
	m_basicMeshes->DrawConeMesh();


	/***********************************************************
	 *
	 *
	 *          PENCIL OBJECT (2)
	 *
	 *
	 ***********************************************************/

	 // set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 5.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.7f, 0.2f, 1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	// set the color for the mesh
	SetShaderColor(0.984, 0.769, 0.376, 1);
	SetShaderTexture("pencil2");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//Tip of the pencil

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.45f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.23f, 0.2f, 2.37f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.92, .78, 0.62, 1);
	SetShaderTexture("tip1");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/

	//part 1 of eraser topper

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.09f, 0.2f, 1.43);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.859, 0.498, 0.69, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//part 2 top of eraser topper

	//xyz scaling for the pryamid mesh
	scaleXYZ = glm::vec3(0.5, 0.6, 0.5);

	//sets rotation for the mesh
	XrotationDegrees = 0.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = -90.0;

	//sets xyz position for the mesh
	positionXYZ = glm::vec3(3.38, 0.2f, 1.45);

	//set transformations into memory to be used on the drawn meshs
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets the shade of the mesh
	SetShaderColor(0.859, 0.498, 0.69, 1.0);

	//draws mesh - pryamid
	m_basicMeshes->DrawPyramid4Mesh();

	/****************************************************************/

	//cone mesh for the Lead of the pencil 

	//xyz scale for the cone mesh
	scaleXYZ = glm::vec3(0.11f, 0.45f, 0.11f);

	//set the XYZ rotation of the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 90.0f;

	positionXYZ = glm::vec3(-2.66f, 0.2f, 2.442f);

	//set the transformation into memory to be used to draw the meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set color of the mesh
	SetShaderColor(0.329412, 0.329411, 0.329412, 1);

	//draw the mesh with the transformation for the cone
	m_basicMeshes->DrawConeMesh();


	/***********************************************************
	 *
	 *
	 *          CANDLE WARMER OBJECT
	 *
	 *
	 ***********************************************************/

	 //main base of the candle warmer
	  //xyz scale for cylinder mesh
	scaleXYZ = glm::vec3(3.5f, 4.5f, 3.5f);

	//set the XYZ roation of the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//xyz postion of the shape
	positionXYZ = glm::vec3(-3.0f, 1.5f, -2.5f);

	//set the transformation into memory to be used to draw the meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//color of shape
	SetShaderColor(0.129f, 0.0f, 0.0f, 0.75f);
	SetShaderMaterial("glass");

	//draws the cylinder mesh shape
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//bottom base of the candle warmer
	//xyz scale for cylinder mesh
	scaleXYZ = glm::vec3(2.5f, 1.5f, 2.5f);

	//set the XYZ roation of the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//xyz postion of the shape
	positionXYZ = glm::vec3(-3.0f, 0.5f, -2.5f);

	//set the transformation into memory to be used to draw the meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//color of shape
	SetShaderColor(0.129f, 0.0f, 0.0f, 0.75f);
	SetShaderMaterial("glass");

	//draws the cylinder mesh shape
	m_basicMeshes->DrawCylinderMesh();



	/***********************************************************
	*
	*
	*          BOOK OBJECT
	*
	*
	***********************************************************/

	//xyz scaling for the octagon mesh shape
	scaleXYZ = glm::vec3(1.5, 7.0, 4.5);

	//rotation for the mesh shape xyz
	XrotationDegrees = 0.0;
	YrotationDegrees = -25.0;
	ZrotationDegrees = 0.0;

	//position of the mesh shape
	positionXYZ = glm::vec3(-9.5, 3.6, -3.9);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//color of the mesh shape
	SetShaderColor(0.329412, 0.329412, 0.329412, 1);
	SetShaderTexture("bookpaper");
	SetTextureUVScale(1.0, 1.0);

	//draws the box shape mesh
	m_basicMeshes->DrawBoxMesh();

	//***********************************************************************


	//xyz scaling for the octagon mesh shape
	scaleXYZ = glm::vec3(1.6, 7.0, 4.7);

	//rotation for the mesh shape xyz
	XrotationDegrees = 0.0;
	YrotationDegrees = -42.0;
	ZrotationDegrees = 0.0;

	//position of the mesh shape
	positionXYZ = glm::vec3(-8.5, 3.6, -3.5);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//color of the mesh shape
	SetShaderColor(0.329412, 0.329412, 0.329412, 1);
	SetShaderTexture("bookpaper");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("paperback");

	//draws the box shape mesh
	m_basicMeshes->DrawBoxMesh();


	//***********************************************************************

	//xyz scaling for the octagon mesh shape
	scaleXYZ = glm::vec3(3.45, 5.0, 2.4);

	//rotation for the mesh shape xyz
	XrotationDegrees = 0.0;
	YrotationDegrees = -42.0;
	ZrotationDegrees = 90.0;

	//position of the mesh shape
	positionXYZ = glm::vec3(-7.84, 3.6, -2.98);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//color of the mesh shape
	SetShaderColor(0.7, 0.4, 0.9, 1);
	SetShaderTexture("bookcover");
	SetTextureUVScale(1, 1);
	SetShaderMaterial("paperback");

	//draws the box shape mesh
	m_basicMeshes->DrawPlaneMesh();
}
