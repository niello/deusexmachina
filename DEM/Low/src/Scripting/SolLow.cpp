#include "SolLow.h"
#include <Input/InputTranslator.h>
#include <Animation/AnimationController.h>
#include <UI/UIWindow.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <Frame/Lights/PointLightAttribute.h>
#include <Scene/SceneNode.h>
#include <Scripting/LuaEventHandler.h>
#include <Events/Signal.h>
#include <Math/Vector3.h>
#include <Data/DataArray.h>
#include <Data/ParamsUtils.h> // CParams::ToString

namespace DEM::Scripting
{

static sol::object MakeObjectFromData(sol::state_view& s, const Data::CData& Data)
{
	if (auto* pVal = Data.As<bool>())
		return sol::make_object(s, *pVal);
	if (auto* pVal = Data.As<int>())
		return sol::make_object(s, *pVal);
	if (auto* pVal = Data.As<float>())
		return sol::make_object(s, *pVal);
	if (auto* pVal = Data.As<std::string>())
		return sol::make_object(s, *pVal);
	if (auto* pVal = Data.As<CStrID>())
		return sol::make_object(s, *pVal);
	if (auto* pVal = Data.As<Data::PParams>())
		return sol::make_object(s, pVal->Get());
	if (auto* pVal = Data.As<Data::PDataArray>())
		return sol::make_object(s, pVal->Get());

	return sol::object();
}
//---------------------------------------------------------------------

void RegisterBasicTypes(sol::state& State)
{
	State.set_function("RandomFloat", sol::overload(
		sol::resolve<float()>(Math::RandomFloat),
		sol::resolve<float(float, float)>(Math::RandomFloat)));

	State.new_usertype<vector3>("vector3"
		, sol::constructors<sol::types<>, sol::types<float, float, float>>()
		, sol::meta_function::index, [](const vector3& Self, size_t i) { return Self.v[i]; }
		, "x", &vector3::x
		, "y", &vector3::y
		, "z", &vector3::z
	);
	sol::table vector3Table = State["vector3"];
	vector3Table.set("Zero", vector3::Zero);
	vector3Table.set("One", vector3::One);
	vector3Table.set("Up", vector3::Up);
	vector3Table.set("AxisX", vector3::AxisX);
	vector3Table.set("AxisY", vector3::AxisY);
	vector3Table.set("AxisZ", vector3::AxisZ);
	vector3Table.set("BaseDir", vector3::BaseDir);

	State.new_usertype<rtm::vector4f>("vector4f"
		, "new", sol::initializers(
			[](rtm::vector4f& Self, float x, float y, float z) { Self = rtm::vector_set(x, y, z); },
			[](rtm::vector4f& Self, float x, float y, float z, float w) { Self = rtm::vector_set(x, y, z, w); }
			)
		, "x", [](rtm::vector4f& Self) { return rtm::vector_get_x(Self); }
		, "y", [](rtm::vector4f& Self) { return rtm::vector_get_y(Self); }
		, "z", [](rtm::vector4f& Self) { return rtm::vector_get_z(Self); }
		, "w", [](rtm::vector4f& Self) { return rtm::vector_get_w(Self); }
	);

	State.new_enum<ESoftBool>("ESoftBool",
		{
			{ "False", ESoftBool::False },
			{ "True", ESoftBool::True },
			{ "Maybe", ESoftBool::Maybe },
		}
	);

	State.new_usertype<CStrID>("CStrID"
		, sol::constructors<sol::types<>, sol::types<const char*>, sol::types<const CStrID&>>()
		, sol::meta_function::to_string, &CStrID::CStr
		, sol::meta_function::length, [](CStrID ID) { return ID.CStr() ? strlen(ID.CStr()) : 0; }
		, sol::meta_function::index, [](CStrID ID, size_t i) { return ID.CStr() ? ID.CStr()[i - 1] : '\0'; }
		, sol::meta_function::concatenation, sol::overload(
			[](const char* a, CStrID b) { return std::string(a) + (b.CStr() ? b.CStr() : ""); }
			, [](CStrID a, const char* b) { return std::string(a.CStr() ? a.CStr() : "") + b; }
			, [](CStrID a, CStrID b) { return std::string(a.CStr() ? a.CStr() : "") + (b.CStr() ? b.CStr() : ""); })
		, sol::meta_function::equal_to, [](CStrID a, CStrID b) { return a == b; }
	);
	sol::table CStrIDTable = State["CStrID"];
	CStrIDTable.set("Empty", CStrID::Empty);

	{
		auto UT = State.new_usertype<Data::CParams>("CParams");
		UT.set_function(sol::meta_function::length, [](const Data::CParams& Self) { return Self.GetCount(); });
		UT.set_function(sol::meta_function::index, [](const Data::CParams& Self, const char* pKey, sol::this_state s)
		{
			if (auto pParam = Self.Find(CStrID(pKey)))
				return MakeObjectFromData(sol::state_view(s), pParam->GetRawValue());
			return sol::object();
		});
		UT.set_function(sol::meta_function::new_index, [](Data::CParams& Self, const char* pKey, sol::object Value) { NOT_IMPLEMENTED; });
		RegisterStringOperations(UT);
	}

	{
		auto UT = State.new_usertype<Data::CDataArray>("CDataArray");
		UT.set_function(sol::meta_function::length, [](const Data::CDataArray& Self) { return Self.size(); });
		UT.set_function(sol::meta_function::index, [](const Data::CDataArray& Self, size_t i, sol::this_state s)
		{
			return (Self.size() > i) ? MakeObjectFromData(sol::state_view(s), Self[i]) : sol::object();
		});
		UT.set_function(sol::meta_function::new_index, [](Data::CDataArray& Self, size_t i, sol::object Value) { NOT_IMPLEMENTED; });
	}

	State.new_usertype<::Events::CEvent>("CEvent"
		, "ID", &::Events::CEvent::ID
		, "Params", [](const ::Events::CEvent& Self) // FIXME: can make e.Params. instead of e.Params(). ? Make C++ field of type CParams, not PParams?
		{
			if (auto pParams = Self.Params.Get())
				return static_cast<const Data::CParams*>(pParams);

			static const Data::CParams EmptyParams;
			return &EmptyParams;
		}
	);

	State.new_usertype<::Events::CEventDispatcher>("CEventDispatcher"
		, "Subscribe", [](::Events::CEventDispatcher& Self, const char* pEventID, sol::function Handler)
		{
			const CStrID EventID(pEventID);
			return EventID ? Self.Subscribe(std::make_shared<::Events::CLuaEventHandler>(std::move(Handler), &Self, EventID)) : ::Events::PSub{};
		}
	);

	// TODO: add to namespace (table) DEM?
	State.set_function("UnsubscribeEvent", [](sol::object Arg)
	{
		if (Arg.is<::Events::PSub>())
		{
			::Events::PSub& Sub = Arg.as<::Events::PSub&>();
			Sub.Disconnect();
			return sol::object{};
		}
		else
		{
			n_assert2_dbg(Arg.get_type() == sol::type::nil, "Lua UnsubscribeEvent() binding received neither PSub nor nil, check your scripts for logic errors!");
			return Arg;
		}
	});

	State.new_usertype<Events::CConnection>("CConnection"
		, "IsConnected", &Events::CConnection::IsConnected
		, "Disconnect", &Events::CConnection::Disconnect
	);

	// TODO: add to namespace (table) DEM?
	State.set_function("DisconnectFromSignal", [](sol::object Arg)
	{
		if (Arg.is<Events::CConnection>())
		{
			Events::CConnection& Conn = Arg.as<Events::CConnection&>();
			Conn.Disconnect();
			return sol::object{};
		}
		else
		{
			n_assert2_dbg(Arg.get_type() == sol::type::nil, "Lua DisconnectFromSignal() binding received neither CConnection nor nil, check your scripts for logic errors!");
			return Arg;
		}
	});

	DEM::Scripting::RegisterSignalType<void()>(State);
	DEM::Scripting::RegisterSignalType<void(UI::PUIWindow)>(State);

	State.new_usertype<Input::CInputTranslator>("CInputTranslator"
		, sol::base_classes, sol::bases<::Events::CEventDispatcher>()
		, "HasContext", &Input::CInputTranslator::HasContext
		, "EnableContext", &Input::CInputTranslator::EnableContext
		, "DisableContext", &Input::CInputTranslator::DisableContext
	);

	State.new_usertype<UI::CUIWindow>("CUIWindow"
		, "AddChild", &UI::CUIWindow::AddChild
		, "RemoveChild", &UI::CUIWindow::RemoveChild
		, "Close", &UI::CUIWindow::Close
		, "Show", &UI::CUIWindow::Show
		, "Hide", &UI::CUIWindow::Hide
		, "SetFocus", &UI::CUIWindow::SetFocus
		, "OnClosed", &UI::CUIWindow::OnClosed
		, "OnOrphaned", &UI::CUIWindow::OnOrphaned
	);
	State.new_usertype<UI::CUIContext>("CUIContext"
		, "PushRootWindow", &UI::CUIContext::PushRootWindow
		, "PopRootWindow", &UI::CUIContext::PopRootWindow
		, "GetRootWindow", &UI::CUIContext::GetRootWindow
		, "RemoveRootWindow", &UI::CUIContext::RemoveRootWindow
		, "ClearRootWindowStack", &UI::CUIContext::ClearRootWindowStack
	);
	State.new_usertype<UI::CUIServer>("CUIServer"
		, "ReleaseWindow", [](UI::CUIServer& Self, UI::CUIWindow* pWnd) { Self.ReleaseWindow(Ptr(pWnd)); } // FIXME: sol - bind Ptr<Derived> to Ptr<Base>!
		, "DestroyAllReusableWindows", & UI::CUIServer::DestroyAllReusableWindows
	);

	//!!!TODO: ensure that it is passed by value as HEntity and CStrID!
	State.new_usertype<HVar>("HVar");

	// TODO: bind var storage to Lua (can make variadic template for it?)
	State.new_usertype<DEM::Anim::CAnimationController>("CAnimationController"
		, "SetString", sol::overload(
			[](DEM::Anim::CAnimationController& Self, HVar Handle, CStrID Value) { if (Handle) Self.GetParams().Set(Handle, Value); return !!Handle; },
			[](DEM::Anim::CAnimationController& Self, CStrID ID, CStrID Value) { return !!Self.GetParams().Set(ID, Value); })
	);

	State.new_usertype<Scene::CSceneNode>("CSceneNode"
		, sol::meta_function::index, [](const Scene::CSceneNode& Self, const char* pKey) { return Self.GetChild(CStrID(pKey)); } //!!!FIXME PERF: add overload with std::string_view!
		, "GetName", &Scene::CSceneNode::GetName
		, "GetChildCount", &Scene::CSceneNode::GetChildCount
		, "GetChildRecursively", &Scene::CSceneNode::GetChildRecursively
		, "GetChild", sol::overload(
			sol::resolve<Scene::CSceneNode*(UPTR) const>(&Scene::CSceneNode::GetChild),
			sol::resolve<Scene::CSceneNode*(CStrID) const>(&Scene::CSceneNode::GetChild))
		, "FindPointLight", &Scene::CSceneNode::FindFirstAttribute<Frame::CPointLightAttribute> //!!!FIXME: improve!!!
	);

	State.new_usertype<Frame::CPointLightAttribute>("CPointLightAttribute"
		, "Color", &Frame::CPointLightAttribute::_Color
		, "Intensity", &Frame::CPointLightAttribute::_Intensity
		, "Range", &Frame::CPointLightAttribute::_Range
	);
}
//---------------------------------------------------------------------

}
