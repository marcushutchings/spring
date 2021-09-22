/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <array>
#include <vector>

#include "Rendering/GL/LightHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/UnitDrawerState.hpp"
#include "Rendering/UnitDefImage.h"
#include "System/EventClient.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"

struct SolidObjectDef;
struct UnitDef;
struct S3DModel;

class CSolidObject;
class CUnit;
class CVertexArray;

struct Command;
struct BuildInfo;
struct SolidObjectGroundDecal;
struct IUnitDrawerState;

namespace icon {
	class CIconData;
}
namespace GL {
	struct GeometryBuffer;
}


struct GhostSolidObject {
public:
	void IncRef() {        ( refCount++     ); }
	bool DecRef() { return ((refCount--) > 1); }

public:
	SolidObjectGroundDecal* decal; //FIXME defined in legacy decal handler with a lot legacy stuff
	S3DModel* model;

	float3 pos;
	float3 dir;

	int facing; //FIXME replaced with dir-vector just legacy decal drawer uses this
	int team;
	int refCount;
	int lastDrawFrame;
};




class CUnitDrawer: public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return
			eventName == "RenderUnitCreated"      || eventName == "RenderUnitDestroyed"  ||
			eventName == "UnitCloaked"            || eventName == "UnitDecloaked"        ||
			eventName == "UnitEnteredRadar"       || eventName == "UnitEnteredLos"       ||
			eventName == "UnitLeftRadar"          || eventName == "UnitLeftLos"          ||
			eventName == "PlayerChanged"          || eventName == "SunChanged";
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit*, int cloaked);
	void RenderUnitDestroyed(const CUnit*);

	void UnitEnteredRadar(const CUnit* unit, int allyTeam);
	void UnitEnteredLos(const CUnit* unit, int allyTeam);
	void UnitLeftRadar(const CUnit* unit, int allyTeam);
	void UnitLeftLos(const CUnit* unit, int allyTeam);

	void UnitCloaked(const CUnit* unit);
	void UnitDecloaked(const CUnit* unit);

	void PlayerChanged(int playerNum);
	void SunChanged();

public:
	CUnitDrawer(): CEventClient("[CUnitDrawer]", 271828, false) {}

	static void InitStatic();
	static void KillStatic(bool reload);

	void Init();
	void Kill();

	void Update();

	void UpdateGhostedBuildings();

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction);
	void DrawShadowPass();
	void DrawAlphaPass();

	void SetDrawForwardPass(bool b) { drawForward = b; }
	void SetDrawDeferredPass(bool b) { drawDeferred = b; }

	static void DrawUnitModel(const CUnit* unit, bool noLuaCall);
	static void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall);
	static void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall);
	// note: make these static?
	void DrawUnitNoTrans(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);
	void DrawUnitTrans(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);

	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);

	/// LuaOpenGL::Unit{Raw}: draw a single unit with full state setup
	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void DrawIndividual(const CUnit* unit, bool noLuaCall);
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall);

	// alpha.x := alpha-value
	// alpha.y := alpha-pass (true or false)
	void SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const;


	void SetupOpaqueDrawing(bool deferredPass);
	void ResetOpaqueDrawing(bool deferredPass);
	void SetupAlphaDrawing(bool deferredPass);
	void ResetAlphaDrawing(bool deferredPass);


	void SetUnitDrawDist(float dist) {
		unitDrawDist = dist;
		unitDrawDistSqr = unitDrawDist * unitDrawDist;
	}
	void SetUnitIconDist(float dist) {
		unitIconDist = dist;
		iconLength = unitIconDist * unitIconDist * 750.0f;
	}

	static float GetUnitIconScaleUI() { return iconScale; }
	static float GetUnitIconFadeStart() { return iconFadeStart; }
	static float GetUnitIconFadeVanish() { return iconFadeVanish; }
	static void SetUnitIconScaleUI(float scale) { iconScale = std::clamp(scale, 0.5f, 2.0f); }
	static void SetUnitIconFadeStart(float scale) { iconFadeStart = std::clamp(scale, 1.0f, 10000.0f); }
	static void SetUnitIconFadeVanish(float scale) { iconFadeVanish = std::clamp(scale, 1.0f, 10000.0f); }

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo);
	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands);

	void DrawUnitMiniMapIcons() const;


	const std::vector<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }

	ModelRenderContainer<CUnit>& GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	ModelRenderContainer<CUnit>& GetAlphaModelRenderer(int modelType) { return alphaModelRenderers[modelType]; }

	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }

	const IUnitDrawerState* GetWantedDrawerState(bool alphaPass) const;
	      IUnitDrawerState* GetDrawerState(unsigned int idx) { return unitDrawerStates[idx]; }

	void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize);
	unsigned int GetUnitDefImage(const UnitDef* unitDef);

	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool& UseAdvShadingRef() { return advShading; }
	bool& WireFrameModeRef() { return wireFrameMode; }

public:
	struct TempDrawUnit {
		const UnitDef* unitDef;

		int team;
		int facing;
		int timeout;

		float3 pos;
		float rotation;

		bool drawAlpha;
		bool drawBorder;
	};

	void AddTempDrawUnit(const TempDrawUnit& tempDrawUnit);
	void UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits);

private:
	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const;
	bool DrawAsIconScreen(CUnit* unit) const;

	bool CanDrawOpaqueUnit(const CUnit* unit, bool drawReflection, bool drawRefraction) const;
	bool CanDrawOpaqueUnitShadow(const CUnit* unit) const;

	void DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueUnitShadow(CUnit* unit);
	void DrawOpaqueUnitsShadow(int modelType);
	void DrawOpaqueUnits(int modelType, bool drawReflection, bool drawRefraction);

	void DrawAlphaUnits(int modelType);
	void DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass);

	void DrawOpaqueAIUnits(int modelType);
	void DrawOpaqueAIUnit(const TempDrawUnit& unit);
	void DrawAlphaAIUnits(int modelType);
	void DrawAlphaAIUnit(const TempDrawUnit& unit);
	void DrawAlphaAIUnitBorder(const TempDrawUnit& unit);

	void DrawGhostedBuildings(int modelType);

public:
	void DrawUnitIcons();
	void DrawUnitIconsScreen();
	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;
	void UpdateUnitDefMiniMapIcons(const UnitDef* ud);
private:
	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);
	void UpdateUnitIconState(CUnit* unit);
	void UpdateUnitIconStateScreen(CUnit* unit);

	static void DrawIcon(CUnit* unit, bool asRadarBlip);
	static void DrawIconScreenArray(const CUnit* unit, const icon::CIconData* icon, bool asRadarBlip, const float dist, CVertexArray* va);
	static void UpdateUnitDrawPos(CUnit* unit);

public:
	static void BindModelTypeTexture(int mdlType, int texType);
	static void PushModelRenderState(int mdlType);
	static void PopModelRenderState(int mdlType);

	// never called directly; combined with PushModelRenderState(S3DModel*)
	// static void BindModelTypeTexture(const S3DModel* m) { BindModelTypeTexture(m->type, m->textureType); }
	static void PushModelRenderState(const S3DModel* m);
	static void PopModelRenderState(const S3DModel* m);

	// never called directly; combined with PushModelRenderState(CSolidObject*)
	// static void BindModelTypeTexture(const CSolidObject* o) { BindModelTypeTexture(o->model); }
	static void PushModelRenderState(const CSolidObject* o);
	static void PopModelRenderState(const CSolidObject* o);

	static void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false);
	static void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false);

	// needed by FFP drawer-state
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();

	static bool ObjectVisibleReflection(const float3 objPos, const float3 camPos, float maxRadius);


public:
	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;
	float sqCamDistToGroundForIcons;
	bool useScreenIcons = false;
	bool iconHideWithUI = true;

	// .x := regular unit alpha
	// .y := ghosted unit alpha (out of radar)
	// .z := ghosted unit alpha (inside radar)
	// .w := AI-temp unit alpha
	float4 alphaValues;

private:
	bool drawForward;
	bool drawDeferred;
	bool wireFrameMode;

	bool advShading;

	bool useDistToGroundForIcons;

	// "icons as UI" fields
	static constexpr float iconSizeMult = 0.005f; // 1/200
	static float iconSizeBase;
	static float iconScale;
	static float iconFadeStart;
	static float iconFadeVanish;
	static float iconZoomDist;

private:
	typedef void (*DrawModelFunc)(const CUnit*, bool);

	std::array<ModelRenderContainer<CUnit>, MODELTYPE_CNT> opaqueModelRenderers;
	std::array<ModelRenderContainer<CUnit>, MODELTYPE_CNT> alphaModelRenderers;

	/// units being rendered (note that this is a completely
	/// unsorted set of 3DO, S3O, opaque, and cloaked models!)
	std::vector<CUnit*> unsortedUnits;

	/// AI unit ghosts
	std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempOpaqueUnits;
	std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempAlphaUnits;

	/// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::array<std::vector<GhostSolidObject*>, MODELTYPE_CNT>> deadGhostBuildings;
	/// buildings that left LOS but are still alive
	std::vector<std::array<std::vector<CUnit*>, MODELTYPE_CNT>> liveGhostBuildings;

	/// units that are only rendered as icons this frame
	std::vector<CUnit*> iconUnits;

	spring::unsynced_map<icon::CIconData*, std::vector<const CUnit*> > unitsByIcon;

	std::vector<UnitDefImage> unitDefImages;


	// caches for ShowUnitBuildSquare
	std::vector<float3> buildableSquares;
	std::vector<float3> featureSquares;
	std::vector<float3> illegalSquares;


	// [0] := no-op path
	// [1] := default shader-path
	// [2] := currently selected state
	std::array<IUnitDrawerState*, DRAWER_STATE_CNT> unitDrawerStates;
	std::array<DrawModelFunc, 3> drawModelFuncs;

private:
	GL::LightHandler lightHandler;
	GL::GeometryBuffer* geomBuffer;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
