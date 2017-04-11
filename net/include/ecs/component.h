#pragma once
#include "defines.h"
#include "data/value.h"
// ECS
namespace VK {
	struct IComponent {
		virtual ~IComponent() {}
		virtual component_id_t get_id() const = 0;		
	};

	// component implement
	template<component_id_t ComponentID>
	class Component : IComponent {
	public:
		component_id_t get_id() const override { return ComponentID; }

		template<typename T>
		T get() {
			
		}

	private:
	};

	// entity implement
	class Entity {
		
	};
}