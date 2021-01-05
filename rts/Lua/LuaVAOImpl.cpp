#include "LuaVAOImpl.h"

#include <algorithm>

#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "lib/sol2/sol.hpp"

#if 0
#include "System/Log/ILog.h"
//			LOG("%s, %f, %p, %d, %d", attr.name.c_str(), *iter, mappedBuf, outValSize, bytesWritten);
#endif
#if 0
#include "System/TimeProfiler.h"
//			SCOPED_TIMER("LuaVAOImpl::UploadImpl::Resize");
#endif
#include "System/SafeUtil.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

LuaVAOImpl::LuaVAOImpl(sol::this_state L_)
{
	memcpy(&L[0], &L_, std::min(sizeof(sol::this_state_container), sizeof(sol::this_state)));
}

void LuaVAOImpl::Delete()
{
	//LOG_L(L_WARNING, "[LuaVAOImpl::%s]", __func__);
	if (vertLuaVBO) {
		vertLuaVBO = nullptr;
	}

	if (instLuaVBO) {
		instLuaVBO = nullptr;
	}

	if (indxLuaVBO) {
		indxLuaVBO = nullptr;
	}

	spring::SafeDestruct(vao);
}

LuaVAOImpl::~LuaVAOImpl()
{
	Delete();
}

bool LuaVAOImpl::Supported()
{
	static bool supported = VBO::IsSupported(GL_ARRAY_BUFFER) && VAO::IsSupported() && GLEW_ARB_instanced_arrays && GLEW_ARB_draw_elements_base_vertex;
	return supported;
}


void LuaVAOImpl::AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget)
{
	if (thisLuaVBO) {
		LuaError("[LuaVAOImpl::%s] LuaVBO already attached", __func__);
	}

	if (luaVBO->defTarget != reqTarget) {
		LuaError("[LuaVAOImpl::%s] LuaVBO should have been created with [%u] target, got [%u] target instead", __func__, reqTarget, luaVBO->defTarget);
	}

	if (!luaVBO->vbo) {
		LuaError("[LuaVAOImpl::%s] LuaVBO is invalid. Did you sucessfully call vbo:Define()?", __func__);
	}

	for (const auto& kv : luaVBO->bufferAttribDefsVec) {
		if (vertLuaVBO && vertLuaVBO->bufferAttribDefs.find(kv.first) != vertLuaVBO->bufferAttribDefs.cend()) {
			LuaError("[LuaVAOImpl::%s] LuaVBO attached as [%s] has defined a duplicate attribute [%d]", __func__, "vertex buffer", kv.first);
		}

		if (instLuaVBO && instLuaVBO->bufferAttribDefs.find(kv.first) != instLuaVBO->bufferAttribDefs.cend()) {
			LuaError("[LuaVAOImpl::%s] LuaVBO attached as [%s] has defined a duplicate attribute [%d]", __func__, "instance buffer", kv.first);
		}
	}

	thisLuaVBO = luaVBO;
}

void LuaVAOImpl::AttachVertexBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO)
{
	AttachBufferImpl(luaVBO, vertLuaVBO, GL_ARRAY_BUFFER);
}

void LuaVAOImpl::AttachInstanceBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO)
{
	AttachBufferImpl(luaVBO, instLuaVBO, GL_ARRAY_BUFFER);
}

void LuaVAOImpl::AttachIndexBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO)
{
	AttachBufferImpl(luaVBO, indxLuaVBO, GL_ELEMENT_ARRAY_BUFFER);
}

template<typename ...Args>
void LuaVAOImpl::LuaError(std::string format, Args ...args)
{
	luaL_error(*reinterpret_cast<sol::this_state*>(L), fmt::sprintf(format, args...).c_str());
}

void LuaVAOImpl::CheckDrawPrimitiveType(GLenum mode)
{
	switch (mode) {
	case GL_POINTS:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_LINES:
	case GL_LINE_STRIP_ADJACENCY:
	case GL_LINES_ADJACENCY:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP_ADJACENCY:
	case GL_TRIANGLES_ADJACENCY:
	case GL_PATCHES:
		break;
	default:
		LuaError("[LuaVAOImpl::%s]: Using deprecated or incorrect primitive type (%d)", __func__, mode);
	}
}

void LuaVAOImpl::CondInitVAO()
{
	if (vao)
		return; //already init

	vao = new VAO();
	vao->Bind();

	// vertLuaVBO existence is checked before
	vertLuaVBO->vbo->Bind(GL_ARRAY_BUFFER); //type is needed cause same buffer could have been rebounded as something else using LuaVBO functions

	if (indxLuaVBO)
		indxLuaVBO->vbo->Bind(GL_ELEMENT_ARRAY_BUFFER);

	#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))

	GLenum indMin = ~0u;
	GLenum indMax =  0u;

	for (const auto& va : vertLuaVBO->bufferAttribDefsVec) {
		const auto& attr = va.second;
		glEnableVertexAttribArray(va.first);
		glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, vertLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
		glVertexAttribDivisor(va.first, 0);
		indMin = std::min(indMin, static_cast<GLenum>(va.first));
		indMin = std::min(indMin, static_cast<GLenum>(va.first));
	}

	if (instLuaVBO) {
		vertLuaVBO->vbo->Unbind();
		instLuaVBO->vbo->Bind(GL_ARRAY_BUFFER);

		for (const auto& va : instLuaVBO->bufferAttribDefsVec) {
			const auto& attr = va.second;
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, instLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 1);
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
		}
	}

	#undef INT2PTR

	vao->Unbind();

	if (vertLuaVBO->vbo->bound)
		vertLuaVBO->vbo->Unbind();

	if (instLuaVBO && instLuaVBO->vbo->bound)
		instLuaVBO->vbo->Unbind();

	if (indxLuaVBO && indxLuaVBO->vbo->bound)
		indxLuaVBO->vbo->Unbind();

	//restore default state
	for (GLuint index = indMin; index <= indMax; ++index) {
		glVertexAttribDivisor(index, 0);
		glDisableVertexAttribArray(index);
	}
}

std::pair<GLsizei, GLsizei> LuaVAOImpl::DrawCheck(const GLenum mode, const sol::optional<GLsizei> drawCountOpt, const sol::optional<int> instanceCountOpt, const bool indexed)
{
	GLsizei drawCount;

	if (!vertLuaVBO)
		LuaError("[LuaVAOImpl::%s]: No vertex buffer is attached. Did you succesfully call vao:AttachVertexBuffer()?", __func__);

	if (indexed) {
		if (!indxLuaVBO)
			LuaError("[LuaVAOImpl::%s]: No index buffer is attached. Did you succesfully call vao:AttachIndexBuffer()?", __func__);

		drawCount = drawCountOpt.value_or(indxLuaVBO->elementsCount);
	} else {
		drawCount = drawCountOpt.value_or(vertLuaVBO->elementsCount);
	}

	if (drawCount <= 0)
		LuaError("[LuaVAOImpl::%s]: %s count[%d] is <= 0", __func__, indexed ? "Indices" : "Vertices", drawCount);

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()
	if (instanceCount > 0 && !instLuaVBO)
		LuaError("[LuaVAOImpl::%s]: Requested rendering of [%d] instances, but no instance buffer is attached. Did you succesfully call vao:AttachInstanceBuffer()?", __func__, instanceCount);

	CheckDrawPrimitiveType(mode);
	CondInitVAO();

	return std::make_pair(drawCount, instanceCount);
}

void LuaVAOImpl::DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt)
{
	const auto vertInstCount = DrawCheck(mode, vertCountOpt, instanceCountOpt, false); //pair<vertCount,instCount>

	const auto first = std::max(firstOpt.value_or(0), 0);

	vao->Bind();

	if (vertInstCount.second == 0)
		glDrawArrays(mode, first, vertInstCount.first);
	else
		glDrawArraysInstanced(mode, first, vertInstCount.first, vertInstCount.second);

	vao->Unbind();
}

void LuaVAOImpl::DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt)
{
	const auto indxInstCount = DrawCheck(mode, indCountOpt, instanceCountOpt, true); //pair<indxCount,instCount>

	const auto indElemOffset = std::max(indElemOffsetOpt.value_or(0), 0);
	const auto indElemOffsetInBytes = indElemOffset * indxLuaVBO->elemSizeInBytes;
	const auto baseVertex = std::max(baseVertexOpt.value_or(0), 0);

	const auto indexType = indxLuaVBO->bufferAttribDefsVec[0].second.type;

	vao->Bind();

#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))
	if (indxInstCount.second == 0) {
		if (baseVertex == 0)
			glDrawElements(mode, indxInstCount.first, indexType, INT2PTR(indElemOffsetInBytes));
		else
			glDrawElementsBaseVertex(mode, indxInstCount.first, indexType, INT2PTR(indElemOffsetInBytes), baseVertex);
	} else {
		if (baseVertex == 0)
			glDrawElementsInstanced(mode, indxInstCount.first, indexType, INT2PTR(indElemOffsetInBytes), indxInstCount.second);
		else
			glDrawElementsInstancedBaseVertex(mode, indxInstCount.first, indexType, INT2PTR(indElemOffsetInBytes), indxInstCount.second, baseVertex);
	}
#undef INT2PTR

	vao->Unbind();
}