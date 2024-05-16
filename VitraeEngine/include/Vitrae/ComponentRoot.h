#pragma once

#include "Vitrae/Types/Typedefs.h"
#include "Vitrae/Types/UniqueAnyPtr.h"
#include "Vitrae/Util/StringId.h"
#include "Vitrae/Util/UniqueId.h"

#include "dynasma/managers/abstract.hpp"
#include "dynasma/cachers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <any>
#include <map>
#include <memory>
#include <span>

namespace Vitrae
{
	class SourceShaderStep;
	class GroupShaderStep;
	class SwitchShaderStep;
	class Shader;
	class Texture;
	class Material;
	class Mesh;
	class Model;
	class RenderStep;
	class RenderPlan;

	/*
	A HUB of multiple asset managers and other components.
	One ComponentRoot must be used for all related resources
	*/
	class ComponentRoot
	{
	public:
		ComponentRoot();
		~ComponentRoot();

        /*
        === Components ===
        */

        /**
        Sets the component of a particular type and
        takes its ownership.
        @param man The component pointer to set
        */
        template <class T> void setComponent(Unique<T> &&comp)
        {
            UniqueAnyPtr &myvar = getGenericStorageVariable<T>();
            myvar = std::move(comp);
        }

        /**
        @return The component of a particular type T
        */
        template <class T> T &getComponent()
        {
            UniqueAnyPtr &myvar = getGenericStorageVariable<T>();
            return *(myvar.get<T>());
        }

        /*
        === AssImp mesh buffers ===
        */

        /**
         * A function that extracts a buffer from an aiMesh
         * @tparam aiType The type of the buffer element
         * @param extMesh The mesh to extract the buffer from
         * @returns a pointer to array of data from an aiMesh,
         * or nullptr if data cannot be found
         */
        template <class aiType>
        using AiMeshBufferExtractor = std::function<const aiType *(const aiMesh &extMesh)>;

        /**
         * Information about an aiMesh buffer
         * @tparam aiType The type of the buffer element
         */
        template <class aiType> struct AiMeshBufferInfo
        {
            /// The name of the vertex component
            StringId name;

            /// The extractor function
            AiMeshBufferExtractor<aiType> extractor;
        };

        /**
         * @tparam aiType The type of the buffer element
         * @return Span of AiMeshBufferInfo for the specified type.
         */
        template <class aiType>
        std::span<const AiMeshBufferInfo<aiType>> getAiMeshBufferInfos() const
        {
            auto &myvar = getMeshBufferInfoList<aiType>();
            return std::span(myvar);
        }

        /**
         * Adds a new AiMeshBufferInfo to the mesh buffer info list.
         * @tparam aiType The type of the buffer element
         * @param newInfo The AiMeshBufferInfo to add
         */
        template <class aiType> void addAiMeshBufferInfo(const AiMeshBufferInfo<aiType> &newInfo)
        {
            getMeshBufferInfoList<aiType>().push_back(newInfo);
        }

        /*
        === Streams ===
        */

        inline std::ostream &getErrStream()
        {
            return *mErrStream;
        }
        inline std::ostream &getInfoStream()
        {
            return *mInfoStream;
        }
        inline std::ostream &getWarningStream()
        {
            return *mWarningStream;
        }
        inline void setErrStream(std::ostream &os)
        {
            mErrStream = &os;
        }
        inline void setInfoStream(std::ostream &os)
        {
            mInfoStream = &os;
        }
        inline void setWarningStr(std::ostream &os)
        {
            mWarningStream = &os;
        }

      protected:
        /*
        Returns a variable to store the manager of a particular type.
        Is specialized to return member variables for defaultly supported types.
        */
        template <class T> UniqueAnyPtr &getGenericStorageVariable()
        {
            return mCustomComponents.at(getClassID<T>());
        }

        template <class aiType> using MeshBufferInfoList = std::vector<MeshBufferInfo<aiType>>;

        template <class aiType> MeshBufferInfoList<aiType> &getMeshBufferInfoList()
        {
            return m_aiMeshInfoLists.at(getClassID<MeshBufferInfoList<aiType>>())
                .get<MeshBufferInfoList<aiType>>();
        }

        std::map<size_t, UniqueAnyPtr> mCustomComponents;
        std::map<size_t, UniqueAnyPtr> m_aiMeshInfoLists;

        std::ostream *mErrStream, *mInfoStream, *mWarningStream;
    };
}