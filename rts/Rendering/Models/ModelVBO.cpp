/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModelVBO.h"

#include "System/StringHash.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "System/SafeUtil.h"

ModelVBO::~ModelVBO()
{

}

template <typename TVertex, typename TIndex>
uint64_t ModelVBO::GetIndex(const int32_t modelID)
{
	if (DoVBOPerModel())
		return static_cast<uint64_t>(static_cast<uint32_t>(modelID));

	uint64_t hashV = static_cast<uint64_t>(hashString(typeid(TVertex).name()));
	uint64_t hashI = static_cast<uint64_t>(hashString(typeid(TIndex ).name()));

	return hashV << 32 | hashI;
}

template<typename TVertex, typename TIndex>
ModelVBO::ModelVBOData& ModelVBO::GetMVD(const int32_t modelID)
{
	const auto idx = ModelVBO::GetIndex<TVertex, TIndex>(modelID);
	auto iter = map.find(idx);
	if (iter != map.cend())
		return *iter;

	VBO vertVBO = VBO(GL_ARRAY_BUFFER, false);
	vertVBO.New(elemCount0 * sizeof(TVertex), GL_STATIC_DRAW, nullptr);

	VBO indxVBO = VBO(GL_ELEMENT_ARRAY_BUFFER, false);
	indxVBO.New(indxCount0 * sizeof(TIndex), GL_STATIC_DRAW, nullptr);

	ModelVBO::ModelVBOData mvd{
		std::move(vertVBO),
		std::move(indxVBO),
		0u,
		0u
	};

	return *map.emplace(std::make_pair(idx, mvd)).first;
}

template<typename TVertex, typename TIndex>
std::pair<uint32_t, uint32_t> ModelVBO::GetStartIndices(const int32_t modelID)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);
	return std::make_pair(mvd.vertIndex, mvd.indxIndex);
}

template<typename TVertex, typename TIndex, typename TVertVec, typename TIndxVec>
void ModelVBO::UploadGeometryData(const int32_t modelID, const TVertVec& vertVec, const TIndxVec& indxVec)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);

	#define UPDATE_ITEM(T, vbo, startIdx, count0, counti, dataVec) \
	{ \
		constexpr auto tSize = sizeof(T); \
		\
		vbo.Bind(); \
		if (vbo.GetSize() < ((startIdx + dataVec.size()) * tSize)) { \
			const uint32_t newSize = (startIdx + std::max(dataVec.size(), counti)) * tSize; \
			vbo.Resize(newSize, GL_STATIC_DRAW); \
		} \
		\
		const auto* memMap = vbo.MapBuffer(startIdx * tSize, dataVec.size() * tSize, GL_WRITE_ONLY); \
		memcpy(memMap, dataVec.data(), dataVec.size() * tSize); \
		vbo.UnmapBuffer(); \
		vbo.Unbind(); \
	}

	UPDATE_ITEM(TVertex, mvd.vertVBO, mvd.vertIndex, elemCount0, elemCounti, vertVec);
	UPDATE_ITEM(TIndex,  mvd.indxVBO, mvd.indxIndex, indxCount0, indxCounti, indxVec);

	#undef UPDATE_ITEM
}


/*

template <typename TVertex, typename TVertVec, typename TIndxVec>
void ModelVBO::UploadGeometryData(const int32_t modelID, const TVertVec& vertVec, const TIndxVec& indxVec)
{
	#define UPDATE_ITEM(T, vbo, startIdx, count0, counti, dataVec) \
	{ \
		constexpr auto tSize = sizeof(T); \
		\
		vbo->Bind(); \
		if (vbo->GetSize() < ((startIdx + dataVec.size()) * tSize)) { \
			const uint32_t newSize = (startIdx + std::max(dataVec.size(), counti)) * tSize; \
			vbo->Resize(newSize, GL_STATIC_DRAW); \
		} \
		\
		const auto* memMap = vbo->MapBuffer(startIdx * tSize, dataVec.size() * tSize, GL_WRITE_ONLY); \
		memcpy(memMap, dataVec.data(), dataVec.size() * tSize); \
		vbo->UnmapBuffer(); \
		vbo->Unbind(); \
	}

	UPDATE_ITEM(TVertex , GetVertexVBO<TVertex>(modelID), GetVertexStartIndex<TVertex>(modelID), elemCount0, elemCounti, vertVec);
	UPDATE_ITEM(uint32_t, GetIndexVBO<TVertex>(modelID) , GetIndexStartIndex<TVertex>(modelID) , indxCount0, indxCounti, indxVec);

	#undef UPDATE_ITEM
}
*/