/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_VBO_HPP
#define MODEL_VBO_HPP

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <utility>

#include "System/StringHash.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

class ModelVBO {
public:
	static ModelVBO& GetInstance() {
		static ModelVBO instance;
		return instance;
	}
public:
	ModelVBO() = default;
	~ModelVBO();

	template <typename TVertex, typename TIndex, typename TVertVec, typename TIndxVec>
	void UploadGeometryData(const int32_t modelID, const TVertVec& vertVec, const TIndxVec& indxVec);

	template <typename TVertex, typename TIndex>
	uint32_t GetStartIndex(const int32_t modelID);

	template <typename TVertex, typename TIndex>
	VBO* GetVertVBO(const int32_t modelID);

	template <typename TVertex, typename TIndex>
	VBO* GetIndxVBO(const int32_t modelID);
private:
	static constexpr uint32_t elemCount0 = 1 << 14u;
	static constexpr uint32_t indxCount0 = 1 << 14u;
	static constexpr uint32_t elemCounti = 1 << 11u;
	static constexpr uint32_t indxCounti = 1 << 11u;
private:
	struct ModelVBOData {
		VBO* vertVBO;
		VBO* indxVBO;
		uint32_t vertIndex;
		uint32_t indxIndex;
	};
private:
	static bool DoVBOPerModel() {
		static bool doVBOPerModel = true; //false is likely broken
		return doVBOPerModel;
	}
	template <typename TVertex, typename TIndex>
	static uint64_t GetIndex(const int32_t modelID);
private:
	template <typename TVertex, typename TIndex>
	ModelVBOData& GetMVD(const int32_t modelID);
private:
	std::unordered_map< uint64_t, ModelVBOData > map;
};

#endif /* MODEL_VBO_HPP */

inline ModelVBO::~ModelVBO() {
	for (auto kv : map) {
		spring::SafeDelete(kv.second.vertVBO);
		spring::SafeDelete(kv.second.indxVBO);
	}
	map.clear();
}

template <typename TVertex, typename TIndex>
inline uint64_t ModelVBO::GetIndex(const int32_t modelID)
{
	if (DoVBOPerModel())
		return static_cast<uint64_t>(static_cast<uint32_t>(modelID));

	uint64_t hashV = static_cast<uint64_t>(hashString(typeid(TVertex).name()));
	uint64_t hashI = static_cast<uint64_t>(hashString(typeid(TIndex).name()));

	return hashV << 32 | hashI;
}

template <typename TVertex, typename TIndex>
inline ModelVBO::ModelVBOData& ModelVBO::GetMVD(const int32_t modelID)
{
	const auto idx = ModelVBO::GetIndex<TVertex, TIndex>(modelID);
	auto iter = map.find(idx);
	if (iter != map.cend())
		return iter->second;

	VBO* vertVBO = new VBO(GL_ARRAY_BUFFER, false);
	vertVBO->New(elemCount0 * sizeof(TVertex), GL_STATIC_DRAW, nullptr);

	VBO* indxVBO = new VBO(GL_ELEMENT_ARRAY_BUFFER, false);
	indxVBO->New(indxCount0 * sizeof(TIndex), GL_STATIC_DRAW, nullptr);

	ModelVBO::ModelVBOData mvd{
		vertVBO,
		indxVBO,
		0u,
		0u
	};

	return map.emplace(idx, mvd).first->second; //first = iterator, second = MVD
}

template<typename TVertex, typename TIndex>
inline uint32_t ModelVBO::GetStartIndex(const int32_t modelID)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);
	return mvd.indxIndex;
}

template<typename TVertex, typename TIndex>
inline VBO* ModelVBO::GetVertVBO(const int32_t modelID)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);
	return mvd.vertVBO;
}

template<typename TVertex, typename TIndex>
inline VBO* ModelVBO::GetIndxVBO(const int32_t modelID)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);
	return mvd.indxVBO;
}

template<typename TVertex, typename TIndex, typename TVertVec, typename TIndxVec>
inline void ModelVBO::UploadGeometryData(const int32_t modelID, const TVertVec& vertVec, const TIndxVec& indxVec)
{
	ModelVBO::ModelVBOData& mvd = GetMVD<TVertex, TIndex>(modelID);

#define UPDATE_ITEM(T, vbo, startIdx, count0, counti, dataVec) \
	{ \
		constexpr auto tSize = sizeof(T); \
		\
		vbo->Bind(); \
		if (vbo->GetSize() < ((startIdx + dataVec.size()) * tSize)) { \
			const uint32_t newSize = (startIdx + std::max(static_cast<uint32_t>(dataVec.size()), counti)) * tSize; \
			vbo->Resize(newSize, GL_STATIC_DRAW); \
		} \
		\
		auto* memMap = vbo->MapBuffer(startIdx * tSize, dataVec.size() * tSize, GL_WRITE_ONLY); \
		memcpy(memMap, dataVec.data(), dataVec.size() * tSize); \
		vbo->UnmapBuffer(); \
		vbo->Unbind(); \
		\
		startIdx += dataVec.size(); \
	}

	UPDATE_ITEM(TVertex, mvd.vertVBO, mvd.vertIndex, elemCount0, elemCounti, vertVec);
	UPDATE_ITEM(TIndex, mvd.indxVBO, mvd.indxIndex, indxCount0, indxCounti, indxVec);

#undef UPDATE_ITEM
}