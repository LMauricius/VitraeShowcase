#pragma once

#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Renderer.hpp"
#include "Vitrae/TypeConversion/AssimpTypeConvert.hpp"
#include "Vitrae/TypeConversion/GLTypeInfo.hpp"
#include "Vitrae/TypeConversion/VectorTypeInfo.hpp"
#include "Vitrae/Types/GraphicPrimitives.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "assimp/mesh.h"

#include "glad/glad.h"
// must be after glad.h
#include "GLFW/glfw3.h"

#include <functional>
#include <map>
#include <optional>
#include <thread>
#include <typeindex>
#include <vector>

namespace Vitrae
{
class Mesh;
class Texture;
class RawSharedBuffer;
class ComposeTask;

struct GLTypeSpec
{
    String glMutableTypeName;
    String glConstTypeName;

    String glslDefinitionSnippet;
    std::vector<const GLTypeSpec *> memberTypeDependencies;

    std::size_t std140Size;
    std::size_t std140Alignment;
    std::size_t layoutIndexSize;

    // used only if the type is a struct and has a flexible array member
    struct FlexibleMemberSpec
    {
        const GLTypeSpec &elementGlTypeSpec;
        std::size_t maxNumElements;
    };
    std::optional<FlexibleMemberSpec> flexibleMemberSpec;
};

struct GLConversionSpec
{
    static void std140ToHostIdentity(const GLConversionSpec *spec, const void *src, void *dst);
    static void hostToStd140Identity(const GLConversionSpec *spec, const void *src, void *dst);
    static dynasma::FirmPtr<RawSharedBuffer> getSharedBufferRaw(const GLConversionSpec *spec,
                                                                const void *src);

    const TypeInfo &hostType;
    const GLTypeSpec &glTypeSpec;

    void (*setUniform)(GLint glUniformId, const Variant &hostValue) = nullptr;
    void (*setBinding)(int bindingIndex, const Variant &hostValue) = nullptr;

    // used only if the type has a flexible array member
    struct FlexibleMemberConversion
    {
        std::size_t (*getNumElements)(const void *src);
    };
    std::optional<FlexibleMemberConversion> flexibleMemberSpec;
};

class OpenGLRenderer : public Renderer
{
  public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    void mainThreadSetup(ComponentRoot &root) override;
    void mainThreadFree() override;
    void mainThreadUpdate() override;

    void anyThreadEnable() override;
    void anyThreadDisable() override;

    GLFWwindow *getWindow();

    void specifyGlType(const GLTypeSpec &newSpec);
    const GLTypeSpec &getGlTypeSpec(StringId glslName) const;
    void specifyTypeConversion(const GLConversionSpec &newSpec);
    const GLConversionSpec &getTypeConversion(const TypeInfo &hostType) const;
    const StableMap<StringId, GLTypeSpec> &getAllGlTypeSpecs() const;

    void specifyVertexBuffer(const PropertySpec &newElSpec);
    template <class T> void specifyVertexBufferAuto()
    {
        specifyVertexBuffer(getTypeConversion(Variant::getTypeInfo<T>().p_id->name()));
    }
    std::size_t getNumVertexBuffers() const;
    std::size_t getVertexBufferLayoutIndex(StringId name) const;
    const StableMap<StringId, const GLTypeSpec *> &getAllVertexBufferSpecs() const;

    enum class GpuValueStorageMethod {
        Uniform,
        UniformBinding,
        UBO,
        SSBO
    };
    GpuValueStorageMethod getGpuStorageMethod(const GLTypeSpec &spec) const;

    const StableMap<StringId, PropertySpec> &getSceneRenderInputDependencies(
        std::size_t hash) const;
    StableMap<StringId, PropertySpec> &getEditableSceneRenderInputDependencies(std::size_t hash);
    std::size_t getSceneRenderInputDependencyHash(
        const ComposeTask *p_composeTask,
        dynasma::LazyPtr<Method<ShaderTask>> p_defaultVertexMethod,
        dynasma::LazyPtr<Method<ShaderTask>> p_defaultFragmentMethod) const;

  protected:
    std::thread::id m_mainThreadId;
    std::mutex m_contextMutex;
    GLFWwindow *mp_mainWindow;

    StableMap<StringId, GLTypeSpec> m_glTypes;
    StableMap<std::type_index, GLConversionSpec> m_glConversions;

    StableMap<StringId, std::size_t> m_vertexBufferIndices;
    std::size_t m_vertexBufferFreeIndex;
    StableMap<StringId, const GLTypeSpec *> m_vertexBufferSpecs;

    mutable StableMap<std::size_t, StableMap<StringId, PropertySpec>>
        m_sceneRenderInputDependencies;
};

} // namespace Vitrae