#pragma once
#include "SolLow.h"
#include <Input/InputTranslator.h>
#include <UI/UIWindow.h>
#include <UI/UIContext.h>
#include <Scripting/LuaEventHandler.h>
#include <Events/Signal.h>
#include <Math/Vector3.h>
#include <Data/DataArray.h>
#include <Data/Buffer.h>
#include <IO/HRDWriter.h>
#include <IO/Streams/MemStream.h>

namespace DEM::Scripting
{

static sol::object MakeObjectFromData(sol::state_view& s, const Data::CData& Data)
{
	if (auto pVal = Data.As<bool>())
		return sol::make_object(s, *pVal);
	if (auto pVal = Data.As<int>())
		return sol::make_object(s, *pVal);
	if (auto pVal = Data.As<float>())
		return sol::make_object(s, *pVal);
	if (auto pVal = Data.As<CString>())
		return sol::make_object(s, pVal->CStr() ? pVal->CStr() : "");
	if (auto pVal = Data.As<CStrID>())
		return sol::make_object(s, *pVal);
	if (auto pVal = Data.As<Data::PParams>())
		return sol::make_object(s, pVal->Get());
	if (auto pVal = Data.As<Data::PDataArray>())
		return sol::make_object(s, pVal->Get());

	return sol::object();
}
//---------------------------------------------------------------------

void RegisterBasicTypes(sol::state& State)
{
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

	State.new_usertype<Data::CParams>("CParams"
		, sol::meta_function::length, [](const Data::CParams& Self) { return Self.GetCount(); }
		, sol::meta_function::index, [](const Data::CParams& Self, const char* pKey, sol::this_state s)
		{
			if (auto pParam = Self.Find(CStrID(pKey)))
				return MakeObjectFromData(sol::state_view(s), pParam->GetRawValue());
			return sol::object();
		}
		, sol::meta_function::new_index, [](Data::CParams& Self, const char* pKey, sol::object Value) {}
		, sol::meta_function::to_string, [](const Data::CParams& Self)
		{
			// TODO: need to improve API for writing into memory (string)!
			IO::CMemStream Stream;
			IO::CHRDWriter Writer(Stream);
			if (!Writer.WriteParams(Self)) return std::string{};
			auto Buf = Stream.Detach();
			return std::string(static_cast<const char*>(Buf->GetConstPtr()), Buf->GetSize());
		}
	);

	State.new_usertype<Data::CDataArray>("CDataArray"
		, sol::meta_function::length, [](const Data::CDataArray& Self) { return Self.GetCount(); }
		, sol::meta_function::index, [](const Data::CDataArray& Self, size_t i, sol::this_state s)
		{
			return (Self.GetCount() > i) ? MakeObjectFromData(sol::state_view(s), Self[i]) : sol::object();
		}
		, sol::meta_function::new_index, [](Data::CDataArray& Self, size_t i, sol::object Value) {}
	);

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

	State.new_usertype<::Events::CSubscription>("CSubscription"
		, sol::meta_function::less_than, [](const ::Events::CSubscription& a, const ::Events::CSubscription& b) { return &a < &b; }
		, sol::meta_function::less_than_or_equal_to, [](const ::Events::CSubscription& a, const ::Events::CSubscription& b) { return &a <= &b; }
		, sol::meta_function::equal_to, [](const ::Events::CSubscription& a, const ::Events::CSubscription& b) { return &a == &b; }
	);

	State.new_usertype<::Events::CEventDispatcher>("CEventDispatcher"
		, "Subscribe", [](::Events::CEventDispatcher& Self, const char* pEventID, sol::function Handler)
		{
			return Self.Subscribe(CStrID(pEventID), n_new(::Events::CLuaEventHandler(std::move(Handler))));
		}
	);

	// TODO: add to namespace (table) DEM?
	State.set_function("UnsubscribeEvent", [](sol::object Arg)
	{
		if (Arg.is<::Events::PSub>())
		{
			::Events::PSub& Sub = Arg.as<::Events::PSub&>();
			Sub = nullptr;
			return sol::object{};
		}
		else
		{
			n_assert2_dbg(Arg.get_type() == sol::type::nil, "Lua UnsubscribeEvent() binding received neither PSub nor nil, check your scripts for logic errors!");
			return Arg;
		}
	});

	State.new_usertype<Input::CInputTranslator>("CInputTranslator"
		, sol::base_classes, sol::bases<::Events::CEventDispatcher>()
		, "HasContext", &Input::CInputTranslator::HasContext
		, "EnableContext", &Input::CInputTranslator::EnableContext
		, "DisableContext", &Input::CInputTranslator::DisableContext
	);

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
}
//---------------------------------------------------------------------

}
