/*
 * \brief  Popup dialog
 * \author Norman Feske
 * \date   2018-09-12
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VIEW__POPUP_DIALOG_H_
#define _VIEW__POPUP_DIALOG_H_

/* Genode includes */
#include <os/reporter.h>
#include <depot/archive.h>

/* local includes */
#include <types.h>
#include <model/launchers.h>
#include <model/sculpt_version.h>
#include <model/component.h>
#include <model/runtime_config.h>
#include <model/download_queue.h>
#include <model/nic_state.h>
#include <model/index_menu.h>
#include <view/dialog.h>
#include <view/activatable_item.h>
#include <depot_query.h>

#include <view/pd_route_dialog.h>
#include <view/resource_dialog.h>

namespace Sculpt { struct Popup_dialog; }


struct Sculpt::Popup_dialog : Dialog
{
	using Depot_users    = Attached_rom_dataspace;
	using Blueprint_info = Component::Blueprint_info;

	Env &_env;

	Sculpt_version const _sculpt_version { _env };

	Launchers  const &_launchers;
	Nic_state  const &_nic_state;
	Nic_target const &_nic_target;

	bool _nic_ready() const { return _nic_target.ready() && _nic_state.ready(); }

	Runtime_info   const &_runtime_info;
	Runtime_config const &_runtime_config;
	Download_queue const &_download_queue;
	Depot_users    const &_depot_users;

	Depot_query &_depot_query;

	struct Construction_info : Interface
	{
		struct With : Interface { virtual void with(Component const &) const = 0; };

		virtual void _with_construction(With const &) const = 0;

		template <typename FN>
		void with_construction(FN const &fn) const
		{
			struct _With : With {
				FN const &_fn;
				_With(FN const &fn) : _fn(fn) { }
				void with(Component const &c) const override { _fn(c); }
			};
			_with_construction(_With(fn));
		}
	};

	struct Refresh : Interface, Noncopyable
	{
		virtual void refresh_popup_dialog() = 0;
	};

	Refresh &_refresh;

	struct Action : Interface
	{
		virtual void launch_global(Path const &launcher) = 0;

		virtual Start_name new_construction(Component::Path const &pkg, Verify,
		                                    Component::Info const &info) = 0;

		struct Apply_to : Interface { virtual void apply_to(Component &) = 0; };

		virtual void _apply_to_construction(Apply_to &) = 0;

		template <typename FN>
		void apply_to_construction(FN const &fn)
		{
			struct _Apply_to : Apply_to {
				FN const &_fn;
				_Apply_to(FN const &fn) : _fn(fn) { }
				void apply_to(Component &c) override { _fn(c); }
			} apply_fn(fn);

			_apply_to_construction(apply_fn);
		}

		virtual void discard_construction() = 0;
		virtual void launch_construction() = 0;

		virtual void trigger_download(Path const &, Verify) = 0;
		virtual void remove_index(Depot::Archive::User const &) = 0;
	};

	Construction_info const &_construction_info;

	Hoverable_item   _item         { };
	Activatable_item _action_item  { };
	Activatable_item _install_item { };
	Hoverable_item   _route_item   { };
	Pd_route_dialog  _pd_route     { _runtime_config };

	Constructible<Resource_dialog> _resources { };

	enum State { TOP_LEVEL, DEPOT_REQUESTED, DEPOT_SHOWN, DEPOT_SELECTION,
	             INDEX_REQUESTED, INDEX_SHOWN,
	             PKG_REQUESTED, PKG_SHOWN, ROUTE_SELECTED };

	State _state { TOP_LEVEL };

	typedef Depot::Archive::User User;
	User _selected_user { };

	Blueprint_info _blueprint_info { };

	Component::Name _construction_name { };

	Constructible<Route::Id> _selected_route { };

	bool _route_selected(Route::Id const &id) const
	{
		return _selected_route.constructed() && id == _selected_route->string();
	}

	bool _resource_dialog_selected() const
	{
		return _route_selected("resources");
	}

	template <typename FN>
	void _apply_to_selected_route(Action &action, FN const &fn)
	{
		unsigned cnt = 0;
		action.apply_to_construction([&] (Component &component) {
			component.routes.for_each([&] (Route &route) {
				if (_route_selected(Route::Id(cnt++)))
					fn(route); }); });
	}

	Index_menu _menu { };

	Hover_result hover(Xml_node hover) override
	{
		Dialog::Hover_result const hover_result = Dialog::any_hover_changed(
			_item        .match(hover, "frame", "vbox", "hbox", "name"),
			_action_item .match(hover, "frame", "vbox", "button", "name"),
			_install_item.match(hover, "frame", "vbox", "float", "vbox", "float", "button", "name"),
			_route_item  .match(hover, "frame", "vbox", "frame", "vbox", "hbox", "name"));

		_pd_route.hover(hover, "frame", "vbox", "frame", "vbox", "hbox", "name");

		if (_resources.constructed() &&
		    hover_result == Dialog::Hover_result::UNMODIFIED)
			return _resources->match_sub_dialog(hover, "frame", "vbox", "frame", "vbox");

		return hover_result;
	}

	void depot_users_scan_updated()
	{
		if (_state == DEPOT_REQUESTED)
			_state = DEPOT_SHOWN;

		if (_state != TOP_LEVEL)
			_refresh.refresh_popup_dialog();
	}

	Attached_rom_dataspace _index_rom { _env, "report -> runtime/depot_query/index" };

	Signal_handler<Popup_dialog> _index_handler {
		_env.ep(), *this, &Popup_dialog::_handle_index };

	void _handle_index()
	{
		/* prevent modifications of index while browsing it */
		if (_state >= INDEX_SHOWN)
			return;

		_index_rom.update();

		if (_state == INDEX_REQUESTED)
			_state = INDEX_SHOWN;

		_refresh.refresh_popup_dialog();
	}

	bool _index_avail(User const &user) const
	{
		bool result = false;
		_index_rom.xml().for_each_sub_node("index", [&] (Xml_node index) {
			if (index.attribute_value("user", User()) == user)
				result = true; });
		return result;
	};

	Path _index_path(User const &user) const
	{
		return Path(user, "/index/", _sculpt_version);
	}

	void _gen_sub_menu_title(Xml_generator &xml,
	                         Start_name const &name,
	                         Start_name const &text) const
	{
		gen_named_node(xml, "hbox", name, [&] () {
			gen_named_node(xml, "float", "left", [&] () {
				xml.attribute("west", "yes");
				xml.node("hbox", [&] () {
					gen_named_node(xml, "button", "back", [&] () {
						xml.attribute("selected", "yes");
						xml.attribute("style", "back");
						_item.gen_hovered_attr(xml, name);
						xml.node("hbox", [&] () { });
					});
					gen_named_node(xml, "label", "label", [&] () {
						xml.attribute("font", "title/regular");
						xml.attribute("text", Path(" ", text));
					});
				});
			});
			gen_named_node(xml, "hbox", "right", [&] () { });
		});
	}

	void _gen_menu_entry(Xml_generator &xml, Start_name const &name,
	                     Component::Info const &text, bool selected,
	                     char const *style = "radio") const
	{
		gen_named_node(xml, "hbox", name, [&] () {

			gen_named_node(xml, "float", "left", [&] () {
				xml.attribute("west", "yes");

				xml.node("hbox", [&] () {
					gen_named_node(xml, "button", "button", [&] () {

						if (selected)
							xml.attribute("selected", "yes");

						xml.attribute("style", style);
						_item.gen_hovered_attr(xml, name);
						xml.node("hbox", [&] () { });
					});
					gen_named_node(xml, "label", "name", [&] () {
						xml.attribute("text", Path(" ", text)); });
				});
			});

			gen_named_node(xml, "hbox", "right", [&] () { });
		});
	}

	void _gen_route_entry(Xml_generator &xml,
	                      Start_name const &name,
	                      Start_name const &text,
	                      bool selected, char const *style = "radio") const
	{
		gen_named_node(xml, "hbox", name, [&] () {

			gen_named_node(xml, "float", "left", [&] () {
				xml.attribute("west", "yes");

				xml.node("hbox", [&] () {
					gen_named_node(xml, "button", "button", [&] () {

						if (selected)
							xml.attribute("selected", "yes");

						xml.attribute("style", style);
						_route_item.gen_hovered_attr(xml, name);
						xml.node("hbox", [&] () { });
					});
					gen_named_node(xml, "label", "name", [&] () {
						xml.attribute("text", Path(" ", text)); });
				});
			});

			gen_named_node(xml, "hbox", "right", [&] () { });
		});
	}

	template <typename FN>
	void _for_each_menu_item(FN const &fn) const
	{
		_menu.for_each_item(_index_rom.xml(), _selected_user, fn);
	}

	static void _gen_info_label(Xml_generator &xml, char const *name,
	                            Component::Info const &info)
	{
		gen_named_node(xml, "label", name, [&] () {
			xml.attribute("font", "annotation/regular");
			xml.attribute("text", Component::Info(" ", info, " ")); });
	}

	void _gen_pkg_info     (Xml_generator &, Component const &) const;
	void _gen_pkg_elements (Xml_generator &, Component const &) const;
	void _gen_menu_elements(Xml_generator &, Xml_node const &depot_users) const;

	void generate(Xml_generator &xml) const override
	{
		xml.node("frame", [&] () {
			xml.node("vbox", [&] () {
				_gen_menu_elements(xml, _depot_users.xml()); }); });
	}

	void click(Action &action);

	void clack(Action &action)
	{
		_action_item.confirm_activation_on_clack();
		_install_item.confirm_activation_on_clack();

		if (_action_item.activated("launch")) {
			action.launch_construction();
			reset();
		}

		bool const pkg_need_install = !_blueprint_info.pkg_avail
		                            || _blueprint_info.incomplete();

		if (pkg_need_install && _install_item.activated("install")) {
			_construction_info.with_construction([&] (Component const &component) {
				action.trigger_download(component.path, component.verify);
				_install_item.reset();
				_refresh.refresh_popup_dialog();
			});
		}
	}

	void reset() override
	{
		_item._hovered = Hoverable_item::Id();
		_route_item._hovered = Hoverable_item::Id();
		_action_item.reset();
		_install_item.reset();
		_state = TOP_LEVEL;
		_selected_user = User();
		_selected_route.destruct();
		_menu._level = 0;
		_resources.destruct();
		_pd_route.reset();
	}

	Popup_dialog(Env &env, Refresh &refresh,
	             Launchers         const &launchers,
	             Nic_state         const &nic_state,
	             Nic_target        const &nic_target,
	             Runtime_info      const &runtime_info,
	             Runtime_config    const &runtime_config,
	             Download_queue    const &download_queue,
	             Depot_users       const &depot_users,
	             Depot_query             &depot_query,
	             Construction_info const &construction_info)
	:
		_env(env), _launchers(launchers),
		_nic_state(nic_state), _nic_target(nic_target),
		_runtime_info(runtime_info), _runtime_config(runtime_config),
		_download_queue(download_queue), _depot_users(depot_users),
		_depot_query(depot_query),
		_refresh(refresh), _construction_info(construction_info)
	{
		_index_rom.sigh(_index_handler);
	}

	bool depot_query_needs_users() const { return _state >= TOP_LEVEL; }

	void gen_depot_query(Xml_generator &xml, Xml_node const &depot_users) const
	{
		if (_state >= TOP_LEVEL)
			depot_users.for_each_sub_node("user", [&] (Xml_node user) {
				xml.node("index", [&] () {
					User const name = user.attribute_value("name", User());
					xml.attribute("user",    name);
					xml.attribute("version", _sculpt_version);
					if (_state >= INDEX_REQUESTED && _selected_user == name)
						xml.attribute("content", "yes");
				});
			});

		if (_state >= PKG_REQUESTED)
			_construction_info.with_construction([&] (Component const &component) {
				xml.node("blueprint", [&] () {
					xml.attribute("pkg", component.path); }); });
	}

	void apply_blueprint(Component &construction, Xml_node blueprint)
	{
		if (_state < PKG_REQUESTED)
			return;

		_resources.construct(construction.affinity_space,
		                     construction.affinity_location,
		                     construction.priority);

		construction.try_apply_blueprint(blueprint);

		_blueprint_info = construction.blueprint_info;

		if (_blueprint_info.ready_to_deploy() && _state == PKG_REQUESTED)
			_state = PKG_SHOWN;

		_refresh.refresh_popup_dialog();
	}

	bool interested_in_file_operations() const
	{
		return _state == DEPOT_SELECTION;
	}
};

#endif /* _VIEW__POPUP_DIALOG_H_ */
