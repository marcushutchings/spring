#include "LuaVBO.h"

#include <unordered_map>
#include <memory>

#include "lib/sol2/sol.hpp"

#include "Rendering/GL/myGL.h"
#include "LuaVBOImpl.h"
#include "LuaUtils.h"

///////////////////////////////////////////////////////////

bool LuaVBO::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetVBO);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaVBOImpl>("VBO",
		sol::constructors<LuaVBOImpl(const sol::optional<GLenum>, const sol::optional<bool>, sol::this_state)>(),
		"Delete", &LuaVBOImpl::Delete,

		"Define", &LuaVBOImpl::Define,
		"Upload", &LuaVBOImpl::Upload,
		"Download", &LuaVBOImpl::Download,

		"FromUnitDefID", &LuaVBOImpl::FromUnitDefID,
		"FromFeatureDefID", &LuaVBOImpl::FromFeatureDefID,

		"BindBufferRange", &LuaVBOImpl::BindBufferRange,
		"UnbindBufferRange", &LuaVBOImpl::UnbindBufferRange,

		"DumpDefinition", &LuaVBOImpl::DumpDefinition
		);

	gl.set("VBO", sol::lua_nil); //because :)

	return true;
}

bool LuaVBO::CheckAndReportSupported(lua_State* L, const unsigned int target) {
	#define ValStr(arg) { arg, #arg }
	#define ValStr2(arg1, arg2) { arg1, #arg2 }

	static std::unordered_map<GLenum, std::string> bufferEnumToStr = {
		ValStr(GL_ARRAY_BUFFER),
		ValStr(GL_ELEMENT_ARRAY_BUFFER),
		ValStr(GL_UNIFORM_BUFFER),
		ValStr(GL_SHADER_STORAGE_BUFFER),
	};

	static std::unordered_map<GLenum, std::string> bufferEnumToExtStr = {
		ValStr2(GL_ARRAY_BUFFER, ARB_vertex_buffer_object),
		ValStr2(GL_ELEMENT_ARRAY_BUFFER, ARB_vertex_buffer_object),
		ValStr2(GL_UNIFORM_BUFFER, ARB_uniform_buffer_object),
		ValStr2(GL_SHADER_STORAGE_BUFFER, ARB_shader_storage_buffer_object),
	};

	if (bufferEnumToStr.find(target) == bufferEnumToStr.cend()) {
		LOG_L(L_ERROR, "[LuaVBO:%s]: Supplied invalid OpenGL buffer type [%u]", __func__, target);
		return false;
	}

	if (!LuaVBOImpl::Supported(target)) {
		LOG_L(L_ERROR, "[LuaVBO:%s]: important OpenGL extension %s is not supported for buffer type %s", __func__, bufferEnumToExtStr[target].c_str(), bufferEnumToStr[target].c_str());
		return false;
	}

	return true;

	#undef ValStr
	#undef ValStr2
}

int LuaVBO::GetVBO(lua_State* L)
{
	unsigned int target = luaL_optint(L, 1, GL_ARRAY_BUFFER);
	if (!LuaVBO::CheckAndReportSupported(L, target))
		return 0;

	return sol::stack::call_lua(L, 1, [L](const sol::optional<GLenum> defTargetOpt, const sol::optional<bool> freqUpdatedOpt) {
		return std::make_shared<LuaVBOImpl>(defTargetOpt, freqUpdatedOpt, L);
		//return LuaVBOImpl{defTargetOpt, freqUpdatedOpt, L};
	});
}