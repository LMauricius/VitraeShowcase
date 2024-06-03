#pragma once

#include "Vitrae/Pipelines/Task.h"
#include "Vitrae/Renderer.h"
#include "Vitrae/TypeConversion/AssimpTypeConvert.h"
#include "Vitrae/TypeConversion/GLTypeInfo.h"
#include "Vitrae/TypeConversion/VectorTypeInfo.h"
#include "Vitrae/Types/GraphicPrimitives.h"
#include "Vitrae/Util/StringId.h"
#include "assimp/mesh.h"

#include <functional>
#include <map>
#include <optional>
#include <typeindex>
#include <vector>

namespace Vitrae
{
class Mesh;
class Texture;

struct GLTypeSpec
{
    String glTypeName;

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
    const TypeInfo &hostType;
    const GLTypeSpec &glTypeSpec;

    void (*std140ToHost)(const GLConversionSpec *spec, const void *src, void *dst);
    void (*hostToStd140)(const GLConversionSpec *spec, const void *src, void *dst);
    dynasma::FirmPtr<SharedBuffer> (*getSharedBuffer)(const GLConversionSpec *spec,
                                                      const void *src);

    static void std140ToHostIdentity(const GLConversionSpec *spec, const void *src, void *dst);
    static void hostToStd140Identity(const GLConversionSpec *spec, const void *src, void *dst);
    static dynasma::FirmPtr<SharedBuffer> getSharedBufferRaw(const GLConversionSpec *spec,
                                                             const void *src);

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

    void setup(ComponentRoot &root) override;
    void free() override;
    void render() override;

    const GLTypeSpec &specifyGlType(const GLTypeSpec &newSpec);
    const GLTypeSpec &getGlTypeSpec(StringId glslName) const;
    const GLConversionSpec &specifyTypeConversion(const GLConversionSpec &newSpec);
    const GLConversionSpec &getTypeConversion(const TypeInfo &hostType) const;
    const std::map<StringId, GLTypeSpec> &getAllGlTypeSpecs() const;

    void specifyVertexBuffer(const PropertySpec &newElSpec);
    template <class T> void specifyVertexBufferAuto()
    {
        specifyVertexBuffer(getTypeConversion(Property::getTypeInfo<T>().p_id->name()));
    }
    std::size_t getNumVertexBuffers() const;
    std::size_t getVertexBufferLayoutIndex(StringId name) const;
    const std::map<StringId, GLConversionSpec> &getAllVertexBufferSpecs() const;

    enum class GpuValueStorageMethod
    {
        Uniform,
        UBO,
        SSBO
    };
    GpuValueStorageMethod getGpuStorageMethod(const GLTypeSpec &spec) const;

  protected:
    std::map<StringId, GLTypeSpec> m_glTypes;
    std::map<std::type_index, GLConversionSpec> m_glConversions;

    std::map<StringId, std::size_t> m_vertexBufferIndices;
    std::size_t m_vertexBufferFreeIndex;
    std::map<StringId, GLConversionSpec> m_vertexBufferSpecs;
};

} // namespace Vitrae