/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_VBO_H
#define MODEL_VBO_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <utility>

struct VBO;

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
		VBO& vertVBO;
		VBO& indxVBO;
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

#endif /* MODEL_VBO_H */