#include "MatrixUploader.h"

#include <limits>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/GL/VBO.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Camera.h"


void ProxyProjectileListener::RenderProjectileCreated(const CProjectile* projectile)
{
	MatrixUploader::GetInstance().visibleProjectilesSet.emplace(projectile);
}

void ProxyProjectileListener::RenderProjectileDestroyed(const CProjectile* projectile)
{
	MatrixUploader::GetInstance().visibleProjectilesSet.erase(projectile);
}


//////////////////



void MatrixUploader::InitVBO(const uint32_t newElemCount)
{
	matrixSSBO = new VBO(GL_SHADER_STORAGE_BUFFER, false);
	matrixSSBO->Bind(GL_SHADER_STORAGE_BUFFER);
	matrixSSBO->New(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
	matrixSSBO->Unbind();
}


void MatrixUploader::Init()
{
	elemUpdateOffset = 0u;

	if (initialized)
		return;

	if (!MatrixUploader::Supported())
		return;

	proxyProjectileListener = new ProxyProjectileListener{};
	InitVBO(elemCount0);
	matrices.reserve(elemCount0);
	initialized = true;
}

void MatrixUploader::KillVBO()
{
	if (matrixSSBO && matrixSSBO->bound)
		matrixSSBO->Unbind();

	spring::SafeDelete(matrixSSBO);
}

void MatrixUploader::Kill()
{
	if (!MatrixUploader::Supported())
		return;

	KillVBO();
	spring::SafeDelete(proxyProjectileListener);
}

template<typename TObj>
bool MatrixUploader::IsObjectVisible(const TObj* obj)
{
	if (losHandler->globalLOS[gu->myAllyTeam])
		return true;

	if constexpr (std::is_same<TObj, CProjectile>::value) //CProjectile has no IsInLosForAllyTeam()
		return losHandler->InLos(obj, gu->myAllyTeam);
	else //else is needed. Otherwise won't compile
		return obj->IsInLosForAllyTeam(gu->myAllyTeam);
}

template<typename TObj>
bool MatrixUploader::IsInView(const TObj* obj)
{
	constexpr float leewayRadius = 16.0f;
	if constexpr (std::is_same<TObj, CProjectile>::value)
		return camera->InView(obj->drawPos, leewayRadius + obj->GetDrawRadius());
	else
		return camera->InView(obj->drawMidPos, leewayRadius + obj->GetDrawRadius());
}


template<typename TObj>
void MatrixUploader::GetVisibleObjects(std::unordered_map<int, const TObj*>& visibleObjects)
{
	visibleObjects.clear();

	if constexpr (std::is_same<TObj, CUnit>::value) {
		for (const auto* obj : unitHandler.GetActiveUnits()) {
			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleObjects[obj->id] = obj;
		}
		return;
	}

	if constexpr (std::is_same<TObj, CFeature>::value) {
		for (const int fID : featureHandler.GetActiveFeatureIDs()) {
			const auto* obj = featureHandler.GetFeature(fID);
			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleObjects[fID] = obj;
		}
		return;
	}

	//CProjectile part is a mess. TODO figure out how to retrieve an addressable ID
	if constexpr (std::is_same<TObj, CProjectile>::value) {
		int iter = 0;
		for (const auto* obj : visibleProjectilesSet) {
		//for (const int pID : visibleProjectilesSet) {
			//const auto* obj = projectileHandler.GetProjectileByUnsyncedID(pID);

			if (!obj->weapon) // is this a weapon projectile? (true implies synced true)
				continue;

			//const CWeaponProjectile* wp = p->weapon ? static_cast<const CWeaponProjectile*>(p) : nullptr;
			//const WeaponDef* wd = p->weapon ? wp->GetWeaponDef() : nullptr;

			if (obj)

			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleObjects[iter++] = obj; //TODO: use projID instead of iter
		}
	}

	static_assert("Wrong TObj in MatrixSSBO::GetVisibleObjects()");
}

uint32_t MatrixUploader::GetMatrixElemCount()
{
	return matrixSSBO->GetSize() / sizeof(CMatrix44f);
}

bool MatrixUploader::UpdateObjectDefs()
{
	if (elemUpdateOffset > 0u)
		return false; //already updated

	const auto updateObjectDefFunc = [this](auto& objDefToModel, const std::string& defName) {
		objDefToModel.clear();
		for (const auto& objDef : unitDefHandler->GetUnitDefsVec()) {
			const auto* model = objDef.LoadModel();
			if (model == nullptr)
				continue;

			objDefToModel[objDef.id] = model->name;

			if (this->modelToOffsetMap.find(model->name) != this->modelToOffsetMap.cend())  //already handled
				continue;

			const int elemBeginIndex = std::distance(matrices.cbegin(), matrices.cend());
			for (const auto* piece : model->pieces) {
				matrices.emplace_back(piece->bposeMatrix);
			}
			const int elemEndIndex = std::distance(matrices.cbegin(), matrices.cend());

			this->modelToOffsetMap[model->name] = elemBeginIndex;
			this->elemUpdateOffset = elemEndIndex;

			LOG_L(L_INFO, "MatrixUploader::%s Updated %s name %s with model %s, elemBeginIndex = %d, elemEndIndex = %d", "UpdateObjectDefs", defName.c_str(), objDef.name.c_str(), model->name.c_str(), elemBeginIndex, elemEndIndex);
		}
	};

	modelToOffsetMap.clear();

	// unitdefs & model->GetPieceMatrices()
	updateObjectDefFunc(unitDefToModel   , "UnitDef"   );
	// featureDef & model->GetPieceMatrices()
	updateObjectDefFunc(featureDefToModel, "FeatureDef");

	return true;
}

template<typename TObj>
void MatrixUploader::UpdateVisibleObjects()
{
	std::unordered_map<int, const TObj*> visibleObjects;
	GetVisibleObjects<TObj>(visibleObjects);

	if constexpr (std::is_same<TObj, CUnit>::value || std::is_same<TObj, CFeature>::value) {
		for (const auto& kv : visibleObjects) {
			const int elemBeginIndex = elemUpdateOffset + std::distance(matrices.cbegin(), matrices.cend());

			const int objID = kv.first;
			const TObj* obj = kv.second;

			matrices.emplace_back(obj->GetTransformMatrix(false, losHandler->globalLOS[gu->myAllyTeam]));
			const auto& lm = obj->localModel;
			for (const auto& lmp : lm.pieces) {
				matrices.emplace_back(lmp.GetModelSpaceMatrix());
			}

			const int elemEndIndex = elemUpdateOffset + std::distance(matrices.cbegin(), matrices.cend());

			if constexpr (std::is_same<TObj, CUnit>::value) {
				unitIDToOffsetMap[objID] = elemBeginIndex;
				//LOG("Adding new unitID %d, elemBeginIndex %d elemEndIndex %d", objID, elemBeginIndex, elemEndIndex);
			}

			if constexpr (std::is_same<TObj, CFeature>::value) {
				featureIDToOffsetMap[objID] = elemBeginIndex;
			}
		}

		return;
	}

	if constexpr (std::is_same<TObj, CProjectile>::value) {
		for (const auto& kv : visibleObjects) {
			const int elemBeginIndex = std::distance(matrices.cbegin(), matrices.cend());

			const int objID = kv.first;
			const TObj* obj = kv.second;

			matrices.emplace_back(obj->GetTransformMatrix(obj->GetProjectileType() == WEAPON_MISSILE_PROJECTILE));

			const int elemEndIndex = std::distance(matrices.cbegin(), matrices.cend());

			weaponIDToOffsetMap[objID] = elemBeginIndex;
		}

		return;
	}
}


void MatrixUploader::UpdateAndBind()
{
	if (!MatrixUploader::Supported())
		return;

	matrices.clear();
	bool updateObjectDefsNow = UpdateObjectDefs(); //will not touch bindpos matrices if already updated

	UpdateVisibleObjects<CUnit>();
	UpdateVisibleObjects<CFeature>();
	UpdateVisibleObjects<CProjectile>();

	LOG_L(L_INFO, "MatrixUploader::%s matrices.size = [%u]", __func__, static_cast<uint32_t>(matrices.size()));

	//resize
	const uint32_t matrixElemCount = GetMatrixElemCount();
	int newElemCount = 0;

	if (matrices.size() > matrixElemCount) {
		newElemCount = std::max(matrixElemCount + elemIncreaseBy, static_cast<uint32_t>(matrices.size()));
		LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %d", __func__, "up", newElemCount);
	}

	matrixSSBO->UnbindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO->GetSize());
	matrixSSBO->Bind(GL_SHADER_STORAGE_BUFFER);

	if (newElemCount > 0) {
		matrixSSBO->Resize(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
	}

	//update on the GPU
	const uint32_t neededElemByteOffset = (updateObjectDefsNow ? 0u : elemUpdateOffset) * sizeof(CMatrix44f); //map the whole buffer first time, map only varying part next time
	auto* buff = matrixSSBO->MapBuffer(neededElemByteOffset, matrices.size() * sizeof(CMatrix44f), GL_WRITE_ONLY); //matrices.size() always has the correct size no matter neededElemByteOffset
	memcpy(buff, matrices.data(), matrices.size() * sizeof(CMatrix44f));
	matrixSSBO->UnmapBuffer();

	matrixSSBO->Unbind();

	matrixSSBO->BindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO->GetSize());
}

uint32_t MatrixUploader::GetUnitDefElemOffset(int32_t unitDefID)
{
	const auto modelIter = unitDefToModel.find(unitDefID);
	if (modelIter == unitDefToModel.cend()) {
		LOG_L(L_ERROR, "MatrixUploader::%s Supplied invalid %s %d", __func__, "UnitDefID", unitDefID);
		return ~0u;
	}

	const auto offsetIter = modelToOffsetMap.find(modelIter->second);
	if (offsetIter == modelToOffsetMap.cend()) { //should never happen, TODO test and remove
		LOG_L(L_ERROR, "MatrixUploader::%s Failed to find an offset corresponding to model %s of %s %d", __func__, modelIter->second.c_str(), "UnitDefID", unitDefID);
		return ~0u;
	}

	return offsetIter->second;
}

//TODO: DO DRY
uint32_t MatrixUploader::GetFeatureDefElemOffset(int32_t featureDefID)
{
	const auto& modelIter = featureDefToModel.find(featureDefID);
	if (modelIter == featureDefToModel.cend()) {
		LOG_L(L_ERROR, "MatrixUploader::%s Supplied invalid %s %d", __func__, "featureDefID", featureDefID);
		return ~0u;
	}

	const auto& offsetIter = modelToOffsetMap.find(modelIter->second);
	if (offsetIter == modelToOffsetMap.cend()) { //should never happen, TODO test and remove
		LOG_L(L_ERROR, "MatrixUploader::%s Failed to find an offset corresponding to model %s of %s %d", __func__, modelIter->second.c_str(), "featureDefID", featureDefID);
		return ~0u;
	}

	return offsetIter->second;
}

uint32_t MatrixUploader::GetUnitElemOffset(int32_t unitID)
{
	const auto& unitIDIter = unitIDToOffsetMap.find(unitID);
	if (unitIDIter == unitIDToOffsetMap.cend()) {
		LOG_L(L_ERROR, "MatrixUploader::%s Supplied invalid %s %d", __func__, "UnitID", unitID);
		return ~0u;
	}

	return unitIDIter->second;
}

//TODO: DO DRY
uint32_t MatrixUploader::GetFeatureElemOffset(int32_t featureID)
{
	const auto& featureIDIter = featureIDToOffsetMap.find(featureID);
	if (featureIDIter == featureIDToOffsetMap.cend()) {
		LOG_L(L_ERROR, "MatrixUploader::%s Supplied invalid %s %d", __func__, "FeatureID", featureID);
		return ~0u;
	}

	return featureIDIter->second;
}