#include "Vitrae/Renderers/OpenGL/Mesh.h"
#include "Vitrae/Renderers/OpenGL.h"
#include "Vitrae/Util/Types.h"
#include "Vitrae/Util/AssimpTypeConvert.h"
#include "Vitrae/Util/GraphicPrimitives.h"
#include "Vitrae/ResourceRoot.h"
#include "Vitrae/Material.h"

#include <vector>
#include <map>

namespace Vitrae
{
    OpenGLMesh::OpenGLMesh()
    {
        
    }

    OpenGLMesh::~OpenGLMesh()
    {
        const size_t s = sizeof(std::map<int, int>::iterator);
    }

    void OpenGLMesh::load(const SetupParams &params, OpenGLRenderer & rend)
    {
        // prepare vertices
        mVertices.resize(params.mesh.mNumVertices);
        mTriangles.resize(params.mesh.mNumFaces);

        // load triangles
        if (params.mesh.HasFaces()) {
            for (int i = 0; i < params.mesh.mNumFaces; i++) {
                for (int j = 0; j < params.mesh.mFaces[i].mNumIndices; j++) {
                    mTriangles[i].ind[j] = params.mesh.mFaces[i].mIndices[j];
                }
            }
        }

        // prepare OpenGL buffers
		glGenVertexArrays(1, &VAO);
        VBOs.resize(rend.getNumVertexBuffers());
		glGenBuffers(rend.getNumVertexBuffers(), VBOs.data());
		glGenBuffers(1, &EBO);


        // load vertices
		glBindVertexArray(VAO);

        auto loadVertexData = [&]<class aiType, class glmType = typename aiTypeCvt<aiType>::glmType>(
            std::vector<VertexBufferSpec<aiType>> vertexBufferSpecs,
            std::map<String, std::vector<glmType>> &namedBuffers)
        {
            for (auto &spec : vertexBufferSpecs)
            {
                // get buffers
                const aiType *src = spec.srcGetter(params.mesh);
                std::vector<glmType> &buffer = namedBuffers[spec.name];
                GLuint &vbo = VBOs[spec.layoutInd];

                // fill buffers
                namedBuffers[spec.name].resize(params.mesh.mNumVertices);
                if (src != nullptr)
                {
                    for (int i = 0; i < params.mesh.mNumVertices; i++)
                    {
                        buffer[i] = aiTypeCvt<aiType>::toGlmVal(src[i]);
                    }
                }
                if (spec.vertexStorage != nullptr)
                {
                    for (int i = 0; i < params.mesh.mNumVertices; i++)
                    {
                        mVertices[i].*(spec.vertexStorage) = aiTypeCvt<aiType>::toGlmVal(src[i]);
                    }
                }

                // send to OpenGL
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(glmType)*params.mesh.mNumVertices, buffer.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(
                    spec.layoutInd, // layout pos
                    spec.NumComponents, spec.ComponentTypeId, GL_FALSE, // data structure info
                    sizeof(glmType), (void*)0 // data location info
                );
                glEnableVertexAttribArray(spec.layoutInd);
                
            }
        };
        
        loadVertexData(rend.getVertexBufferSpecs<aiVector2D>(), namedVec2Buffers);
        loadVertexData(rend.getVertexBufferSpecs<aiVector3D>(), namedVec3Buffers);
        loadVertexData(rend.getVertexBufferSpecs<aiColor3D>(), namedVec3Buffers);
        loadVertexData(rend.getVertexBufferSpecs<aiColor4D>(), namedVec4Buffers);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * mTriangles.size(), (void *)(mTriangles.data()), GL_STATIC_DRAW);

        glBindVertexArray(0);

        // get material
        if (params.mesh.mMaterialIndex >= 0) {
            String name = toString(params.scene.mMaterials[params.mesh.mMaterialIndex]->GetName());

            auto mat = params.resRoot.getManager<Material>().getResource(name);
        }
    }

    void OpenGLMesh::setMaterial(resource_ptr<Material> mat)
    {
        mMaterial = mat;
    }

    resource_ptr<Material> OpenGLMesh::getMaterial() const
    {
        return mMaterial.value();
    }

    const std::vector<Vertex> &OpenGLMesh::getVertices() const
    {
        return mVertices;
    }
    
    const std::vector<Triangle> &OpenGLMesh::getTriangles() const
    {
        return mTriangles;
    }
    
}