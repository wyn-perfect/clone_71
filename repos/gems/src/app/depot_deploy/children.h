/*
 * \brief  State tracking of subsystems deployed from depot packages
 * \author Norman Feske
 * \date   2018-01-23
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CHILDREN_H_
#define _CHILDREN_H_

/* Genode includes */
#include <util/list_model.h>
#include <util/xml_generator.h>
#include <base/service.h>
#include <depot/archive.h>

/* local includes */
#include "child.h"

namespace Depot_deploy { class Children; }


class Depot_deploy::Children
{
	private:

		Allocator &_alloc;

		List_model<Child> _children { };

		struct Model_update_policy : List_model<Child>::Update_policy
		{
			Allocator &_alloc;

			unsigned num_changes = 0;

			Model_update_policy(Allocator &alloc) : _alloc(alloc) { }

			void destroy_element(Child &c)
			{
				num_changes++;
				destroy(_alloc, &c);
			}

			Child &create_element(Xml_node node)
			{
				num_changes++;
				return *new (_alloc) Child(_alloc, node);
			}

			void update_element(Child &child, Xml_node node)
			{
				if (child.apply_config(node))
					num_changes++;
			}

			static bool element_matches_xml_node(Child const &child, Xml_node node)
			{
				return node.attribute_value("name", Child::Name()) == child.name();
			}

			static bool node_is_element(Xml_node node) { return node.has_type("start"); }

		} _model_update_policy { _alloc };

	public:

		Children(Allocator &alloc) : _alloc(alloc) { }

		/*
		 * \return true if config had any effect
		 */
		bool apply_config(Xml_node config)
		{
			unsigned const orig_num_changes = _model_update_policy.num_changes;

			_children.update_from_xml(_model_update_policy, config);

			return (orig_num_changes != _model_update_policy.num_changes);
		}

		/*
		 * \return true if launcher had any effect
		 */
		bool apply_launcher(Child::Launcher_name const &name, Xml_node launcher)
		{
			bool any_child_changed = false;

			_children.for_each([&] (Child &child) {
				if (child.apply_launcher(name, launcher))
					any_child_changed = true; });

			return any_child_changed;
		}

		/*
		 * \return true if blueprint had an effect on any child
		 */
		bool apply_blueprint(Xml_node const &blueprint)
		{
			bool any_child_changed = false;

			blueprint.for_each_sub_node("pkg", [&] (Xml_node pkg) {
				_children.for_each([&] (Child &child) {
					if (child.apply_blueprint(pkg))
						any_child_changed = true; }); });

			blueprint.for_each_sub_node("missing", [&] (Xml_node missing) {
				_children.for_each([&] (Child &child) {
					if (child.mark_as_incomplete(missing))
						any_child_changed = true; }); });

			return any_child_changed;
		}

		/*
		 * \return true if the condition of any child changed
		 */
		template <typename COND_FN>
		bool apply_condition(COND_FN const &fn)
		{
			bool any_condition_changed = false;
			_children.for_each([&] (Child &child) {
				any_condition_changed |= child.apply_condition(fn); });
			return any_condition_changed;
		}

		/**
		 * Call 'fn' with start 'Xml_node' of each child that has an
		 * unsatisfied start condition.
		 */
		template <typename FN>
		void for_each_unsatisfied_child(FN const &fn) const
		{
			_children.for_each([&] (Child const &child) {
				child.apply_if_unsatisfied(fn); });
		}

		void reset_incomplete()
		{
			_children.for_each([&] (Child &child) {
				child.reset_incomplete(); });
		}

		void gen_start_nodes(Xml_generator &xml, Xml_node common,
		                     Child::Prio_levels prio_levels,
		                     Affinity::Space affinity_space,
		                     Child::Depot_rom_server const &cached_depot_rom,
		                     Child::Depot_rom_server const &uncached_depot_rom) const
		{
			_children.for_each([&] (Child const &child) {
				child.gen_start_node(xml, common, prio_levels, affinity_space,
				                     cached_depot_rom, uncached_depot_rom); });
		}

		void gen_queries(Xml_generator &xml) const
		{
			_children.for_each([&] (Child const &child) {
				child.gen_query(xml); });
		}

		void gen_installation_entries(Xml_generator &xml) const
		{
			_children.for_each([&] (Child const &child) {
				child.gen_installation_entry(xml); });
		}

		template <typename FN>
		void for_each_missing_pkg_path(FN const &fn) const
		{
			_children.for_each([&] (Child const &child) {
				child.with_missing_pkg_path(fn); });
		}

		size_t count() const
		{
			size_t count = 0;
			_children.for_each([&] (Child const &) {
				++count; });
			return count;
		}

		bool any_incomplete() const {

			bool result = false;
			_children.for_each([&] (Child const &child) {
				result |= child.incomplete(); });
			return result;
		}

		bool any_blueprint_needed() const
		{
			bool result = false;
			_children.for_each([&] (Child const &child) {
				result |= child.blueprint_needed(); });
			return result;
		}

		bool exists(Child::Name const &name) const
		{
			bool result = false;
			_children.for_each([&] (Child const &child) {
				if (child.name() == name)
					result = true; });
			return result;
		}
};

#endif /* _CHILDREN_H_ */
